/**
  * Win32 smart dialog wrapper
  *
  * Pixel-less, dynamic layout abstraction over Ye Olde Win32 window API.
  */

#pragma once

#include <vector>

namespace SmartDlg {
	// It makes little sense for things like area or padding to accept
	// negative values.
	struct unsigned_point_t {
		unsigned int x = 0;
		unsigned int y = 0;
	};

	struct unsigned_rect_t {
		unsigned int left = 0;
		unsigned int top = 0;
		unsigned int right = 0;
		unsigned int bottom = 0;
	};

	// Good luck trying to do the same with templates.
#define CACHED(type, name, get_method, update_method) \
	private: \
		type name; \
	public: \
		bool name##_stale = true; \
		\
		const type& get_method() { \
			if(name##_stale) { \
				update_method(name); \
				name##_stale = false; \
			} \
			return name; \
		}

	/// Fonts
	/// -----
	// Base class
	class Font {
	protected:
		void create(const LOGFONTW *lf);
	public:
		HDC hDC = GetDC(nullptr);
		LONG height = 0;
		LONG pad = 0;
		HFONT hFont = nullptr;

		void getPadding(unsigned_rect_t &padding);

		~Font();
	};

	// Default Windows dialog font
	class FontDefault : public Font {
	public:
		FontDefault::FontDefault();
	};
	/// -----

	/// Base classes
	/// ------------
	// Base class for all objects in a window's hierarchy
	class Base {
	protected:
		virtual void updateArea(unsigned_point_t &area) = 0;
		virtual void updatePos(POINT &pos_abs) {
			pos_abs.x = 0;
			pos_abs.y = 0;
		}
		virtual void updatePadding(unsigned_rect_t &padding) {
			getFont().getPadding(padding);
		}

	public:
		Base *parent = nullptr;

		CACHED(unsigned_point_t, area, getArea, updateArea);
		CACHED(unsigned_rect_t, padding, getPadding, updatePadding);
		CACHED(POINT, pos, getPos, updatePos);

		virtual unsigned_point_t getAreaPadded();
		virtual POINT getPosPadded();

		virtual Font& getFont() {
			assert(parent != nullptr);
			return parent->getFont();
		}

		virtual void applyFontRecursive() = 0;
		virtual void createRecursive(HWND hWndParent) = 0;

		virtual void addChild(Base *child) = 0;
	};

	// Base class for all displayed widgets
	class BaseWidget : public Base {
	private:
		virtual LPCSTR CLASS_NAME() { return NULL; }

	public:
		Base *child = nullptr;

		HWND hWnd = nullptr;
		const char *text = nullptr;
		DWORD style = 0;
		DWORD style_ex = 0;

		virtual void applyFontRecursive();
		virtual void createRecursive(HWND hWndParent);

		virtual void addChild(Base *w);

		BaseWidget(Base *parent) {
			if(parent) {
				parent->addChild(this);
			}
		}
	};

	// Base class for layout groups
	class BaseGroup : public Base {
	protected:
		std::vector<Base *> children;

	public:
		virtual void applyFontRecursive();
		virtual void createRecursive(HWND hWndParent);

		virtual void addChild(Base *w);

		BaseGroup(Base *parent) {
			assert(parent);
			parent->addChild(this);
		}
	};
	/// ------------

	/// Label
	/// -----
	class Label : public BaseWidget {
	private:
		virtual void updateArea(unsigned_point_t &area);

		virtual LPCSTR CLASS_NAME() { return "Static"; }

	public:
		Label(Base *parent, const char *str) : BaseWidget(parent) {
			text = str;
		}
	};
	/// -----

	/// Top-level dialog window
	/// -----------------------
	class Top : public BaseWidget {
	private:
		virtual void updateArea(unsigned_point_t &area);
		virtual void updatePos(POINT &pos_abs);
		virtual void updatePadding(unsigned_rect_t &padding);

		virtual LPCSTR CLASS_NAME() { return MAKEINTRESOURCEA(WC_DIALOG); }

	protected:
		FontDefault font;

	public:
		HANDLE event_created = CreateEvent(nullptr, true, false, nullptr);

		virtual Font& getFont() { return font; }

		// The Win32 API demands that the message loop must be run in the
		// same thread that called CreateWindow(), so we might as well
		// combine both into a single function.
		virtual WPARAM create_and_run();

		virtual void close();

		Top() : BaseWidget(nullptr) {
			style |= WS_OVERLAPPED;
		}

		~Top() {
			CloseHandle(event_created);
		}
	};
}

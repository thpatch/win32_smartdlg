/**
  * Win32 smart dialog wrapper
  *
  * Pixel-less, dynamic layout abstraction over Ye Olde Win32 window API.
  */

#include <win32_utf8.h>
#include "smartdlg.hpp"

namespace SmartDlg {
	HMODULE hMod = GetModuleHandle(nullptr);

	LRESULT CALLBACK DlgProc(
		HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam
	)
	{
		switch(uMsg) {
		case WM_CLOSE: // Yes, these are not handled by DefDlgProc().
			DestroyWindow(hWnd);
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		}
		return DefDlgProcW(hWnd, uMsg, wParam, lParam);
	}

	/// Fonts
	/// -----
	void Font::create(const LOGFONTW *lf)
	{
		assert(lf);

		hFont = CreateFontIndirectW(lf);
		if(hFont) {
			SelectObject(hDC, hFont);
		}
		height = (lf->lfHeight < 0 ? -lf->lfHeight : lf->lfHeight);
		pad = height / 2;
	}

	void Font::getPadding(unsigned_rect_t &padding)
	{
		padding.top = pad;
		padding.left = pad;
		padding.right = pad;
		padding.bottom = pad;
	}

	Font::~Font()
	{
		if(hFont) {
			DeleteObject(hFont);
		}
	}

	FontDefault::FontDefault()
	{
		NONCLIENTMETRICSW nc_metrics = {sizeof(nc_metrics)};
		if(SystemParametersInfoW(
			SPI_GETNONCLIENTMETRICS, sizeof(nc_metrics), &nc_metrics, 0
		)) {
			create(&nc_metrics.lfMessageFont);
		}
	}
	/// -----

	/// Base classes
	/// ------------
	unsigned_point_t Base::getAreaPadded()
	{
		auto ret = getArea();
		const auto& pad = getPadding();
		ret.x += pad.left + pad.right;
		ret.y += pad.top + pad.bottom;
		return ret;
	}

	POINT Base::getPosPadded()
	{
		POINT ret = getPos();
		const auto& pad = getPadding();
		ret.x += pad.left;
		ret.y += pad.right;
		return ret;
	}

	void BaseWidget::applyFontRecursive()
	{
		const auto &font = getFont();
		SendMessageW(hWnd, WM_SETFONT, (WPARAM)font.hFont, 0);
		if(child) {
			child->applyFontRecursive();
		}
	}

	void BaseWidget::createRecursive(HWND hWndParent)
	{
		const auto &pos = getPosPadded();
		const auto &area = getArea();
		if(hWndParent) {
			style |= WS_CHILD | WS_VISIBLE;
			style_ex |= WS_EX_NOPARENTNOTIFY;
		}
		hWnd = CreateWindowExU(
			style_ex, CLASS_NAME(), text, style,
			pos.x, pos.y, area.x, area.y,
			hWndParent, nullptr, hMod, nullptr
		);
		if(child) {
			child->createRecursive(hWnd);
		}
	}

	void BaseWidget::addChild(Base *w)
	{
		assert(child == nullptr);
		child = w;
		w->parent = this;
	}
	/// -----------------

	/// Label
	/// -----
	void Label::updateArea(unsigned_point_t &area)
	{
		assert(area_stale); // Call getArea() instead!

		RECT rect = {0};
		const auto& font = getFont();
		DrawText(font.hDC, text, -1, &rect, DT_CALCRECT);
		area.x = rect.right;
		area.y = rect.bottom;
	}
	/// -----

	/// Top-level dialog window
	/// -----------------------
	void Top::updateArea(unsigned_point_t &area)
	{
		assert(child);
		assert(area_stale); // Call getArea() instead!

		auto child_area = child->getAreaPadded();
		RECT self = {0, 0, child_area.x, child_area.y};
		AdjustWindowRectEx(&self, style, false, style_ex);
		area.x = self.right - self.left;
		area.y = self.bottom - self.top;
	}

	void Top::updatePos(POINT &pos_abs)
	{
		assert(pos_stale); // Call getPos() instead!

		const auto &area = getArea();
		RECT screen;
		if(!SystemParametersInfoW(SPI_GETWORKAREA, sizeof(RECT), &screen, 0)) {
			screen.right = GetSystemMetrics(SM_CXSCREEN);
			screen.bottom = GetSystemMetrics(SM_CYSCREEN);
		}
		pos_abs.x = (screen.right / 2) - (area.x / 2);
		pos_abs.y = (screen.bottom / 2) - (area.y / 2);
	}

	void Top::updatePadding(unsigned_rect_t &padding)
	{
		ZeroMemory(&padding, sizeof(padding));
	}

	WPARAM Top::create_and_run()
	{
		createRecursive(nullptr);
		applyFontRecursive();
		ShowWindow(hWnd, SW_SHOW);
		UpdateWindow(hWnd);

		SetWindowLongPtrW(hWnd, GWLP_WNDPROC, (LPARAM)DlgProc);

		SetEvent(event_created);

		MSG msg;
		BOOL msg_ret;

		while((msg_ret = GetMessage(&msg, nullptr, 0, 0)) != 0) {
			if(msg_ret != -1) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		return msg.wParam;
	}

	void Top::close()
	{
		if(hWnd) {
			SendMessageW(hWnd, WM_CLOSE, 0, 0);
		}
	}
	/// ----------------------
}

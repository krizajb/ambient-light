#pragma once

#include <Windows.h>

// See https://msdn.microsoft.com/en-us/library/windows/desktop/ff381400(v=vs.85).aspx

template <class DERIVED_TYPE>
class BaseWindow
{
public:
	static LRESULT CALLBACK WindowProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
	{
		DERIVED_TYPE *pThis = nullptr;

		if ( uMsg == WM_NCCREATE )
		{
			CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
			pThis = (DERIVED_TYPE*)pCreate->lpCreateParams;
			SetWindowLongPtr( hwnd, GWLP_USERDATA, (LONG_PTR)pThis );

			pThis->m_hwnd = hwnd;
		}
		else
		{
			pThis = (DERIVED_TYPE*)GetWindowLongPtr( hwnd, GWLP_USERDATA );
		}
		if ( pThis )
		{
			return pThis->HandleMessage( uMsg, wParam, lParam );
		}
		else
		{
			return DefWindowProc( hwnd, uMsg, wParam, lParam );
		}
	}

	BaseWindow() : m_hwnd( nullptr ) { }
	virtual ~BaseWindow() { }

	BOOL Create(
		PCWSTR lpWindowName,
		DWORD dwStyle,
		DWORD dwExStyle = 0,
		int x = CW_USEDEFAULT,
		int y = CW_USEDEFAULT,
		int nWidth = CW_USEDEFAULT,
		int nHeight = CW_USEDEFAULT,
		HWND hWndParent = nullptr,
		HMENU hMenu = nullptr
	)
	{
		WNDCLASS wc ={ 0 };

		wc.lpfnWndProc   = DERIVED_TYPE::WindowProc;
		wc.hInstance     = GetModuleHandle( nullptr );
		wc.lpszClassName = ClassName();

		if ( RegisterClass( &wc ) )
		{
			m_hwnd = CreateWindowEx(
				dwExStyle, ClassName(), lpWindowName, dwStyle, x, y,
				nWidth, nHeight, hWndParent, hMenu, GetModuleHandle( nullptr ), this
			);
		}

		return ( m_hwnd ? TRUE : FALSE );
	}

	HWND WindowHandle() const
	{
		return m_hwnd;
	}

protected:
	virtual PCWSTR  ClassName() const = 0;
	virtual LRESULT HandleMessage( UINT uMsg, WPARAM wParam, LPARAM lParam ) = 0;

	HWND m_hwnd;
};


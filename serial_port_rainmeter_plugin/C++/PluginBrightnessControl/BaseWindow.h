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

	BaseWindow( void ) : m_hwnd( nullptr )
	{
	}

	virtual ~BaseWindow()
	{
		if ( registered )
		{
			UnregisterClass( this->wc.lpszClassName, this->wc.hInstance );
			registered = false;
		}
	}

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
		PCWSTR className = this->ClassName();

		this->wc.lpfnWndProc   = DERIVED_TYPE::WindowProc;
		this->wc.hInstance     = GetModuleHandle( nullptr );
		this->wc.lpszClassName = className;

		if ( RegisterClass( &this->wc ) )
		{
			this->m_hwnd = CreateWindowEx(
				dwExStyle, className, lpWindowName, dwStyle, x, y,
				nWidth, nHeight, hWndParent, hMenu, GetModuleHandle( nullptr ), this
			);
			this->registered = true;
		}
		else
		{
			this->registered = false;
		}

		return ( this->m_hwnd ? TRUE : FALSE );
	}

	HWND WindowHandle( void ) const
	{
		return this->m_hwnd;
	}

protected:
	virtual PCWSTR  ClassName( void ) const = 0;
	virtual LRESULT HandleMessage( UINT uMsg, WPARAM wParam, LPARAM lParam ) = 0;

	HWND m_hwnd;
	// Windows class for registration
	WNDCLASS wc ={ 0 };

private:
	// Register flag for #wc
	bool registered = false;
};


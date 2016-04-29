#include "WindowsEvents.h"

#include <atlstr.h>
#include "../../API/RainmeterAPI.h"
#include "Measure.h"


#include <iostream>

Window::Window( std::list<Measure*> &measures )
	: measures( measures )
{
}

PCWSTR Window::ClassName() const
{
	return L"HiddenWindow";
}

void Window::Notify( const bool value ) const
{
	std::list<Measure*>::const_iterator it;

	for ( it = measures.begin(); it != measures.end(); ++it )
	{
		( *it )->WindowsEventHandler( value );
	}
}

LRESULT Window::HandleMessage( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	if ( uMsg == WM_POWERBROADCAST )
	{
		CString report;
		report.Format( L"WM_POWERBROADCAST wParam '%d' lParam '%d'", wParam, lParam );

		RmLog( LOG_DEBUG, report );

		// Resumed from suspend (sleep) state
		if ( PBT_APMRESUMEAUTOMATIC == wParam )
		{
			report.Format( L"PBT_APMRESUMEAUTOMATIC" );
			RmLog( LOG_DEBUG, report );

			this->Notify( false );
		}
		else if ( PBT_APMRESUMESUSPEND == wParam )
		{
			report.Format( L"PBT_APMRESUMESUSPEND" );
			RmLog( LOG_DEBUG, report );
		}
		// Entering suspend (sleep) state
		else if ( PBT_APMSUSPEND == wParam )
		{
			report.Format( L"PBT_APMSUSPEND" );
			RmLog( LOG_DEBUG, report );

			this->Notify( true );
		}
		//Do something
	}

	return TRUE;//DefWindowProc( this->WindowHandle(), uMsg, wParam, lParam );
}

//static LRESULT CALLBACK  WindowProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
//{
//
//
//	if ( message == WM_POWERBROADCAST )
//	{
//		CString report;
//		report.Format( L"WM_POWERBROADCAST wParam '%d' lParam '%d'", wParam, lParam );
//
//		RmLog( LOG_DEBUG, report );
//
//		// Resumed from suspend (sleep) state
//		if ( PBT_APMRESUMEAUTOMATIC == wParam )
//		{
//			report.Format( L"PBT_APMRESUMEAUTOMATIC" );
//
//			RmLog( LOG_DEBUG, report );
//			
//		}
//		else if ( PBT_APMRESUMESUSPEND == wParam )
//		{
//			report.Format( L"PBT_APMRESUMESUSPEND" );
//			RmLog( LOG_DEBUG, report );
//		}
//		// Entering suspend (sleep) state
//		else if ( PBT_APMSUSPEND == wParam )
//		{
//			report.Format( L"PBT_APMSUSPEND" );
//			RmLog( LOG_DEBUG, report );
//		}
//		//Do something
//
//
//
//		return DefWindowProc( hWnd, message, wParam, lParam );;
//
//	}
//	else
//	{
//		return DefWindowProc( hWnd, message, wParam, lParam );
//	}	
//}

//int _tmain( int argc, _TCHAR* argv[] )
//{
//	WNDCLASS wc ={ 0 };
//
//
//	// Set up and register window class
//	wc.lpfnWndProc = WindowProc;
//	wc.lpszClassName = _T( "SomeNameYouInvented" );
//	RegisterClass( &wc );
//	HWND hWin = CreateWindow( _T( "SomeNameYouInvented" ), _T( "" ), 0, 0, 0, 0, 0, NULL, NULL, NULL, 0 );
//
//	BOOL bRet;
//	MSG msg;
//	while ( ( bRet = GetMessage( &msg, hWin, 0, 0 ) ) != 0 )
//	{
//		if ( bRet == -1 )
//		{
//			// handle the error and possibly exit
//		}
//		else
//		{
//			TranslateMessage( &msg );
//			DispatchMessage( &msg );
//		}
//	}
//	return 0;
//}

unsigned int WindowsEvents::user_idle_time = 2000000;

WindowsEvents::WindowsEvents()
{
	this->activityThread = std::thread( &WindowsEvents::ActivityMain, this );
	this->sleepThread = std::thread( &WindowsEvents::SleepMain, this );
}

void WindowsEvents::RegisterMeasure( Measure* measure )
{
	this->measures.push_back(measure);
}

void WindowsEvents::SetUserIdleTime( unsigned const int ms )
{
	WindowsEvents::user_idle_time = ms;
}

WindowsEvents::~WindowsEvents()
{
	this->exit = true;
	// closes GetMessage queue
	PostMessage( hidden_window->WindowHandle(), WM_CLOSE, 0, 0 );

	if ( sleepThread.joinable() )
	{
		sleepThread.join();
	}

	if ( this->activityThread.joinable() )
	{
		this->activityThread.join();
	}
}

void WindowsEvents::SleepMain( void )
{
	//WNDCLASS wc ={ 0 };

	//// Set up and register window class
	//wc.lpfnWndProc = WindowProc;
	//wc.lpszClassName = _T( "SomeNameYouInvented" );

	//if ( !RegisterClass( &wc ) )
	//{
	//	RmLog( LOG_ERROR, L"Unable to register window class" );
	//	return;
	//}

	//hWin = CreateWindow( _T( "SomeNameYouInvented" ), _T( "" ), 0, 0, 0, 0, 0, NULL, NULL, NULL, 0 );

	//if (!hWin)
	//{
	//	RmLog( LOG_ERROR, L"Unable to create hidden window" );
	//	return;
	//}

	//BOOL bRet;
	//MSG msg;
	//while ( ( ( bRet = GetMessage( &msg, hWin, 0, 0 ) ) != 0 ) && !exit )
	//{
	//	if ( bRet == -1 )
	//	{
	//		// handle the error and possibly exit
	//		RmLog( LOG_ERROR, L"Error in hidden window" );
	//		break;
	//	}
	//	else
	//	{
	//		TranslateMessage( &msg );
	//		DispatchMessage( &msg );
	//	}
	//}




	//PCWSTR lpWindowName,
	//	DWORD dwStyle,
	//	DWORD dwExStyle = 0,
	//	int x = CW_USEDEFAULT,
	//	int y = CW_USEDEFAULT,
	//	int nWidth = CW_USEDEFAULT,
	//	int nHeight = CW_USEDEFAULT,
	//	HWND hWndParent = nullptr,
	//	HMENU hMenu = nullptr

	//	lpClassName, 
	//	lpWindowName, 
	//	dwStyle, 
	//	x, 
	//	y, 
	//	nWidth, 
	//	nHeight, 
	//	hWndParent, 
	//	hMenu, 
	//	hInstance, 
	//	lpParam)\

	//CreateWindow( _T( "SomeNameYouInvented" ), _T( "" ), 0, 0, 0, 0, 0, NULL, NULL, NULL, 0 );

	hidden_window = new ( std::nothrow )Window( this->measures );

	if ( !hidden_window->Create( _T( "Hidden" ), 0, 0, 0, 0, 0, NULL, NULL, NULL) )
	{
		return;
	}

	BOOL bRet;
	MSG msg;
	while ( ( ( bRet = GetMessage( &msg, hidden_window->WindowHandle(), 0, 0 ) ) != 0 ) && !exit )
	{
		if ( bRet == -1 )
		{
			// handle the error and possibly exit
			RmLog( LOG_ERROR, L"Error in hidden window" );
			break;
		}
		else
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
	}
}

void WindowsEvents::Notify( const bool value ) const
{
	std::list<Measure*>::const_iterator it;

	for ( it = measures.begin(); it != measures.end(); ++it )
	{
		( *it )->WindowsEventHandler( value );
	}
}

void WindowsEvents::ActivityMain( void )
{
	BOOL screensaver_running = FALSE;

	LASTINPUTINFO last_input;
	last_input.cbSize = sizeof(last_input);

	DWORD idle_time;
	bool screensaverOn = false;

	//CString report;

	// main loop to check if user has been idle long enough
	while ( !exit )
	{		
		if ( !SystemParametersInfo( SPI_GETSCREENSAVERRUNNING, 0, &screensaver_running, SPIF_SENDCHANGE )
		  || !GetLastInputInfo( &last_input )
			)
		{
			RmLog( LOG_ERROR, L"WinApi load error while retrieving 'SPI_GETSCREENSAVERRUNNING' and 'GetLastInputInfo' info" );

			Sleep( 500 );
			continue;
		}

		idle_time = GetTickCount() - last_input.dwTime;

		//report.Format(L"idle time '%d' screensaver_running '%d'", idle_time, screensaver_running );
		//RmLog( LOG_DEBUG, report );	

		// Change was not detected, notify all interested parties
		if ( idle_time > this->user_idle_time && TRUE == screensaver_running )
		{
			RmLog( LOG_DEBUG, L"Inactive" );
			if ( !screensaverOn )
			{
				// Screensaver is running
				screensaverOn = true;
				this->Notify( true );
			}
		}
		else if ( idle_time < this->user_idle_time  && screensaverOn )
		{
			RmLog( LOG_DEBUG, L"Active" );

			// Screensaver is not running
			screensaverOn = false;
			this->Notify( false );
		}

		Sleep( 500 );
	}
}
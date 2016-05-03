#include "WindowsEvents.h"

#include <atlstr.h>
#include "../../API/RainmeterAPI.h"
#include "Measure.h"

Window::Window( std::list<std::weak_ptr<Measure>> &measures )
	: measures( measures )
{
}

PCWSTR Window::ClassName() const
{
	return L"HiddenWindow";
}

void Window::Notify( const bool value ) const
{
	std::list<std::weak_ptr<Measure>>::const_iterator it;
	std::shared_ptr<Measure> measure;

	for ( it = this->measures.begin(); it != this->measures.end(); ++it )
	{
		measure = it->lock();
		if ( nullptr != measure )
		{
			measure->WindowsEventHandler( value );
		}
	}
}

LRESULT Window::HandleMessage( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	if ( uMsg == WM_POWERBROADCAST )
	{
		CString report;
		report.Format( L"WM_POWERBROADCAST wParam '%llu' lParam '%llu'", wParam, lParam );

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
	}

	return TRUE;//DefWindowProc( this->WindowHandle(), uMsg, wParam, lParam );
}

unsigned int WindowsEvents::user_idle_time = 2000000;

WindowsEvents::WindowsEvents()
{
	this->activityThread = std::thread( &WindowsEvents::ActivityMain, this );
	this->sleepThread = std::thread( &WindowsEvents::SleepMain, this );
}

WindowsEvents::~WindowsEvents()
{
	this->exit = true;

	if (nullptr != this->hidden_window)
	{
		// Closes hidden window GetMessage queue
		PostMessage( this->hidden_window->WindowHandle(), WM_CLOSE, 0, 0 );
	}

	if ( this->sleepThread.joinable() )
	{
		this->sleepThread.join();
	}

	if ( this->activityThread.joinable() )
	{
		this->activityThread.join();
	}
}

void WindowsEvents::RegisterMeasure( std::weak_ptr<Measure> measure )
{
	this->measures.push_back(measure);
}

void WindowsEvents::SetUserIdleTime( unsigned const int ms )
{
	WindowsEvents::user_idle_time = ms;
}

void WindowsEvents::SleepMain( void )
{
	this->hidden_window = std::make_shared<Window>( this->measures );

	if ( !this->hidden_window->Create( _T( "Hidden" ), 0, 0, 0, 0, 0, NULL, nullptr, nullptr ) )
	{
		return;
	}

	BOOL bRet;
	MSG msg;
	while ( ( ( bRet = GetMessage( &msg, this->hidden_window->WindowHandle(), 0, 0 ) ) != 0 ) && !exit )
	{
		if ( bRet == -1 )
		{
			// Handle the error and possibly exit
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
	std::list<std::weak_ptr<Measure>>::const_iterator it;
	std::shared_ptr<Measure> measure;

	for ( it = this->measures.begin(); it != this->measures.end(); ++it )
	{
		measure = it->lock();
		if ( nullptr != measure )
		{
			measure->WindowsEventHandler( value );
		}
	}
}

void WindowsEvents::ActivityMain( void ) const
{
	BOOL screensaver_running = FALSE;

	LASTINPUTINFO last_input;
	last_input.cbSize = sizeof(last_input);

	DWORD idle_time;
	bool screensaverOn = false;

	//CString report;

	// Main loop to check if user has been idle long enough
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
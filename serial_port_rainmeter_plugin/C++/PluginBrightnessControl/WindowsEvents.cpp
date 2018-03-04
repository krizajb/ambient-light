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
	CString report;
	report.Format( L"uMsg '%d' wParam '%llu' lParam '%llu'", uMsg, wParam, lParam );
	RmLog( LOG_DEBUG, report );

	// Handle sleep events
	if ( uMsg == WM_POWERBROADCAST )
	{
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
	// Handle shut down events
	else if ( WM_ENDSESSION == uMsg )
	{
		report.Format( L"WM_ENDSESSION wParam '%llu' lParam '%llu'", wParam, lParam );
		RmLog( LOG_DEBUG, report );

		if ( ENDSESSION_LOGOFF == lParam)
		{
			report.Format(L"User is logging off");
			RmLog( LOG_DEBUG, report );
		}

		this->Notify( true );
	}
	else if ( WM_QUERYENDSESSION == uMsg)
	{
		report.Format( L"WM_QUERYENDSESSION wParam '%llu' lParam '%llu'", wParam, lParam );
		RmLog( LOG_DEBUG, report );
		
	}

	return DefWindowProc( this->WindowHandle(), uMsg, wParam, lParam );
}

unsigned int WindowsEvents::user_idle_time = 60 * 20 * 1000;	// 20 minutes

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
		CString report;
		report.Format(L"%s", this->hidden_window->Error() );
		RmLog(LOG_ERROR, report);
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

void WindowsEvents::ActivityMain( void )
{
	BOOL screensaver_running = FALSE;
	BOOL screensaver_enabled = FALSE;

	LASTINPUTINFO last_input;
	last_input.cbSize = sizeof(last_input);

	DWORD idle_time;
	bool screensaverOn = false;

	if ( !SystemParametersInfo(SPI_GETSCREENSAVEACTIVE, 0, &screensaver_enabled, 0) )
	{
		RmLog(LOG_ERROR, L"WinApi load error while retrieving 'SPI_GETSCREENSAVEACTIVE'");
	}
	else
	{
		if ( !screensaver_enabled )
		{
			RmLog(LOG_NOTICE, L"Screensaver not enabled.'");
			this->exit = true;
		}
	}

	CString report;

	// For screensaver detection to work one has to:
	// - Enable screensaver (Run "control desk.cpl,,@screensaver")
	// - Disable display power off (set timer to "Never") or
	// - Set display power off timer higher then screensaver timer

	// Main loop to check if user has been idle long enough
	while ( !this->exit )
	{
		if (!SystemParametersInfo(SPI_GETSCREENSAVERRUNNING, 0, &screensaver_running, SPIF_SENDCHANGE)
			|| !GetLastInputInfo(&last_input)
			)
		{
			RmLog(LOG_ERROR, L"WinApi load error while retrieving 'SPI_GETSCREENSAVERRUNNING' and 'GetLastInputInfo' info");

			Sleep(500);
			continue;
		}

		idle_time = GetTickCount() - last_input.dwTime;

		//report.Format(L"idle time '%d' screensaver_running '%d'", idle_time, screensaver_running );
		//RmLog( LOG_DEBUG, report );	

		// Change was detected, notify all interested parties
		if (idle_time > this->user_idle_time && TRUE == screensaver_running)
		{
			if (!screensaverOn)
			{
				RmLog(LOG_DEBUG, L"Inactive");
				// Screensaver is running
				screensaverOn = true;
				this->Notify(true);
			}
		}
		else if (idle_time < this->user_idle_time  && screensaverOn)
		{
			RmLog(LOG_DEBUG, L"Active");

			// Screensaver is not running
			screensaverOn = false;
			this->Notify(false);
		}

		Sleep(500);
	}
}
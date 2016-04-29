#include "WindowsEvents.h"

#include <atlstr.h>
#include "../../API/RainmeterAPI.h"

#include <windows.h>
#include <iostream>


WindowsEvents::WindowsEvents()
{
	this->activityThread = std::thread( &WindowsEvents::ActivityMain, this );
}

void WindowsEvents::SetNotificationHandler( std::function<void( const bool )> fn )
{
	this->notify = fn;
}

void WindowsEvents::SetUserIdleTime( unsigned const int ms )
{
	this->user_idle_time = ms;
}

WindowsEvents::~WindowsEvents()
{
	this->exit = true;

	if ( this->activityThread.joinable() )
	{
		this->activityThread.join();
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

		//report.Format(L"Idle time '%d' allowed idle time '%d'", a, this->user_idle_time );
		//RmLog( LOG_DEBUG, report );	

		// Change was not detected, notify all interested parties
		if ( idle_time < this->user_idle_time && !screensaver_running)
		{
			//RmLog( LOG_DEBUG, L"Active" );
			if ( screensaverOn )
			{
				// Screensaver is not running
				screensaverOn = false;
				if ( nullptr != this->notify)
				{
					this->notify( false );
				}
			}
		}
		else
		{
			//RmLog( LOG_DEBUG, L"Inactive" );
			if ( !screensaverOn )
			{
				// Screensaver is running
				screensaverOn = true;
				if ( nullptr != this->notify )
				{
					this->notify( true );
				}
			}
		}

		Sleep( 500 );
	}
}
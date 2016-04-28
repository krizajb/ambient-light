#include "WindowsEvents.h"

#include <atlstr.h>
#include "../../API/RainmeterAPI.h"

#include <windows.h>
#include <iostream>

// do something after 10 minutes of user inactivity
static const unsigned int idle_milliseconds = 60 * 1 * 1000;
// wait at least an hour between two runs
static const unsigned int interval = 60 * 60 * 1000;

WindowsEvents::WindowsEvents()
{
	activityThread = std::thread( &WindowsEvents::ActivityMain, this );
}


WindowsEvents::~WindowsEvents()
{
	exit = true;

	if ( activityThread.joinable() )
		activityThread.join();
}

void WindowsEvents::ActivityMain( void )
{
	BOOL screensaver_running_change = FALSE;
	BOOL screensaver_running = FALSE;

	// main loop to check if user has been idle long enough
	while ( !exit )
	{
		if ( !SystemParametersInfo( SPI_GETSCREENSAVERRUNNING, 0, &screensaver_running_change, 0 ) )
		{
			RmLog( LOG_ERROR, L"WinApi load error while retrieving 'SPI_GETSCREENSAVERRUNNING' info" );

			Sleep( 500 );
			continue;
		}

		// Change was detected, notify all interested parties
		if ( screensaver_running_change != screensaver_running )
		{
			notify( screensaver_running_change );
			screensaver_running = screensaver_running_change;
		}
		else
		{
			// Screensaver is not running
			screensaver_running = screensaver_running_change;

			Sleep( 500 );
			continue;
		}
	}
}

#pragma once

#include <atomic>
#include <string>
#include <atlstr.h>

#include "Globals.h"
#include "Serial.h"
#include "WindowsEvents.h"

#include "../../API/RainmeterAPI.h"

struct Measure
{
	std::shared_ptr<Serial> serial;
	std::atomic<int> brightness_value = NOT_SET_INT;
	std::atomic<bool> isOn = false;

	// this is here just for reference counting
	std::shared_ptr<WindowsEvents> win_events;

	void WindowsEventHandler( const bool isInactive );
	void SerialEventHandler( char* const data);

};

inline void Measure::WindowsEventHandler( const bool isInactive )
{
	// Turn on led strip
	if ( !isInactive )
	{
		RmLog( LOG_DEBUG, L"BrightnessControl.dll: Triggering on event " );

		if ( nullptr != this->serial )
		{
			if ( !this->serial->IsConnected() )
			{
				this->serial->Reconnect( true );
			}
			this->serial->WriteData( &On, 1 );
			this->isOn = true;
		}
	}
	// Turn off led strip
	else
	{
		RmLog( LOG_DEBUG, L"BrightnessControl.dll: Triggering off event" );

		if ( nullptr != this->serial )
		{
			if ( !this->serial->IsConnected() )
			{
				this->serial->Reconnect( true );
			}
			this->serial->WriteData( &Off, 1 );
			this->isOn = false;
		}
	}
}

inline void Measure::SerialEventHandler( char* const data )
{
	if ( nullptr == data ) return;
	if ( nullptr == serial ) return;

	// Report string
	CString report;
	report.Format( L"BrightnessControl.dll: Data received: '%hs' from '%hs'", data, this->serial->Port().c_str() );

	RmLog( LOG_DEBUG, report );

	static char buffer[32];
	static int bufferPos = 0;
	const char* inbyte;
	Status status = UNDEFINED;
	int brightness = NOT_SET_INT;

	for ( inbyte = data; *inbyte != '\0'; inbyte++ )
	{
		if ( *inbyte == Off )
		{
			status = OFF;
		}
		else if ( *inbyte == On )
		{
			status = ON;
		}
		else if ( *inbyte == Comma )
		{
			bufferPos = 0;
			brightness = std::stoi( buffer );
		}
		else
		{
			buffer[bufferPos++] = *inbyte;
		}
	}

	//std::lock_guard<std::mutex> lock( g_i_mutex );
	if ( status != UNDEFINED )
	{
		this->isOn = StatusToBool[status];
	}
	if ( brightness != NOT_SET_INT )
	{
		this->brightness_value = brightness;
	}
}



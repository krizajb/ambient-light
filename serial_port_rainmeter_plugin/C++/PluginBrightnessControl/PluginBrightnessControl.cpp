/*
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <Windows.h>
#include <cstdio>
#include <string>
#include <mutex>
#include <atlstr.h>
#include <atomic>
#include <map>

#include "../../API/RainmeterAPI.h"
#include "Serial.h"
#include "WindowsEvents.h"

#define DELETE_AND_CLEAR(ptr) \
	if (nullptr != ptr)  { delete ptr; ptr = nullptr; } \

const int NOT_SET_INT = -1;

// Serial communication parameters
const char Init( 'I' );
const char Share( 'S' );
const char Comma( ',' );
const char On( 'n' );
const char Off( 'f' );

std::string Port( "COM7" );	

// Brightness values
const int MIN = 0;
const int MAX = 255;

static Serial* serial = nullptr;
static WindowsEvents* winEvents = nullptr;
static std::atomic<int> brightness_value = NOT_SET_INT;
static std::atomic<bool> isOn = false;

// Status of led strip
enum Status
{
	OFF = 0,
	ON,
	UNDEFINED
};

std::map<Status, bool> StatusToBool;

/* Sets brightness value if offset isn't set, offset changes currently set brightness value by the offset.
 * Brightness value must be in MIN, MAX range.
 */
void SetBrightness( int value, int offset = 0 )
{
	int brightness = value;

	// Report string
	CString report;

	if ( 0 != offset )
	{
		//std::lock_guard<std::mutex> lock( g_i_mutex );
		brightness = brightness_value + offset;
	}

	if ( brightness >= MIN && brightness <= MAX )
	{
		if ( brightness < 5 )
		{
			brightness = MIN;
		}

		if ( brightness > 250 )
		{
			brightness = MAX;
		}

		{
			//std::lock_guard<std::mutex> lock( g_i_mutex );
			brightness_value = brightness;
			isOn = true;
		}

		std::string str = std::to_string( brightness_value ) + Comma;
		//char* astr = new char( int( str.length() ) + 1 );
		//strcpy( astr, str.c_str() );

		if ( nullptr != serial && !serial->WriteData( str.c_str() ) )
		{
			report.Format( L"BrightnessControl.dll: Enable to write data to '%hs' port.", Port.c_str() );
			RmLog( LOG_ERROR, report );
		}
		else
		{
			report.Format( L"BrightnessControl.dll: Changing brightness value '%d'", brightness_value );
			RmLog( LOG_DEBUG, report );
		}
	}
	else
	{
		report.Format( L"BrightnessControl.dll: Brightness value '%d' out of range", brightness_value );
		RmLog( LOG_ERROR, report );
	}
}

// Handles windows event
void WindowsEvent( const bool isInactive )
{
	// Turn on led strip
	if ( !isInactive )
	{
		RmLog(LOG_DEBUG, L"BrightnessControl.dll: Screensaver off ");

		if ( nullptr != serial )
		{
			serial->WriteData( "n", 1 );
			isOn = true;
		}
	} 
	// Turn off led strip
	else
	{
		RmLog( LOG_DEBUG, L"BrightnessControl.dll: Screensaver on" );

		if ( nullptr != serial )
		{
			serial->WriteData( "f", 1 );
			isOn = false;
		}
	}
}

/* Handles buffer data received from COM port.
 */
void DataAvail( const char* data )
{
	if ( nullptr == data ) return;

	// Report string
	CString report;
	report.Format( L"BrightnessControl.dll: Data received: '%hs' from '%hs'", data, Port.c_str() );

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
		isOn = StatusToBool[status];
	}
	if ( brightness != NOT_SET_INT )
	{
		brightness_value = brightness;
	}
}

PLUGIN_EXPORT void Initialize( void** data, void* rm )
{
	Port = CW2A( RmReadString( rm, L"Port", L"COM8", false ) );

	unsigned long idle_time = _wtol(RmReadString( rm, L"UserIdle", L"120000", false )); // default 20min
	
	StatusToBool.insert( std::make_pair( OFF, false ) );
	StatusToBool.insert( std::make_pair( ON, true ) );

	winEvents = new ( std::nothrow ) WindowsEvents();
	winEvents->SetUserIdleTime( idle_time );
	winEvents->SetNotificationHandler( WindowsEvent );

	// Allow controller to properly setup - this is blocking
	serial = new ( std::nothrow ) Serial( const_cast<char *>( Port.c_str() ) );
	serial->dataAvail = DataAvail;

	// Send initialization to controller
	if ( !serial->WriteData( &Init ) )
	{
		CString report;
		report.Format( L"BrightnessControl.dll: Unable to send initialization data to '%hs'", Port.c_str() );
		RmLog( LOG_ERROR, report );
	}
}

PLUGIN_EXPORT void Finalize( void* data )
{
	DELETE_AND_CLEAR( serial )
	DELETE_AND_CLEAR( winEvents )
}

PLUGIN_EXPORT void Reload( void* data, void* rm, double* maxValue )
{
	*maxValue = 100;
}

PLUGIN_EXPORT double Update( void* data )
{
	try
	{
		// Auto reconnect, this should happen if monitor ie usb is turned off
		if ( nullptr != serial )
		{
			if ( !serial->IsConnected() )
			{
				RmLog( LOG_WARNING, L"BrightnessControl.dll: Serial connection closed! Reconnecting ..." );
				serial->Disconnect();
				serial->Connect( const_cast<char*>( Port.c_str() ), true );
			}
		}
	}
	catch ( ... )
	{
		RmLog( LOG_ERROR, L"BrightnessControl.dll: Error during auto-reconnect progress" );
	}


	double value = 0.0;
	{
		//std::lock_guard<std::mutex> lock( g_i_mutex );
		if ( isOn )
		{
			value =  ( brightness_value / double( MAX ) ) * 100.0;
		}
	}

	return value;
}

/*
PLUGIN_EXPORT LPCWSTR GetString(void* data)
{
	Measure* measure = (Measure*)data;
	static WCHAR buffer[128];

	int value = Update( nullptr );
	swprintf_s( buffer, L"%d", value );

	return buffer;
}
*/

PLUGIN_EXPORT void ExecuteBang( void* data, LPCWSTR args )
{
	std::wstring wholeBang = args;

	size_t pos = wholeBang.find( ' ' );
	if ( pos != -1 )
	{
		std::wstring bang = wholeBang.substr( 0, pos );
		wholeBang.erase( 0, pos + 1 );

		if ( _wcsicmp( bang.c_str(), L"ChangeBrightness" ) == 0 )
		{
			// Parse parameters
			int offset = 0;
			if ( 1 == swscanf_s( wholeBang.c_str(), L"%d", &offset ) && offset )
			{
				SetBrightness( 0, offset );
			}
			else
			{
				RmLog( LOG_WARNING, L"BrightnessControl.dll: Incorrect number of arguments for bang 'ChangeBrightness'" );
			}
		}
		else if ( _wcsicmp( bang.c_str(), L"SetBrightness" ) == 0 )
		{
			// Parse parameters
			int brightness = 0;
			if ( 1 == swscanf_s( wholeBang.c_str(), L"%d", &brightness ) )
			{
				// Skin returns procent, convert it into brightness value
				double procent = brightness / 100.0;
				brightness = int( MAX*( procent ) );
				SetBrightness( brightness );
			}
			else
			{
				RmLog( LOG_WARNING, L"BrightnessControl.dll: Incorrect number of arguments for bang 'SetBrightness'" );
			}
		}
	}
	else
	{
		RmLog( LOG_WARNING, L"BrightnessControl.dll: Unknown bang" );
	}
}

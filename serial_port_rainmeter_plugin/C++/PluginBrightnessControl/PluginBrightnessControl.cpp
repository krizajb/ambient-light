/*
  Copyright (C) 2014 Birunthan Mohanathas

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

#include "../../API/RainmeterAPI.h"
#include "Serial.h"

const int NOT_SET_INT = -1;
const char Init('I');
const char Share('S');
const char Comma(',');
const char On('n');
const char Off('f');

const double MIN = 0.0;
const double MAX = 255.0;

std::string Port("COM4");	// make it configurable

static Serial* serial = nullptr;
static int brightness_value = NOT_SET_INT;
static bool isOn = false;

std::mutex g_i_mutex;  // protects brightness_value and isOn

/* Sets brightness value if offset isn't set, offset changes currently set brightness value by the offset.
 * Brightness value must be in MIN, MAX range.
 */ 
void SetBrightness( int value, int offset = 0 )
{
	int brightness = value;

	if ( 0 != offset )
	{
		brightness = brightness_value + offset;
	}

	if ( brightness >= MIN && brightness <= MAX )
	{
		if ( brightness < 5 )
		{
			brightness = MIN;
		}

		if ( brightness > 250)
		{
			brightness = MAX;
		}

		{
			std::lock_guard<std::mutex> lock( g_i_mutex );
			brightness_value = brightness;
			isOn = true;
		}

		std::string str = std::to_string( brightness_value ) + ",";
		char* astr = new char( str.length() + 1 );
		strcpy( astr, str.c_str() );

		if ( !serial->WriteData( astr, strlen( astr ) ))
		{
			RmLog( LOG_ERROR, L"BrightnessControl.dll: Enable to write data to COM port" );
		}
		else
		{
			std::wstring wstr = std::to_wstring( brightness_value );
			LPCWSTR str_value = wstr.c_str();
			RmLog( LOG_DEBUG, L"BrightnessControl.dll: Changing brightness" );
			RmLog( LOG_DEBUG, str_value );
		}
	}
	else
	{
		RmLog( LOG_ERROR, L"BrightnessControl.dll: Brightness value out of range" );
	}
}

/* Handles buffer data received from COM port.
 */
void DataAvail(const char* buffer)
{
	if (NULL == buffer) return;

	RmLog( LOG_DEBUG, L"BrightnessControl.dll: Data received: " );
	wchar_t value[20];

	mbstowcs( value, buffer, strlen( buffer ) + 1 );
	LPWSTR ptr = value;

	if (value[0] == Off)
	{
		isOn = false;
	}
	else if (value[0] == On)
	{
		isOn = true;
	}
	else
	{
		std::lock_guard<std::mutex> lock( g_i_mutex );
		brightness_value = atoi( buffer );
		isOn = true;
	}

	RmLog( LOG_DEBUG, value );
}

PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
	serial = new Serial( const_cast<char*>(Port.c_str()) );
	serial->dataAvail = DataAvail;
	serial->WriteData(&Init, 1);
}

PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
{
	*maxValue = 100;
}

PLUGIN_EXPORT double Update(void* data)
{
	try
	{
		// Auto reconnect, this should happen if monitor ie usb is turned off
		if ( NULL != serial )
		{
			if ( !serial->IsConnected() )
			{
				RmLog( LOG_WARNING, L"BrightnessControl.dll: Serial connection closed! Reconnecting ..." );
				serial->Disconnect();
				serial->Connect( const_cast<char*>(Port.c_str()) );
			}
		}
	}
	catch ( ... )
	{
		RmLog( LOG_ERROR, L"BrightnessControl.dll: Error during auto-reconnect progress" );
	}

	
	auto value = 0;
	if ( isOn )
	{
		std::lock_guard<std::mutex> lock( g_i_mutex );
		value =  ( brightness_value / MAX ) * 100;
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

PLUGIN_EXPORT void Finalize(void* data)
{
	delete serial;
	serial = nullptr;
}

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
				SetBrightness(0, offset);
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
				double procent = brightness/100.0;
				brightness = MAX*( procent );
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

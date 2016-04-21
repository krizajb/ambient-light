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

std::string Port("COM4");

static Serial* serial = nullptr;
static int brightness_value = NOT_SET_INT;
static bool isOn = false;

std::mutex g_i_mutex;  // protects brightness_value and isOn

void SetBrightness( int value, int offset = 0 )
{
	auto brightness = value;

	if ( 0 != offset )
	{
		brightness = brightness_value + offset;
	}

	if ( brightness >= 0 && brightness <= 255)
	{
		if ( brightness < 5 )
		{
			brightness = 0;
		}

		if ( brightness > 250)
		{
			brightness = 255;
		}

		{
			std::lock_guard<std::mutex> lock( g_i_mutex );
			brightness_value = brightness;
			isOn = true;
		}

		std::string str = std::to_string( brightness_value ) + ",";
		char* astr = new char( str.length() + 1 );
		strcpy( astr, str.c_str() );

		serial->WriteData( astr, strlen( astr ) );

		std::wstring wstr = std::to_wstring( brightness_value );
		LPCWSTR str_value = wstr.c_str();
		RmLog( LOG_DEBUG, L"Changing brightness" );
		RmLog( LOG_DEBUG, str_value );
	}
	else
	{
		// out of range brightness
	}
}

void DataAvail(const char* buffer)
{
	if (NULL == buffer) return;

	RmLog( LOG_DEBUG, L"Data avail callback!" );
	wchar_t value[20];

	mbstowcs( value, buffer, strlen( buffer ) + 1 );
	LPWSTR ptr = value;

	if (value[0] == 'f')
	{
		isOn = false;
	}
	else if (value[0] == 'n')
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
	/*
	// Implement auto reconnect!
	if ( NULL != fSerial )
	{
		if ( !fSerial->IsConnected() )
		{
			//RmLog( LOG_DEBUG, L"Connection closed!" );
		}
	}
	*/
	int value = 0;
	if ( isOn )
	{
		std::lock_guard<std::mutex> lock( g_i_mutex );
		value =  ( brightness_value / 255.0 ) * 100;
	}

	//RmLog( LOG_DEBUG, L"Update" );
	//RmLog( LOG_DEBUG, std::to_wstring( value ).c_str() );

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

	RmLog( LOG_DEBUG, args );

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
				double procent = brightness/100.0;
				brightness = 255*( procent );
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

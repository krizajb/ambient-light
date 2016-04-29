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

#include "Measure.h"

WindowsEvents* win_events = nullptr;
int win_events_ref_count = 0;


/* Sets brightness value if offset isn't set, offset changes currently set brightness value by the offset.
 * Brightness value must be in MIN, MAX range.
 */
void SetBrightness( Measure* const measure, int value, int offset = 0 )
{
	int brightness = value;

	// Report string
	CString report;

	if ( 0 != offset )
	{
		brightness = measure->brightness_value + offset;
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
			measure->brightness_value = brightness;
			measure->isOn = true;
		}

		std::string str = std::to_string( measure->brightness_value ) + Comma;

		if ( nullptr != measure->serial && !measure->serial->WriteData( str.c_str() ) )
		{
			report.Format( L"BrightnessControl.dll: Enable to write data to '%hs' port.", measure->port.c_str() );
			RmLog( LOG_ERROR, report );
		}
		else
		{
			report.Format( L"BrightnessControl.dll: Changing brightness value '%d'", measure->brightness_value );
			RmLog( LOG_DEBUG, report );
		}
	}
	else
	{
		report.Format( L"BrightnessControl.dll: Brightness value '%d' out of range", brightness );
		RmLog( LOG_ERROR, report );
	}
}

PLUGIN_EXPORT void Initialize( void** data, void* rm )
{
	Measure* measure = new( std::nothrow )Measure;

	std::string port = CW2A( RmReadString( rm, L"Port", L"COM8", false ) );

	unsigned long idle_time = _wtol( RmReadString( rm, L"UserIdle", L"120000", false ) ); // default 20min
	WindowsEvents::SetUserIdleTime( idle_time );

	if ( nullptr == win_events )
	{
		win_events = new ( std::nothrow ) WindowsEvents();
	}
	win_events_ref_count++;
	win_events->RegisterMeasure( measure );

	// Allow controller to properly setup - this is 3sec blocking
	Serial* serial = new ( std::nothrow ) Serial( const_cast<char *>( port.c_str() ) );
	//serial->SetHandler( DataAvail );
	serial->SetMeasure( measure );

	// Send initialization to controller
	if ( !serial->WriteData( &Init, 1 ) )
	{
		CString report;
		report.Format( L"BrightnessControl.dll: Unable to send initialization data to '%hs'", port.c_str() );
		RmLog( LOG_ERROR, report );
	}

	measure->serial = serial;
	measure->port = port;
	//measure->win_events = win_events;

	*data = measure;
}

PLUGIN_EXPORT void Finalize( void* data )
{
	Measure* measure = (Measure*)data;

	// this is by design, to avoid throws in destructor
	measure->serial->Disconnect();
	DELETE_AND_CLEAR( measure->serial )

	win_events_ref_count--;
	if ( win_events_ref_count == 0 )
	{
		// start using smart pointers!
		DELETE_AND_CLEAR( win_events )
	}


	DELETE_AND_CLEAR( measure )
}

PLUGIN_EXPORT void Reload( void* data, void* rm, double* maxValue )
{
	*maxValue = 100;
}

PLUGIN_EXPORT double Update( void* data )
{
	Measure* measure = (Measure*)data;
	if ( nullptr == measure ) return 0;

	try
	{
		// Auto reconnect, this should happen if monitor ie usb is turned off
		if ( nullptr != measure->serial )
		{
			if ( !measure->serial->IsConnected() )
			{
				RmLog( LOG_WARNING, L"BrightnessControl.dll: Serial connection closed! Reconnecting ..." );
				measure->serial->Disconnect();
				measure->serial->Connect( const_cast<char*>( measure->port.c_str() ), true );
			}
		}
	}
	catch ( ... )
	{
		RmLog( LOG_ERROR, L"BrightnessControl.dll: Error during auto-reconnect progress" );
	}


	double value = 0.0;
	{
		if ( measure->isOn )
		{
			value =  ( measure->brightness_value / double( MAX ) ) * 100.0;
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
	Measure* measure = (Measure*)data;
	if ( nullptr == measure ) return;

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
				SetBrightness( measure, 0, offset );
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
				SetBrightness( measure, brightness );
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

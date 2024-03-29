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
#include <string>
#include <atlstr.h>
#include <stdlib.h>  

#include "../../API/RainmeterAPI.h"
#include "Measure.h"



static std::shared_ptr<WindowsEvents> win_events( nullptr );

// Added since rainmeter doesn't support smart pointer, never delete raw data* pointer
// Setting rainmeter is owner of Measure instance
std::list<std::shared_ptr<Measure>> measures;

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
			measure->SetLedStatus(false);
		}
		else
		{
			measure->SetLedStatus(true);
		}

		if ( brightness > 250 )
		{
			brightness = MAX;
		}

		{
			measure->brightness_value = brightness;
			
		}

		std::string str = std::to_string( measure->brightness_value ) + Comma;

		if ( nullptr == measure->serial )
		{
			report.Format( L"BrightnessControl.dll: Enable to write data '%hs' to '%hs' port.", str.c_str(), measure->serial->Port().c_str());
			RmLog( LOG_ERROR, report );
		}
		else
		{
			measure->serial->Send( str );

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
	std::shared_ptr<Measure> measure = std::make_shared<Measure>();
	// not sure if any od this is OK since in the end we return raw pointer
	//std::atomic<std::shared_ptr<Measure>> measure1 = std::make_shared<Measure>();
	//measure1.load()->init = false;
	measures.push_back( measure );

	std::string port = CW2A( RmReadString( rm, L"Port", L"COM8", false ) );

	unsigned long idle_time = _wtol( RmReadString( rm, L"UserIdle", L"120000", false ) ); // default 20min
	WindowsEvents::SetUserIdleTime( idle_time );

	if ( nullptr == win_events )
	{
		win_events = std::make_shared<WindowsEvents>();
	}
	win_events->RegisterMeasure( measure );

	// Allow controller to properly setup
	std::shared_ptr<Serial> serial = std::make_shared<Serial>( port.c_str(), true, measure->report );
	serial->SetHandler( measure );

	// Send initialization to controller
	std::string msg( &Init, 1 );
	serial->Send( msg );
	measure->init = true;
	measure->started = false;

	measure->serial = serial;
	measure->win_events = win_events;

	*data = measure.get();
}

PLUGIN_EXPORT void Finalize( void* data )
{
	Measure* measure = static_cast<Measure*>( data );

	// Finally find shared pointer of measure and dereference it
	std::list<std::shared_ptr<Measure>>::iterator it = measures.begin();
	while ( it != measures.end() )
	{
		if (it->get() == measure)
		{
			it = measures.erase(it);
		}
		if ( it == measures.end() || measures.size() == 0 )
		{
			break;
		}
		++it;
	}

	// No one is using win_events, destroy it
	if ( win_events.use_count() == 1 )
	{
		win_events.reset();
	}
}

PLUGIN_EXPORT void Reload( void* data, void* rm, double* maxValue )
{
	UNUSED(data);
	UNUSED(rm);

	*maxValue = 100;
}

PLUGIN_EXPORT double Update( void* data )
{
	Measure* measure = static_cast<Measure*>( data );
	if ( nullptr == measure ) return 0;

	try
	{
/*		// Wait for connection  
		if (measure->init)
		{
			Sleep(500);
			measure->init = false;
		}*/

		// Auto reconnect, this should happen if screen ie usb is turned off
		if ( nullptr != measure->serial )
		{
			if ( measure->serial->Status() != Serial::CONNECTED)
			{
				measure->SetDeviceStatus( false );
				if (measure->report)
				{
					// To avoid spam, notify only on start
					RmLog(LOG_WARNING, L"BrightnessControl.dll: Serial connection closed! Reconnecting ...");
					measure->report = false;
				}

			}
			else
			{
				measure->SetDeviceStatus( true );
				measure->report = true;
				measure->init = false;
			}

		}
	}
	catch ( ... )
	{
		RmLog( LOG_ERROR, L"BrightnessControl.dll: Error during auto-reconnect progress" );
	}


	double value = 0.0;
	{
		if ( !measure->IsDeviceOn() )
		{
			// This is still rounded to 0% which makes it possible to detect device offline
			// by skin so one can disable controls etc
			value = OFF;
		}
		else if ( measure->IsLedOn() )
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
	Measure* measure = static_cast<Measure*>( data );
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

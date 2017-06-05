
#include "Measure.h"
#include <string>

#ifdef RAINMETER
  #include <atlstr.h>
  #include "../../API/RainmeterAPI.h"
#endif

void Measure::WindowsEventHandler(const bool isInactive)
{
	// Turn on led strip
	if (!isInactive)
	{
		RmLog(LOG_DEBUG, L"BrightnessControl.dll: Triggering on event ");

		if (nullptr != this->serial)
		{
			if (!this->serial->IsConnected())
			{
				this->serial->Reconnect(true);
			}
			this->serial->WriteData(&On, 1);
			//this->status |= LED_ON;
			this->SetLedStatus(true);
		}
	}
	// Turn off led strip
	else
	{
		RmLog(LOG_DEBUG, L"BrightnessControl.dll: Triggering off event");

		if (nullptr != this->serial)
		{
			if (!this->serial->IsConnected())
			{
				this->serial->Reconnect(true);
			}
			this->serial->WriteData(&Off, 1);
			//this->status |= LED_OFF;
			this->SetLedStatus(false);
		}
	}
}

void Measure::SerialEventHandler(char* const data)
{
	if (nullptr == data) return;
	if (nullptr == serial) return;

	// Report string
	CString report;
	report.Format(L"BrightnessControl.dll: Data received: '%hs' from '%hs'", data, this->serial->Port().c_str());

	RmLog(LOG_DEBUG, report);

	static char buffer[32];
	static int bufferPos = 0;
	const char* inbyte;
	//Status status = UNDEFINED;
	int brightness = NOT_SET_INT;

	for (inbyte = data; *inbyte != '\0'; inbyte++)
	{
		if (*inbyte == Off)
		{
			//status = LED_OFF;
			this->SetLedStatus(false);
		}
		else if (*inbyte == On)
		{
			//status = LED_ON;
			this->SetLedStatus(true);
		}
		else if (*inbyte == Comma)
		{
			bufferPos = 0;
			brightness = std::stoi(buffer);
		}
		else
		{
			buffer[bufferPos++] = *inbyte;
		}
	}

	//std::lock_guard<std::mutex> lock( g_i_mutex );
/*	if (status != UNDEFINED)
	{
		this->isOn = StatusToBool[status];
	}*/
	if (brightness != NOT_SET_INT)
	{
		this->brightness_value = brightness;
	}
}

void Measure::SetLedStatus(bool status)
{
	this->status &= ~UNDEFINED;
	status ? this->status |= LED_ON : this->status &= ~LED_ON;
}

void Measure::SetDeviceStatus(bool status)
{
	this->status &= ~UNDEFINED;
	status ? this->status |= DEVICE_ON : this->status &= ~DEVICE_ON;
}

bool Measure::IsDeviceOn(void) const
{
	return this->status & DEVICE_ON;
}

bool Measure::IsLedOn(void) const
{
	return this->status & LED_ON;
}
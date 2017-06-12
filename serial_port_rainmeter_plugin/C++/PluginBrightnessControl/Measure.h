#pragma once

#include <atomic>

#include "Globals.h"
#include "Serial.h"
#include "WindowsEvents.h"


struct Measure : public ISerialHandler
{
private:
	// See #Status
	std::atomic<int> status = UNDEFINED;

public:
	//Measure(const Measure &) = default;
	// Serial handle
	std::shared_ptr<Serial> serial;
	// Brightness value of the led strip
	std::atomic<int> brightness_value = NOT_SET_INT;
	// Measure init flag
	std::atomic<bool> init = true;
	// Reporting flag
	std::atomic<bool> report = true;
	// Serial init flag
	std::atomic<bool> started = false;

	// This is here just for reference counting
	std::shared_ptr<WindowsEvents> win_events;

	// Handles windows inActive event (sleep, idle, screensaver)
	void WindowsEventHandler( const bool isInactive );
	// Handles #serial data
	void SerialEventHandler( char* const data);

	// Getters/Setters
	void SetLedStatus( bool status );
	void SetDeviceStatus( bool status );

	bool IsDeviceOn( void ) const;
	bool IsLedOn( void ) const;

	virtual void HandleData( char* const data ) override;
	virtual void HandleStatus( const char status ) override;

};


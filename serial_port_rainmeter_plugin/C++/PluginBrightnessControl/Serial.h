#ifndef SERIALCLASS_H_INCLUDED
#define SERIALCLASS_H_INCLUDED

#pragma once

#define ARDUINO_WAIT_TIME 2000

#include <windows.h>
#include <thread>
#include <mutex>
#include <atomic>


struct Measure;

class Serial
{
private:
	// Serial comm handler
	HANDLE hSerial = nullptr;

	// Get various information about the connection
	COMSTAT status;

	// Keep track of last error
	DWORD errors;

	// Thread for reading serial data
	std::thread readThread;

	// Exit point from readThread
	bool exit = false;

	// Port name used for connection establishment
	std::string port;

	// Measure used for notification
	std::weak_ptr<Measure> measure;

	// Report flag
	std::atomic<bool> &report;

	// Protects all Serial members (just to be 100% sure)
	//std::mutex mutex;

public:
	// Initialize Serial communication with the given COM port
	Serial( const char *portName, bool sleepOnConnect, std::atomic<bool> &report );

	// Initialize Serial communication
	Serial( std::atomic<bool> &report );

	// Close the connection
	~Serial();

	// Read data in a buffer, if nbChar is greater than the
	// maximum number of bytes available, it will return only the
	// bytes available. The function return -1 when nothing could
	// be read, the number of bytes actually read.
	int ReadData( char *buffer, unsigned int nbChar );

	// See ReadData(), this runs in separated thread and notifies #measure
	void ReadDataMain( void );

	// Writes data from a buffer through the Serial connection
	// return true on success.
	bool WriteData( const char *buffer, unsigned int nbChar );

	// Same as above but attempts to sends whole buffer
	bool WriteData( const char *buffer);

	// Check if we are actually connected
	bool IsConnected( void );

	// Close the connection, throwable
	void Disconnect(void) const;

	// Initialize Serial communication with the given portName
	void Connect( const char* portName, bool sleep = false );

	// Reconnects to set port
	void Reconnect( bool sleep = false );

	// Reconnects to given portName
	void Reconnect( const char* portName, bool sleep = false );

	// Setters/Getters
	void SetMeasure( std::weak_ptr<Measure> measure );
	void SetPort( const char* portName );

	std::string Port( void ) const;
	DWORD Error(void) const;
};

#endif // SERIALCLASS_H_INCLUDED
#ifndef SERIALCLASS_H_INCLUDED
#define SERIALCLASS_H_INCLUDED

#pragma once

#define ARDUINO_WAIT_TIME 2000

#include <windows.h>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>

interface ISerialHandler
{
public:
	virtual ~ISerialHandler() {}
	// Handles various data
	virtual void HandleData( char* const data ) = 0;
	// Handles status change
	virtual void HandleStatus( const char status ) = 0;
};

struct Measure;

typedef struct Message
{
	std::string buffer;
	const unsigned long nbChar = 0;

	Message( std::string buffer, unsigned long nbChar )
		: buffer( buffer )
		, nbChar( nbChar )
	{
	}

} Message;

class Serial
{
public:
	typedef enum
	{
		CONNECTING = 0,
		CONNECTED
	} Mode;

private:
	// Serial comm handler
	HANDLE hSerial = nullptr;

	// Get various information about the connection
	COMSTAT status;

	// Keep track of last error
	DWORD errors;

	// Serial 
	std::thread mainThread;

	// Exit point from readThread
	bool exit = false;

	// Port name used for connection establishment
	std::string port;

	// Serial communication handler
	std::weak_ptr<ISerialHandler> handler;

	// Report flag
	std::atomic<bool> &report;

	// Protects all needed Serial members (sendQueue, mode)
	std::mutex mutex;

	// Current Serial mode
	Mode mode;

	// Message buffer
	std::queue<std::shared_ptr<Message>> sendQueue;

public:
	// Initialize Serial communication with the given COM port
	Serial( const char *portName, bool sleepOnConnect, std::atomic<bool> &report );

	// Initialize Serial communication
	Serial( std::atomic<bool> &report );

	// Close the connection
	~Serial();

	// Buffers the msg, each iteration tires to write one message from the #sendQueue
	void Send( std::string buffer, unsigned long nbChar );
	void Send( std::string buffer );

	Mode Status( void );

	// Setters/Getters
	void SetHandler( std::weak_ptr<ISerialHandler> handler );
	void SetPort( const char* portName );

	std::string Port( void ) const;
	DWORD Error( void ) const;

private:
	// Read data in a buffer, if nbChar is greater than the
	// maximum number of bytes available, it will return only the
	// bytes available. The function return -1 when nothing could
	// be read, the number of bytes actually read.
	int ReadData( char *buffer, unsigned long nbChar );

	// See ReadData(), this runs in separated thread and notifies #measure
	void ReadData( void ) const;

	// Writes data from a buffer through the Serial connection
	// return true on success.
	bool WriteData( const char *buffer, unsigned long nbChar );

	// Same as above but attempts to sends whole buffer
	bool WriteData( const char *buffer );

	void Update( void );

	// Check if we are actually connected
	bool IsConnected( void );

	// Close the connection, throwable
	void Disconnect( void ) const;

	void SerialMain( void );

	// Initialize Serial communication with the given portName
	void Connect( const char* portName, bool sleep = false );

	// Reconnects to set port
	void Reconnect( bool sleep = false );

	// Reconnects to given portName
	void Reconnect( const char* portName, bool sleep = false );


};

#endif // SERIALCLASS_H_INCLUDED
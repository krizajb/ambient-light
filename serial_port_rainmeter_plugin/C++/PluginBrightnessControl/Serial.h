#ifndef SERIALCLASS_H_INCLUDED
#define SERIALCLASS_H_INCLUDED

#pragma once

#define ARDUINO_WAIT_TIME 2000

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <functional> 
#include <thread>



class Serial
{
private:
	//Serial comm handler
	HANDLE hSerial;

	//Connection status
	bool connected;

	//Get various information about the connection
	COMSTAT status;

	//Keep track of last error
	DWORD errors;

	//Thread for reading serial data
	std::thread readThread;

	//Exit point from thread
	bool exit;

public:
	//Initialize Serial communication with the given COM port
	Serial( char *portName );

	//Close the connection
	~Serial();

	//Read data in a buffer, if nbChar is greater than the
	//maximum number of bytes available, it will return only the
	//bytes available. The function return -1 when nothing could
	//be read, the number of bytes actually read.
	int ReadData( char *buffer, unsigned int nbChar );

	//See ReadData(), this runs in separated thread and calls dataAvail
	void ReadDataMain( void );

	//Writes data from a buffer through the Serial connection
	//return true on success.
	bool WriteData( const char *buffer, unsigned int nbChar );

	//Check if we are actually connected
	bool IsConnected( void );

	//Close the connection
	void Disconnect(void);

	//Initialize Serial communication with the given portName
	void Connect( char* portName );

	//Function pointer for data avail notification
	std::function<void( const char* )> dataAvail;
};

#endif // SERIALCLASS_H_INCLUDED
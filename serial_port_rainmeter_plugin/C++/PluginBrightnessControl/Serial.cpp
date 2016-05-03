#include <Intsafe.h>

#include "Serial.h"
#include "Measure.h"

#ifdef RAINMETER
  #include <atlstr.h>
  #include "../../API/RainmeterAPI.h"
#endif

Serial::Serial( const char *portName )
	: port (portName)
{
	this->Connect( portName, true );
	readThread = std::thread( &Serial::ReadDataMain, this );
}

Serial::Serial( void )
{
	readThread = std::thread( &Serial::ReadDataMain, this );
}

Serial::~Serial()
{
	this->exit = true;

	if ( this->readThread.joinable() )
	{
		this->readThread.join();
	}

	this->Disconnect();
	this->hSerial = INVALID_HANDLE_VALUE;
}

int Serial::ReadData( char *buffer, unsigned int nbChar )
{
	//Number of bytes we'll have read
	DWORD bytesRead;
	//Number of bytes we'll really ask to read
	unsigned int toRead;

	//std::lock_guard<std::mutex> lock( this->mutex );

	//Use the ClearCommError function to get status info on the Serial port
	ClearCommError( this->hSerial, &this->errors, &this->status );

	//Check if there is something to read
	if ( this->status.cbInQue > 0 )
	{
		//If there is we check if there is enough data to read the required number
		//of characters, if not we'll read only the available characters to prevent
		//locking of the application.
		if ( this->status.cbInQue > nbChar )
		{
			toRead = nbChar;
		}
		else
		{
			toRead = this->status.cbInQue;
		}

		//Try to read the require number of chars, and return the number of read bytes on success
		if ( ReadFile( this->hSerial, buffer, toRead, &bytesRead, nullptr ) )
		{
			return bytesRead;
		}

	}

	//If nothing has been read, or that an error was detected return 0
	return 0;
}

void Serial::ReadDataMain( void )
{
	char *buffer = new char[32];
	unsigned int nbChar = sizeof( buffer );

	//Number of bytes we'll have read
	DWORD bytesRead;
	//Number of bytes we'll really ask to read
	unsigned int toRead;

	memset( buffer, 0, sizeof( buffer ) );

#ifdef RAINMETER
	RmLog( LOG_DEBUG, L"Starting read data main ..." );
#endif
	printf( "Starting read data main ..." );

	while ( !exit )
	{
		//std::lock_guard<std::mutex> lock( this->mutex );

		//Use the ClearCommError function to get status info on the Serial port
		ClearCommError( this->hSerial, &this->errors, &this->status );

		//Check if there is something to read
		if ( this->status.cbInQue > 0 )
		{
			//If there is we check if there is enough data to read the required number
			//of characters, if not we'll read only the available characters to prevent
			//locking of the application.
			if ( this->status.cbInQue > nbChar )
			{
				toRead = nbChar;
			}
			else
			{
				toRead = this->status.cbInQue;
			}

			//Try to read the require number of chars, and return the number of read bytes on success
			if ( ReadFile( this->hSerial, buffer, toRead, &bytesRead, nullptr ) )
			{
				std::shared_ptr<Measure> locked_measure = measure.lock();
				if ( nullptr != locked_measure )
				{
					// Forward buffer to handler
					locked_measure->SerialEventHandler( buffer );
				}
			}
			memset( buffer, 0, sizeof( buffer ) );
		}
	}

	// Clear buffer
	delete[] buffer;
}


bool Serial::WriteData( const char *buffer, unsigned int nbChar )
{
	bool returnValue = true;
	//std::lock_guard<std::mutex> lock( this->mutex );

	if ( this->hSerial == nullptr )
	{
		returnValue = false;
	}
	else
	{
		DWORD bytesSend;
		// Try to write the buffer on the Serial port
		if ( !WriteFile( this->hSerial, buffer, nbChar, &bytesSend, nullptr ) )
		{
			// In case it don't work get comm error and return false
			ClearCommError( this->hSerial, &this->errors, &this->status );
			returnValue = false;
		}
	}

	return returnValue;
}

bool Serial::WriteData( const char *buffer )
{
	bool returnValue = true;
	//std::lock_guard<std::mutex> lock( this->mutex );

	if ( this->hSerial == nullptr )
	{
		returnValue = false;
	}
	else
	{
		CString report;
		report.Format(L"Sending '%hs' to '%hs'", buffer, this->port.c_str());
		RmLog(LOG_DEBUG, report );

		DWORD bytesSent;
		DWORD bytesToSend;
		SIZETToDWord( strlen( buffer ), &bytesToSend );

		// Try to write the buffer on the Serial port
		if ( !WriteFile( this->hSerial, buffer, bytesToSend, &bytesSent, nullptr ) )
		{
			// In case it don't work get comm error and return false
			ClearCommError( this->hSerial, &this->errors, &this->status );
			returnValue = false;
		}
	}

	return returnValue;
}

bool Serial::IsConnected( void )
{
	bool isConnected = true;

	//std::lock_guard<std::mutex> lock( this->mutex );

	// Use the ClearCommError function to get status info on the Serial port
	ClearCommError( this->hSerial, &this->errors, &this->status );

	DCB dcbSerialParams = { 0 };

	// Try to get the current state
	GetCommState( this->hSerial, &dcbSerialParams );

	// This is working, note that for proper 100% serial connection check
	// one has to have a request-response mechanism 
	if ( dcbSerialParams.XoffLim == 0 )
	{
		isConnected = false;
	}

	return isConnected;
}

void Serial::Reconnect( bool sleep )
{
	this->Disconnect();
	this->Connect(this->port.c_str(), sleep );
}

void Serial::Reconnect( const char* portName, bool sleep )
{
	this->Disconnect();
	this->Connect( portName, sleep );
}

void Serial::Disconnect( void ) const
{
	//std::lock_guard<std::mutex> lock(this->mutex);
	// Close the serial handler
	CloseHandle( this->hSerial );
}

void Serial::Connect( const char* portName, bool sleep )
{
	wchar_t port[20];
	mbstowcs( port, portName, strlen( portName ) + 1 );
	LPWSTR ptr = port;

	this->port = portName;

#ifdef RAINMETER
	// Report string
	CString report;
	report.Format( L"Connecting to '%hs' ...", portName );
	RmLog( LOG_DEBUG, report );
#endif
	printf( "Connecting to '%s' ...", portName );

	//std::lock_guard<std::mutex> lock( this->mutex );

	//Try to connect to the given port through CreateFile
	this->hSerial = CreateFile( ptr,
		GENERIC_READ | GENERIC_WRITE,
		0,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		nullptr );

	//Check if the connection was successful
	if ( this->hSerial == INVALID_HANDLE_VALUE )
	{
		//If not success full display an Error
		if ( GetLastError() == ERROR_FILE_NOT_FOUND )
		{
#ifdef RAINMETER
			report.FormatMessage( L"'%s' Handle was not attached. Reason not available.", portName );
			RmLog( LOG_ERROR, report );
#endif
			printf( "ERROR: '%s' Handle was not attached.Reason not available.", portName );
		}
		else
		{
#ifdef RAINMETER
			report.FormatMessage( L"Handle was not attached." );
			RmLog( LOG_ERROR, report );
#endif
			printf( "ERROR: '%s' Handle was not attached.", portName );
		}
	}
	else
	{
		COMMTIMEOUTS timeout ={ 0 };

		timeout.ReadIntervalTimeout = 50;
		timeout.ReadTotalTimeoutConstant = 50;
		timeout.ReadTotalTimeoutMultiplier = 10;
		timeout.WriteTotalTimeoutConstant = 50;
		timeout.WriteTotalTimeoutMultiplier = 10;


		//If connected we try to set the comm parameters
		DCB dcbSerialParams ={ 0 };

		//Try to get the current
		if ( !GetCommState( this->hSerial, &dcbSerialParams ) )
		{
			//If impossible, show an error
#ifdef RAINMETER
			report.FormatMessage( L"Failed to get '%hs' current serial parameters!", portName );
			RmLog( LOG_WARNING, report );
#endif
			printf( "Failed to get current '%s' serial parameters!", portName );
		}
		else
		{
			//Define serial connection parameters for the Arduino board
			dcbSerialParams.BaudRate=CBR_9600;
			dcbSerialParams.ByteSize=8;
			dcbSerialParams.StopBits=ONESTOPBIT;
			dcbSerialParams.Parity=NOPARITY;
			//Setting the DTR to Control_Enable ensures that the Arduino is properly
			//reset upon establishing a connection
			//dcbSerialParams.fDtrControl = DTR_CONTROL_ENABLE;
			dcbSerialParams.fDtrControl = DTR_CONTROL_DISABLE;	// Disabled tmp?

			//Set the parameters and check for their proper application
			if ( !SetCommState( this->hSerial, &dcbSerialParams ) )
			{
#ifdef RAINMETER
				report.FormatMessage( L"Could not set '%hs' serial parameters", portName );
				RmLog( LOG_NOTICE, report );
#endif
				printf( "Could not set '%s' serial parameters", portName );
			}
			else
			{
				SetCommTimeouts( this->hSerial, &timeout );
				//If everything went fine we're connected

#ifdef RAINMETER
				report.Format( L"Connected to '%hs' ...", portName );
				RmLog( LOG_DEBUG, report );
#endif
				printf( "Connected to '%hs' ...", portName );
				//Flush any remaining characters in the buffers 
				PurgeComm( this->hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR );
				if ( sleep )
				{
					//We wait 2s as the Arduino board will be reseting
					Sleep( ARDUINO_WAIT_TIME + 1000 );
				}
			}
		}
	}
}

void Serial::SetMeasure( std::weak_ptr<Measure> measure )
{
	this->measure = measure;
}

void Serial::SetPort( const char* portName )
{
	this->port = portName;
}

std::string Serial::Port( void )const
{
	return this->port;
}

DWORD Serial::Error( void ) const
{
	return this->errors;
}
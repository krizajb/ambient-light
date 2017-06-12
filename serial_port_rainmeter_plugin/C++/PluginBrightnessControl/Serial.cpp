#include <Intsafe.h>

#include "Serial.h"
#include "Measure.h"

#ifdef RAINMETER
  #include <atlstr.h>
  #include "../../API/RainmeterAPI.h"
#endif

Serial::Serial( const char *portName, bool sleepOnConnect, std::atomic<bool> &report )
	: port( portName )
	, report( report )
	, mode( CONNECTING )
{
	this->mainThread = std::thread( &Serial::SerialMain, this );
}

Serial::Serial( std::atomic<bool> &report )
	: report( report )
	, mode( CONNECTING )
{
	this->mainThread = std::thread( &Serial::SerialMain, this );
}

Serial::~Serial()
{
	this->exit = true;
	if ( this->mainThread.joinable() )
	{
		this->mainThread.join();
	}

	this->Disconnect();
	this->hSerial = INVALID_HANDLE_VALUE;
}

int Serial::ReadData( char *buffer, unsigned long nbChar )
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

void Serial::ReadData( void ) const
{
	char *buffer = new char[32];
	unsigned int nbChar = sizeof(buffer);

	//Number of bytes we'll have read
	DWORD bytesRead;

	memset(buffer, 0, sizeof(buffer));

	if (this->hSerial != INVALID_HANDLE_VALUE)
	{
		//Try to read the require number of chars, and return the number of read bytes on success
		if (ReadFile(this->hSerial, buffer, sizeof(buffer), &bytesRead, nullptr))
		{
			if (bytesRead != 0)
			{
				std::shared_ptr<ISerialHandler> locked_handler = handler.lock();
				if (nullptr != locked_handler)
				{
					// Forward buffer to handler
					locked_handler->HandleData(buffer);
				}
			}
		}
		memset(buffer, 0, sizeof(buffer));
	}

	// Clear buffer
	delete[] buffer;
}

void Serial::Send(std::string buffer, unsigned long nbChar)
{
	std::lock_guard<std::mutex> lock(this->mutex);

	std::shared_ptr<Message> message(new Message(buffer, nbChar));
	this->sendQueue.push(message);
}

void Serial::Send(std::string buffer)
{
	std::lock_guard<std::mutex> lock(this->mutex);

	DWORD bytesToSend;
	SIZETToDWord(strlen(buffer.c_str()), &bytesToSend);

	std::shared_ptr<Message> message(new Message(buffer, bytesToSend));
	this->sendQueue.push(message);
}


bool Serial::WriteData( const char *buffer, unsigned long nbChar )
{
	bool returnValue = true;

	if ( this->hSerial == nullptr )
	{
		returnValue = false;
	}
	else
	{
		if (this->hSerial != INVALID_HANDLE_VALUE)
		{
			if (this->report)
			{
				CString report_msg;
				report_msg.Format( L"Sending '%hs' to '%hs'", buffer, this->port.c_str() );
				RmLog(LOG_DEBUG, report_msg);
			}

			DWORD bytesSend;
			// Try to write the buffer on the Serial port
			if (!WriteFile(this->hSerial, buffer, nbChar, &bytesSend, nullptr))
			{
				// In case it don't work get comm error and return false
				ClearCommError(this->hSerial, &this->errors, &this->status);
				returnValue = false;
			}
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
	else if (this->hSerial == INVALID_HANDLE_VALUE)
	{
		returnValue = false;
	}
	else
	{
		if ( this->report )
		{
			CString report_msg;
			report_msg.Format( L"Sending '%hs' to '%hs'", buffer, this->port.c_str() );
			RmLog( LOG_DEBUG, report_msg );
		}

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

Serial::Mode Serial::Status(void)
{
	std::lock_guard<std::mutex> lock(this->mutex);
	return this->mode;
}

void Serial::Update( void )
{
	std::lock_guard<std::mutex> lock(this->mutex);
	if ( !this->IsConnected() )
	{
		this->mode = CONNECTING;
	}
	else
	{
		this->mode = CONNECTED;
	}
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

void Serial::SerialMain(void)
{
	//std::unique_lock<std::mutex> lock(this->mutex);

	while (!this->exit)
	{
		// Update status
		this->Update();

		// Reconnect if necessary
		if (CONNECTING == mode)
		{
			this->Reconnect(true);
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}
		else
		{
			// Handle Read/Write data
			if ( !this->sendQueue.empty() )
			{
				std::shared_ptr<Message> msg = this->sendQueue.front();
				if ( this->WriteData(msg->buffer.c_str(), msg->nbChar) )
				{
					this->sendQueue.pop();
				}
				else
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(500));
				}
			}
			this->ReadData();
		}

		//std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

void Serial::Connect( const char* portName, bool sleep )
{
	wchar_t port[20];
	mbstowcs( port, portName, strlen( portName ) + 1 );
	LPWSTR ptr = port;

	this->port = portName;

#ifdef RAINMETER
	// Report string
	CString report_msg;
	if ( this->report )
	{
		report_msg.Format( L"Connecting to '%hs' ...", portName );
		RmLog( LOG_DEBUG, report_msg );
	}
#endif
	printf( "Connecting to '%s' ...", portName );

	size_t t_id = std::hash<std::thread::id>()(std::this_thread::get_id());
	report_msg.Format(L"Connect Thread ID '%llu' ...", t_id);
	RmLog(LOG_NOTICE, report_msg);

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
			if ( this->report )
			{
				report_msg.FormatMessage( L"'%s' Handle was not attached. Reason not available.", portName );
				RmLog( LOG_ERROR, report_msg );
			}
#endif
			printf( "ERROR: '%s' Handle was not attached.Reason not available.", portName );
		}
		else
		{
#ifdef RAINMETER
			if ( this->report )
			{
				report_msg.FormatMessage( L"Handle was not attached." );
				RmLog( LOG_ERROR, report_msg );
			}
#endif
			printf( "ERROR: '%s' Handle was not attached.", portName );
		}
	}
	else
	{
		//If connected we try to set the comm parameters
		DCB dcbSerialParams = { 0 };

		//Try to get the current
		if ( !GetCommState( this->hSerial, &dcbSerialParams ) )
		{
			//If impossible, show an error
#ifdef RAINMETER
			report_msg.FormatMessage( L"Failed to get '%hs' current serial parameters!", portName );
			RmLog( LOG_WARNING, report_msg );
#endif
			printf( "Failed to get current '%s' serial parameters!", portName );
		}
		else
		{
			//Define serial connection parameters for the Arduino board
			dcbSerialParams.BaudRate = CBR_9600;
			dcbSerialParams.ByteSize = 8;
			dcbSerialParams.StopBits = ONESTOPBIT;
			dcbSerialParams.Parity = NOPARITY;
			//Setting the DTR to Control_Enable ensures that the Arduino is properly
			//reset upon establishing a connection
			//dcbSerialParams.fDtrControl = DTR_CONTROL_ENABLE;
			dcbSerialParams.fDtrControl = DTR_CONTROL_DISABLE;	// Disabled tmp?
			dcbSerialParams.fRtsControl = RTS_CONTROL_DISABLE;	// Disabled tmp?

			//Set the parameters and check for their proper application
			if ( !SetCommState( this->hSerial, &dcbSerialParams ) )
			{
#ifdef RAINMETER
				report_msg.FormatMessage( L"Could not set '%hs' serial parameters", portName );
				RmLog( LOG_NOTICE, report_msg );
#endif
				printf( "Could not set '%s' serial parameters", portName );
			}
			else
			{
				//Flush any remaining characters in the buffers 
				PurgeComm( this->hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR );

				this->mode = CONNECTED;
					
				COMMTIMEOUTS timeouts;
				timeouts.ReadIntervalTimeout = 1;
				timeouts.ReadTotalTimeoutMultiplier = 1;
				timeouts.ReadTotalTimeoutConstant = 1;
				timeouts.WriteTotalTimeoutMultiplier = 1;
				timeouts.WriteTotalTimeoutConstant = 1;

				//Set timeouts and check for their proper application
				if (!SetCommTimeouts(this->hSerial, &timeouts))
				{
#ifdef RAINMETER
					report_msg.FormatMessage(L"Could not set timeout '%hs' serial parameters", portName);
					RmLog(LOG_WARNING, report_msg);
#endif
				}
				if ( sleep )
				{
					//We wait 2s as the Arduino board will be reseting
					std::this_thread::sleep_for(std::chrono::milliseconds(ARDUINO_WAIT_TIME));
				}

				std::shared_ptr<ISerialHandler> locked_handler = handler.lock();
				locked_handler->HandleStatus( On );

#ifdef RAINMETER
				report_msg.Format(L"Connected to '%hs' ...", portName);
				RmLog(LOG_DEBUG, report_msg);
#endif
				printf("Connected to '%hs' ...", portName);
			}
		}
	}
}

void Serial::SetHandler( std::weak_ptr<ISerialHandler> handler )
{
	this->handler = handler;
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
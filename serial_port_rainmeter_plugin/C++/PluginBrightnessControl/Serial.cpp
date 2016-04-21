#include "Serial.h"
#include "../../API/RainmeterAPI.h"

Serial::Serial( char *portName )
	: exit(false)
{
	this->Connect(portName);
	readThread = std::thread(&Serial::ReadDataMain, this );
}

Serial::~Serial()
{
	this->Disconnect();
	exit = true;

	if (readThread.joinable())
		readThread.join();
}

int Serial::ReadData( char *buffer, unsigned int nbChar )
{
	//Number of bytes we'll have read
	DWORD bytesRead;
	//Number of bytes we'll really ask to read
	unsigned int toRead;

	//Use the ClearCommError function to get status info on the Serial port
	ClearCommError( this->hSerial, &this->errors, &this->status );

	//Check if there is something to read
	if ( this->status.cbInQue>0 )
	{
		//If there is we check if there is enough data to read the required number
		//of characters, if not we'll read only the available characters to prevent
		//locking of the application.
		if ( this->status.cbInQue>nbChar )
		{
			toRead = nbChar;
		}
		else
		{
			toRead = this->status.cbInQue;
		}

		//Try to read the require number of chars, and return the number of read bytes on success
		if ( ReadFile( this->hSerial, buffer, toRead, &bytesRead, NULL ) )
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
	unsigned int nbChar = sizeof(buffer);

	//Number of bytes we'll have read
	DWORD bytesRead;
	//Number of bytes we'll really ask to read
	unsigned int toRead;

	memset( buffer, 0, sizeof( buffer ) );

	while (!exit)
	{
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
			if ( ReadFile( this->hSerial, buffer, toRead, &bytesRead, NULL ) )
			{
				// Forward buffer to handler
				dataAvail( buffer );
			}
			memset(buffer, 0, sizeof(buffer));
		}
		//Sleep(10);
	}

	// Clear buffer
	delete[] buffer;
}


bool Serial::WriteData( const char *buffer, unsigned int nbChar )
{
	DWORD bytesSend;

	//Try to write the buffer on the Serial port
	if ( !WriteFile( this->hSerial, (void *)buffer, nbChar, &bytesSend, 0 ) )
	{
		//In case it don't work get comm error and return false
		ClearCommError( this->hSerial, &this->errors, &this->status );

		return false;
	}
	else
		return true;
}

bool Serial::IsConnected(void)
{
	//Simply return the connection status
	//return this->connected;

	//Use the ClearCommError function to get status info on the Serial port
	ClearCommError( this->hSerial, &this->errors, &this->status );

	DCB dcbSerialParams ={ 0 };

	//Try to get the current
	GetCommState( this->hSerial, &dcbSerialParams );

	auto isConnected = true;
	//RmLog(LOG_DEBUG, dcbSerialParams.XoffLim );

	if (dcbSerialParams.XoffLim == 0)
	{
		isConnected = false;
	}

	return isConnected;
}

void Serial::Disconnect( void )
{
	//Check if we are connected before trying to disconnect
	if ( this->connected )
	{
		//We're no longer connected
		this->connected = false;
		//Close the serial handler
		CloseHandle( this->hSerial );
	}
}

void Serial::Connect( char* portName )
{
	//We're not yet connected
	this->connected = false;

	wchar_t port[20];
	mbstowcs( port, portName, strlen( portName ) + 1 );
	LPWSTR ptr = port;

	//Try to connect to the given port through CreateFile
	this->hSerial = CreateFile( ptr,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL );

	//Check if the connection was successful
	if ( this->hSerial == INVALID_HANDLE_VALUE )
	{
		//If not success full display an Error
		if ( GetLastError() == ERROR_FILE_NOT_FOUND )
		{

			//Print Error if necessary
			printf( "ERROR: Handle was not attached. Reason: %s not available.\n", portName );

		}
		else
		{
			printf( "ERROR!!!" );
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
			printf( "failed to get current serial parameters!" );
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
				printf( "ALERT: Could not set Serial Port parameters" );
			}
			else
			{
				SetCommTimeouts( this->hSerial, &timeout );
				//If everything went fine we're connected
				this->connected = true;
				//Flush any remaining characters in the buffers 
				PurgeComm( this->hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR );
				//We wait 2s as the Arduino board will be reseting
				//Sleep( ARDUINO_WAIT_TIME );
			}
		}
	}
}

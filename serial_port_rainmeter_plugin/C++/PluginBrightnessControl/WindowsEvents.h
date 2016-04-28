#pragma once

#include <thread>

class WindowsEvents
{
public:
	WindowsEvents();
	~WindowsEvents();

	//Function pointer for notification
	std::function<void( const bool )> notify;

private:
	std::thread activityThread;
	void ActivityMain( void );

	bool exit = false;

	bool wasActive = true;


};


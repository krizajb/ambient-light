#pragma once

#include <thread>

class WindowsEvents
{
public:
	WindowsEvents();
	~WindowsEvents();

	void SetNotificationHandler( std::function<void( const bool )> fn );
	void SetUserIdleTime(unsigned const int ms);

private:
	// ActivityMain thread
	std::thread activityThread;

	// exit point of activityThread
	bool exit = false;

	// user idle time allowed
	unsigned int user_idle_time = 200000;	// 20 minutes

	// function pointer for notification
	std::function<void( const bool )> notify;

	// screensaver and user idle detection
	void ActivityMain( void ) const;
};

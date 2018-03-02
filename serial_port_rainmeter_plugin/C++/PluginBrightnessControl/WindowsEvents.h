#pragma once

#include <thread>
#include <list>

#include <windows.h>

#include "BaseWindow.h"

struct Measure;

// Window class with customized WindowProc() message handling
class Window : public BaseWindow<Window>
{
public:
	Window( std::list<std::weak_ptr<Measure>> &measures );

	// Returns unique class name
	virtual PCWSTR  ClassName( void ) const override;
	// Overloads WindowProc, catching windows sleep events
	virtual LRESULT HandleMessage( UINT uMsg, WPARAM wParam, LPARAM lParam ) override;

private:
	// List of registered event listeners
	std::list<std::weak_ptr<Measure>> &measures;

	// notifies all registered event listeners 
	void Notify( const bool value ) const;
};

// Detects windows events: 
// - idle, screensaver (on, off)
// - sleep (on, off)
// And notifies all interested parties set using SetMeasure().
// To adjust user idle time use SetUserIdleTime()
class WindowsEvents
{
public:
	WindowsEvents();
	~WindowsEvents();

	// Registers measure for windows event notifications
	void RegisterMeasure( std::weak_ptr<Measure> measure );
	// Sets allowed user idle time, default idle time 20min
	static void SetUserIdleTime(unsigned const int ms);

private:
	// ActivityMain thread, controlled by exit flag
	std::thread activityThread;

	// SleepMain thread, controlled by exit flag
	std::thread sleepThread;

	// Exit point of activityThread and sleepThread
	bool exit = false;

	// User idle time allowed in milliseconds 
	static unsigned int user_idle_time;	// 20 minutes

	// List of registered event listeners
	std::list<std::weak_ptr<Measure>> measures;

	// Hidden window catching sleep event
	std::shared_ptr<Window> hidden_window;


	// Notifies all registered event listeners 
	void Notify(const bool value) const;

	// Screensaver and user idle detection
	void ActivityMain( void );

	// Sleep detection
	void SleepMain( void );
};

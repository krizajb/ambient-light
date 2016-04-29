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
	Window( std::list<Measure*> &measures );

	virtual PCWSTR  ClassName( void ) const override;
	virtual LRESULT HandleMessage( UINT uMsg, WPARAM wParam, LPARAM lParam ) override;

private:
	// list of registered event listeners
	std::list<Measure*> &measures;

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

	// registers measure for windows event notifications
	void RegisterMeasure(Measure* measure);
	// sets allowed user idle time, default idle time 20min
	static void SetUserIdleTime(unsigned const int ms);

private:
	// ActivityMain thread, controlled by exit flag
	std::thread activityThread;

	// SleepMain thread, controlled by exit flag
	std::thread sleepThread;

	// exit point of activityThread
	bool exit = false;

	// user idle time allowed
	static unsigned int user_idle_time;	// 20 minutes

	// list of registered event listeners
	std::list<Measure*> measures;

	// hidden window catching sleep event
	Window* hidden_window;


	// notifies all registered event listeners 
	void Notify(const bool value) const;

	// screensaver and user idle detection
	void ActivityMain( void ) ;

	// sleep detection
	void SleepMain( void );
};

#pragma once

#define RAINMETER	// Used mainly for debugging/reporting purpose

#define DELETE_AND_CLEAR(ptr) \
	if ( nullptr != ptr)  { delete ptr; ptr = nullptr; } \

#define UNUSED(ptr) (void) ptr;

const int NOT_SET_INT = -1;

// Serial communication parameters
const char Init( 'I' );		// Intialize device
const char Share( 'S' );	// Shared message between devices
const char Comma( ',' );	// Message delimiter
const char On( 'n' );		// Turn device ON
const char Off( 'f' );		// Turn device OFF

// Brightness values
const int MIN = 0;
const int MAX = 255;
const double OFF = 0.0112;

// Status of the controller and led strip
enum Status
{
	UNDEFINED	= ( 1u << 0 ),	// Status undefined
	LED_ON		= ( 1u << 1 ),	// Led strip is turned on 
	DEVICE_ON	= ( 1u << 2 ),	// Device is turned on - connected 
};
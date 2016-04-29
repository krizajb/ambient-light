#pragma once

#include <map>

const int NOT_SET_INT = -1;

#define DELETE_AND_CLEAR(ptr) \
	if ( nullptr != ptr)  { delete ptr; ptr = nullptr; } \

// Serial communication parameters
const char Init( 'I' );
const char Share( 'S' );
const char Comma( ',' );
const char On( 'n' );
const char Off( 'f' );

// Brightness values
const int MIN = 0;
const int MAX = 255;

// Status of led strip
enum Status
{
	OFF = 0,
	ON,
	UNDEFINED
};

// Status to bool conversion
static std::map<Status, bool> StatusToBool = { {OFF, false}, {ON, true} };

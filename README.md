Ambient light for screen(s). Very rough description of the project bellow.

The project contains arduino code supporting following functionality:
- touch ON/OFF switch
- serial communcation via USB
- selecting LED brightness, 1 step from 0 to 255 via communication
- dimming effect turning ON/OFF LED (strip)

Rainmeter plugin:
- serial communication with arduino (sending brightness value and reciving current LED state)
- windows sleep/wakeup, screen saver and shutdown/hibernate event detection
- windows event handling (turning LED ON/OFF)

Rainmeter skin:
- displaying current LED state (ON/OFF)
- displaying brightness value of LED strip
- on scroll or on click brithness selection

Standalone demo/development serial communication client written in Qt.


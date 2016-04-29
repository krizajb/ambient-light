/*
 * ambient light (led strip) control
 * 
 * 25.4.2016 blaz.krizaj@gmail.com
 */

#include <EEPROM.h>

// input/output
#define TOUCH 2
#define STRIP 5
#define LED13 13

// eeprom
#define EPROM 0
#define EPROMS 1

// serialization
#define SIZE 32

// brightness
#define MAX 255
#define MIN 0

// brightness fields
int brightness;
bool isOn;

// serial port communication fields
int sBufferPos = 0; // position in read buffer
char sBuffer[SIZE];
char sByte;
int sBrightness;

// serial port fields
const char Init( 'I' );
const char Comma( ',' );
const char Share( 'S' );
const char On( 'n' );
const char Off( 'f' );

// reporting
const bool Debug(false);

// forward declared to support default arguments
void log( int value, bool newLine=true );
void log( String value, bool newLine=true );
void log( char value, bool newLine=true );

// the setup function runs once when you press reset or power the board
void setup()
{
// open serial port - also for debugging
  Serial.begin( 9600 );  
  while ( !Serial ) 
  {
    //; // wait for serial port to connect. Needed for native USB port only
  }
  log("Init");

// initialize digital input/output 
  pinMode( TOUCH, INPUT );
  pinMode( STRIP, OUTPUT );

  /* development, useful for com port communication
  pinMode(LED13, OUTPUT);
  digitalWrite(LED13, HIGH); 
  */

// initialize init variables
  brightness = EEPROM.read( EPROM );
  log("Brightness: "+String( brightness ) );
  
  isOn = EEPROM.read( EPROMS );
  log("Is on: "+String( isOn ));

// set initial brightness
  setBrightness( brightness ); 
}

// sets brightness and stores value to eprom
void setBrightness( int value ) 
{
  if ( isOn )
  {
    log( "Setting brightness value: "+String( value ) );
    analogWrite( STRIP, value ); 
    delay( 30 );
    EEPROM.write( EPROM, value );
    brightness = value;
  }
}

void turnOn()
{
  int cBrightness = 0;
  while ( cBrightness != brightness )
  {
     analogWrite( STRIP, cBrightness++ ); 
     delay(30);
  }
}

void turnOff()
{
  int cBrightness = brightness - 50;
  while ( cBrightness != 0 )
  {
     analogWrite( STRIP, cBrightness-- ); 
     delay(30);
  }
}

// enables/disables led strip and stores value to eprom
void setEnabled( bool value )
{
  isOn = value;
  EEPROM.write( EPROMS, value );
}

// returns current status
char isEnabled( void )
{
  char isEnabled = On;
  if ( !isOn )
  {
    isEnabled = Off;
  }

  return isEnabled;
}

// reports value with optional newLine feed
void log( int value, bool newLine ) 
{
  if ( Debug ) 
  {
    String report = String( value );
    if ( newLine ) { report += "\n";  }
    Serial.print( report );
  }
}

// reports value with optional newLine feed
void log( String value, bool newLine ) 
{
  if ( Debug ) 
  {
    String report = value;
    if ( newLine ) { report += "\n"; }
    Serial.print( report );
  }
}

// reports value with optional newLine feed
void log( char value, bool newLine ) 
{
  if ( Debug ) 
  {
    String report( value );
    if ( newLine ) { report += "\n"; }
    Serial.print( report );
  }
}

// the loop function runs over and over again forever
void loop() 
{ 
 // turn off the led
  if ( digitalRead( TOUCH ) == HIGH && isOn ) 
  {
    log( "Strip OFF" );
    setEnabled( false );

    analogWrite( STRIP, 0 ); 
  // notify all interested parties
    Serial.print( Off );
    Serial.flush();
    
    delay( 500 );
  } 
// turn on the led with previous setting
  else if ( digitalRead( TOUCH ) == HIGH && !isOn ) 
  {
    log( "Strip ON" );
    setEnabled( true );

    setBrightness( brightness );   

  // notify all interested parties
    Serial.print( On );
    Serial.flush();
    
    delay( 500 );
  }
}

// serial port data handler
void serialEvent()
{
// read the incoming byte
  sByte = Serial.read();
  log( "D"+String( sByte )+"D " );
   
// add to buffer
  sBuffer[sBufferPos++] = sByte;

// init serial communication
  if ( sByte == Init )
  {
    log( "Init com" );
  // notify current brightess
    String init = String( isEnabled() )+String( brightness )+Comma;
    Serial.print( init );
    Serial.flush();
    
    sBuffer[sBufferPos-1] = 0;
    sBufferPos = sBufferPos-1;
  }
// screensaver turned off
  else if ( sByte == On )
  {
  // intentional bypassing EPROM save
    turnOn();

    //Serial.print( On );

    sBuffer[sBufferPos-1] = 0;
    sBufferPos = sBufferPos-1;
  }
// screensaver turned on
  else if ( sByte == Off )
  {
  // intentional bypassing EPROM save
    turnOff();

    //Serial.print( Off );
    
    sBuffer[sBufferPos-1] = 0;
    sBufferPos = sBufferPos-1;
  }
// check for delimiter
  else if ( sByte == ',' )
  {
    sBuffer[sBufferPos-1] = 0;
    log( "B"+String( sBuffer )+"B ", false );

    sBrightness = atoi( sBuffer );
    if ( sBrightness >= MIN && sBrightness <= MAX )
    {
      isOn = true;
      setBrightness( sBrightness );
      log( "C"+String( sBrightness )+"C " );
    }     
    sBufferPos = 0;
  }
}


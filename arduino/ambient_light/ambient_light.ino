/*
  Brightness with one button example
 */

#include <EEPROM.h>

// input/output
#define TOUCH 2
#define STRIP 5
#define LED13 13

// eeprom
#define EPROM 0

// serialization
#define SIZE 100

// brightness fields
int brightness;
int brightnessBuffer;
bool isOn;

// serial port fields
char* sInit;
char* sCommand;
const char Init('I');
const char Comma(',');
const char Share('S');

/*
char sBuffer[SIZE + 1];
int sBufferPos = 0;
char sData[SIZE + 1];
*/

const bool Debug(false);

// forward declared to support default arguments
void log(int value, bool newLine=true);
void log(String value, bool newLine=true);
void log(char value, bool newLine=true);
void remove(char* input, char ch);
int lastIndexOf(const char * string, char target);


// the setup function runs once when you press reset or power the board
void setup() {
// open serial port - also for debugging
  Serial.begin(9600);  
  while (!Serial) {
    //; // wait for serial port to connect. Needed for native USB port only
  }
  log("Init");

  brightness = EEPROM.read(EPROM);
  log("Brightness: "+String(brightness));
  
// initialize digital pin 2 as an input, digital pin 5 as an output
  pinMode(TOUCH, INPUT);
  pinMode(STRIP, OUTPUT);

  pinMode(LED13, OUTPUT);
  digitalWrite(LED13, HIGH); 

// initialize init variables
  brightnessBuffer = brightness;
  isOn = true;

// set initial brightness
  analogWrite(STRIP, brightness); 

/*
// clear serialization data
  memset(&sBuffer, 0, sizeof(sBuffer));
  memset(&sData, 0, sizeof(sData));
*/
}

// finds last target character in string
int lastIndexOf(const char * string, char target)
{
   int ret = -1;
   int curIdx = 0;
   while(string[curIdx] != '\0')
   {
      if (string[curIdx] == target) ret = curIdx;
      curIdx++;
   }
   return ret;
}

// removes all ch from string
void remove(char* string, char ch)
{
  char* output = string;
  while (*string)
  {
    if (*string != ch)
    {
      *(output++) = *string;
    }
    ++string;
  }
  *output = 0;
}

// sets brightness and stores value to eprom
void setBrightness(int value) 
{
  //log("Setting brightness value: "+String(value));
  analogWrite(STRIP, value); 
  delay(30);
  EEPROM.write(EPROM, value);
  //brightnessBuffer = value;
}

void log(int value, bool newLine) 
{
  if (Debug) 
  {
    String report = String(value);
    if (newLine) { report += "\n";  }
    Serial.print(report);
  }
}

void log(String value, bool newLine) 
{
  if (Debug) 
  {
    String report = value;
    if (newLine) { report += "\n"; }
    Serial.print(report);
  }
}

void log(char value, bool newLine) 
{
  if (Debug) 
  {
    String report(value);
    if (newLine) { report += "\n"; }
    Serial.print(report);
  }
}

int sBufferPos = 0; // position in read buffer
char sBuffer[32];
char sByte;

// the loop function runs over and over again forever
void loop() 
{ 
 // turn off the led
  if (digitalRead(TOUCH) == HIGH && isOn) 
  {
    log("Strip OFF");
    brightnessBuffer = brightness;
    brightness = 0;
    analogWrite(STRIP, brightness); 
  // notify all interested parties
    Serial.print("f");
    Serial.flush();
    
    isOn = false;
    delay(500);
  } 
// turn on the led with previous setting
  else if (digitalRead(TOUCH) == HIGH && !isOn) 
  {
    log("Strip ON");
    brightness = brightnessBuffer;
    setBrightness(brightnessBuffer);   

  // notify all interested parties
    Serial.print("n");
    Serial.flush();
    
    isOn = true;
    delay(500);
  }
}

// serial port data handler
void serialEvent()
{
// read the incoming byte
  sByte = Serial.read();
  //Serial.print("D"+String(sByte)+"D ");
   
// add to buffer
  sBuffer[sBufferPos++] = sByte;

// init serial communication
  if( sByte == 'I')
  {
    log("Init Com");
  // notify current brightess
    Serial.print(brightness);
    Serial.flush();
    
    sBuffer[sBufferPos-1] = 0;
    sBufferPos = sBufferPos-1;

    //Serial.print("Init");
    //Serial.print(sBuffer);
  }
// check for delimiter
  else if ( sByte == ',' )
  {
    sBuffer[sBufferPos-1] = 0;
    log("B"+String(sBuffer)+"B ", false);
    //Serial.print("B"+String(sBuffer)+"B ");
    brightness = atoi(sBuffer);
    if ( brightness >= 0 && brightness <= 255 )
    {
      setBrightness(brightness);
      isOn = true;
      log("C"+String(brightness)+"C ");
      //Serial.print("C"+String(brightness)+"C ");
    }     
    sBufferPos = 0;
  }


    /*

  
// reset data
  memset(&sData, 0, sizeof(sData));
  // try using some other read method this one sucks :)
// read the data from serial port
  byte size = Serial.readBytes(sData, sizeof(sData));
  sData[size] = 0;
  Serial.print(String(size)+"| ");



  //Serial.print("|"+String(sData)+"| ");
  

// check if this is initial connection
  sInit = strchr(sData, 'I');
  if (NULL != sInit) {
    log("Init detected");
   
  // notify current brightess
    Serial.print(brightness);
    Serial.flush();
  // remove the initial mark
    remove(sData, 'I');
  }

// prepend buffer to current data
  if (strlen(sBuffer) != 0) 
  {
    log("Prepend buffer");
    char sTmp[SIZE+1];
    strcpy(sTmp, sData);
    Serial.print("T"+String(sTmp)+"T ");

  // reset data
    memset(&sData, 0, sizeof(sData));
  // copy from buffer
    strcpy(sData, sBuffer);
  // append old data
    strcat(sData, sTmp);
   
  // reset the buffer
    sBufferPos = 0;  
    memset(&sBuffer, 0, sizeof(sBuffer));
  }
  
  Serial.print("|"+String(sData)+"| ");

// buffer the data in case it doesn't end with comma
  if (sData[strlen(sData)-1] != ',')
  {
    sBufferPos = 0;
    memset(&sBuffer, 0, sizeof(sBuffer));
    
    
    log("Buffer the data");  
    int index = lastIndexOf(sData, ',');
    int len = int(strlen(sData));
    Serial.print("I "+String(index)+ " L "+String(len)+" ");
  // no need to copy last comma
    for (int i = index+1; i < len; i++)
    {
      Serial.print("H"+String(i)+" "+String(sData[i])+" H");
      sBuffer[sBufferPos++] = sData[i];
      
      //sData[i] = 0;
    }

    for (int i = index+1; i < len; i++)
    {     
      sData[i] = 0;
    }

  }
  
  Serial.print("D"+String(sData)+"D ");
  Serial.print("B"+String(sBuffer)+"B ");
     
// read first command
  sCommand = strtok(sData, ",");

  while (sCommand != 0)
  {
    //brightness = atoi(sCommand);
    //log(brightness);
    Serial.print("C"+String(sCommand)+"C ");

    setBrightness(atoi(sCommand));

  // find the next command in data string
    sCommand = strtok(0, ",");
  }
  */
}


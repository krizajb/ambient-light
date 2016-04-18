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
#define SIZE 20

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

char sBuffer[SIZE + 1];
int sBufferPos = 0;
char sData[SIZE + 1];

const bool Debug(false);

// forward declared to support default arguments
void log(int value, bool newLine=true);
void log(String value, bool newLine=true);
void log(char value, bool newLine=true);
void remove(char* input, char ch);


// the setup function runs once when you press reset or power the board
void setup() {
      Serial.end();
  if (!Serial)
  {
    log("Serial port closed");
    }

// open serial port - also for debugging
  Serial.begin(9600);  
  while (!Serial) {
    //; // wait for serial port to connect. Needed for native USB port only
    log("test");
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
  //analogWrite(STRIP, brightness); 

// clear serialization data
  memset(&sBuffer[0], 0, sizeof(sBuffer));
  memset(&sData[0], 0, sizeof(sData));

#if defined(HAVE_HWSERIAL0)
  //Serial.print("Defined");
#endif
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
  analogWrite(STRIP, value); 
  EEPROM.write(EPROM, value);
  brightnessBuffer = value;
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

// the loop function runs over and over again forever
void loop() 
{
 // turn off the led
  if (digitalRead(TOUCH) == HIGH && isOn) 
  {
    log("Strip OFF");
    brightness = 0;
    setBrightness(brightness);
    isOn = false;
    delay(500);
  } 
// turn on the led with previous setting
  else if (digitalRead(TOUCH) == HIGH && !isOn) 
  {
    log("Strip ON");
    setBrightness(brightnessBuffer);
    isOn = true;
    delay(500);
  }
  /*
  delay(1000);
  Serial.print(Serial.available());
  */
}

// serial port data handler
void serialEvent()
{
  //Serial.print("Data avail");
  while (Serial.available())
  {
  // reset data
    memset(&sData[0], 0, sizeof(sData));
    
  // read the data from serial port
    byte size = Serial.readBytes(sData, SIZE);
    sData[size] = 0;
    //Serial.print(size);
    //Serial.print(sData);
    //Serial.flush();

  // check if this is initial connection
    sInit = strchr(sData, 'I');
    if (NULL != sInit) {
      log("Found I");

      digitalWrite(LED13, LOW); 
      //delay(5000);
      digitalWrite(LED13, HIGH); 
      
    // notify current brightess
    /*
    // this isn't tested yet, but could be the reason for init issues over time
      if (!Serial) {
        log("Serial not opened, reopening");
        Serial.begin(9600); 
      }
      */
      Serial.print(brightness);
      Serial.flush();
    // remove the initial mark
      remove(sData, 'I');
    }
  
  // prepend buffer to current data
    if (strlen(sBuffer) != 0) 
    {
      char sTmp[SIZE];
      strcpy(sTmp, sData);

    // reset data
      memset(&sData[0], 0, sizeof(sData));
    // copy from buffer
      strcpy(sData, sBuffer);
    // append old data
      strcat(sData, sTmp);
     
    // reset the buffer
      sBufferPos = 0;  
      memset(&sBuffer[0], 0, sizeof(sBuffer));
    }

  // buffer the data in case it doesn't end with comma
    if (sData[strlen(sData)-1] != ',')
    {
      log("Buffer the data");  
      int index = lastIndexOf(sData, ',');
    // no need to copy last comma
      for(int i = index+1; i < strlen(sData); i++)
      {
        sBuffer[sBufferPos++] = sData[i];
        sData[i] = 0;
      }
    }
    else 
    {
    // reset the buffer
      sBufferPos = 0;  
      memset(&sBuffer[0], 0, sizeof(sBuffer));
    }
       
  // read first command
    sCommand = strtok(sData, ",");
 
    while (sCommand != 0)
    {
      brightness = atoi(sCommand);
      log(brightness);
      //Serial.print(brightness);

      setBrightness(brightness);
  
    // find the next command in data string
      sCommand = strtok(0, ",");
    }
  }
}

/***********************************************************
Joystick controlled Clock using OLED Display
by Chris Rouse July 2015

This sketch uses an Arduino Mega to provide more memory

//
OLED Analog Clock using U8GLIB Library

visit https://code.google.com/p/u8glib/ for full details of the U8GLIB library and
full instructions for use.

This version uses a smoother font (profont15) and allows the centre of the clock
to be positioned on the screen by altering the variables clockCentreX and clockCentreY

Using a IIC 128x64 OLED with SSD1306 chip and RTC DS1307 

Connections:

Active Buzzer Alarm
Vcc connect to Arduino 5 volts
Gnd connect to Arduino Gnd
Out connect to Arduino pin 4

If Out is HIGH the buzzer is OFF
If Out is LOW the Buzzer is ON

This sketch provides a pulsed output 1 second ON
and 1 seconds OFF suitable for an alarm
//
//
Wire RTC:
  VCC +5v
  GND GND
  SDA Analog pin 4 on Uno, Mega pin 20
  SCL Analog pin 5 on Uno, Mega pin 21
//
//
Wire OLED:
  VCC +5v
  GND GND
  SDA Analog pin 4 on Uno, Mega pin 20
  SCL Analog pin 5 on Uno, Mega pin 21
//
//

************************************************************/

// Add libraries
  #include "U8glib.h"
  #include <SPI.h>
  #include <Wire.h>
  #include "RTClib.h"
  // BMP180
  #include "I2Cdev.h"
  #include "BMP085.h"
  
  
// setup u8g object
  U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE);	// I2C 
//

// Setup RTC
  RTC_DS1307 RTC;
  char monthString[37]= {"JanFebMarAprMayJunJulAugSepOctNovDec"};
  int  monthIndex[122] ={0,3,6,9,12,15,18,21,24,27,30,33};
  String thisMonth = "";
  String thisTime = "";
  String thisDay="";
  String thisWeekday ="";
  int clockCentreX = 64; // used to fix the centre the analog clock
  int clockCentreY = 32; // used to fix the centre the analog clock

//
// setup BMP180
  BMP085 barometer;
  int temperature;
  float pressure;
  String thisPressure = "";
  String thisTemperature = "";
  int showData = 1; // used to show data screen 1 = now, 2 = -24hrs, 3 = -48hrs
  float localTemp = 0.00;
  float localTempF = 0.00; // temp in Farenheit
  float localPressure = 0.00;
  boolean showC = true;  // if false show temp in Farenheit
  boolean switchForecast = false; // if false then current forecast shown
  //
  // Data strings, used to store temperature and pressure for 36 hours
  float recordDataTemp[3][25];
  float recordDataPressure[3][25];  
  float tempYvalueP;    
  float tempYvalue;  
  int recordPointer = 0; // points to current entry in record
  boolean doOnce = false; // only let temperature be read once for logging
  int recordNumber = 0; // counts data entries in current 24 hours
//
// Variables and defines
//
  #define buzzerPin 4 // the pin the buzzer is connected to
  #define ledPin 13 // onboard led used for silent alarm
  #define joySwitch 2 // joystick switch
  #define joyPinX A1 // X pot output on joystick
  #define joyPinY A0 // Y pot output on joystick
  
  
// Alarm Clock::
//
// User can change the following 3 variables as required
//
  int alarmHour = 7;
  int alarmMinute = 30;
  int maxAlarmTime = 10; //maximum time alarm sound for if seconds up to 59  
//
// flag to show an alarm has been set
  volatile boolean alarmSet = false;
  volatile boolean alarmSetMinutes = true;
  boolean setMinutes = true; // flag to show whether to set minutes or hours
  String alarmThisTime = "";
  static unsigned long last_interrupt_time2 = 0;
//
// joystick::
//
  int joyX = 512; // midpoint
  int joyY = 512; // midpoint
  boolean xValid = true;
  boolean yValid = false;
  volatile boolean buttonFlag = false;
//
// timer::
//
  int timerSecs = 0;
  int timerMins = 0; 
  unsigned long previousMillis = 0; 
  const long interval = 1000;  
  const char* newTimeTimer = "00:00";
  String thisTimeTimer = "";
//
// Buzzer::
//
  unsigned long buzzerPreviousMillis = 0; // will store last time LED was updated
  const long buzzerInterval = 1000;
  int ledState = LOW;
  int buzzerState = HIGH;
//
// Set start screen and maximum number of screens::
//
  volatile int displayScreen = 0; // screen number to start at
  int screenMax = 11; // highest screen number
//
// general delay::
//
  static unsigned long lastMicros = 0; // used in BMP180 read routine
  static unsigned long last_interrupt_time4 = 0;
  unsigned long interrupt_time4 = 0;
//
// Weather Forecast
  String thisForecast ="";
  String lastForecast="";
//
// analog and digital clock displays::
//
  const char* greetingTime = "";
  const char* greetingTemp = "";
  volatile boolean timeAlarmSet = false;
//
// calendar::
//
  int startDay = 0; // Sunday's value is 0, Saturday is 6
  String week1 ="";
  String week2 ="";
  String week3 ="";
  String week4 ="";
  String week5 ="";
  int newWeekStart = 0; // used to show start of next week of the month
  char monthString2[37]= {"JanFebMarAprMayJunJulAugSepOctNovDec"};
  int  monthIndex2[122] ={0,3,6,9,12,15,18,21,24,27,30,33};
  char monthName2[3]="";
//
// moon phase
static unsigned char full_moon_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x07, 0x00, 0x00, 0xff, 0x3f, 0x00,
   0x80, 0xff, 0x7f, 0x00, 0xe0, 0xff, 0xff, 0x01, 0xf0, 0xff, 0xff, 0x03,
   0xf0, 0xff, 0xff, 0x03, 0xf8, 0xff, 0xff, 0x07, 0xfc, 0xff, 0xff, 0x0f,
   0xfc, 0xff, 0xff, 0x0f, 0xfc, 0xff, 0xff, 0x0f, 0xfe, 0xff, 0xff, 0x1f,
   0xfe, 0xff, 0xff, 0x1f, 0xfe, 0xff, 0xff, 0x1f, 0xfe, 0xff, 0xff, 0x1f,
   0xfe, 0xff, 0xff, 0x1f, 0xfe, 0xff, 0xff, 0x1f, 0xfe, 0xff, 0xff, 0x1f,
   0xfe, 0xff, 0xff, 0x1f, 0xfc, 0xff, 0xff, 0x0f, 0xfc, 0xff, 0xff, 0x0f,
   0xfc, 0xff, 0xff, 0x0f, 0xf8, 0xff, 0xff, 0x07, 0xf0, 0xff, 0xff, 0x03,
   0xf0, 0xff, 0xff, 0x03, 0xe0, 0xff, 0xff, 0x01, 0x80, 0xff, 0x7f, 0x00,
   0x00, 0xff, 0x3f, 0x00, 0x00, 0xf8, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00 };

static unsigned char waning_gibbous_bits[] = {
   0x00, 0xf8, 0x0f, 0x00, 0x00, 0xff, 0x3f, 0x00, 0x80, 0xff, 0xff, 0x00,
   0xe0, 0xff, 0xff, 0x00, 0xf0, 0xff, 0x7f, 0x00, 0xf8, 0xff, 0x7f, 0x00,
   0xf8, 0xff, 0x3f, 0x00, 0xfc, 0xff, 0x1f, 0x00, 0xfe, 0xff, 0x1f, 0x00,
   0xfe, 0xff, 0x0f, 0x00, 0xfe, 0xff, 0x0f, 0x00, 0xff, 0xff, 0x0f, 0x00,
   0xff, 0xff, 0x0f, 0x00, 0xff, 0xff, 0x07, 0x00, 0xff, 0xff, 0x07, 0x00,
   0xff, 0xff, 0x07, 0x00, 0xff, 0xff, 0x07, 0x00, 0xff, 0xff, 0x07, 0x00,
   0xff, 0xff, 0x07, 0x00, 0xfe, 0xff, 0x0f, 0x00, 0xfe, 0xff, 0x0f, 0x00,
   0xfe, 0xff, 0x0f, 0x00, 0xfc, 0xff, 0x1f, 0x00, 0xf8, 0xff, 0x1f, 0x00,
   0xf8, 0xff, 0x3f, 0x00, 0xf0, 0xff, 0x3f, 0x00, 0xe0, 0xff, 0xff, 0x00,
   0x80, 0xff, 0xff, 0x00, 0x00, 0xff, 0x3f, 0x00, 0x00, 0xf8, 0x1f, 0x00 };

static unsigned char last_quarter_bits[] = {
   0x00, 0xf8, 0x0f, 0x00, 0x00, 0xff, 0x07, 0x00, 0x80, 0xff, 0x01, 0x00,
   0xe0, 0xff, 0x01, 0x00, 0xf0, 0xff, 0x00, 0x00, 0xf8, 0x7f, 0x00, 0x00,
   0xf8, 0x3f, 0x00, 0x00, 0xfc, 0x3f, 0x00, 0x00, 0xfe, 0x1f, 0x00, 0x00,
   0xfe, 0x1f, 0x00, 0x00, 0xfe, 0x0f, 0x00, 0x00, 0xff, 0x0f, 0x00, 0x00,
   0xff, 0x0f, 0x00, 0x00, 0xff, 0x0f, 0x00, 0x00, 0xff, 0x0f, 0x00, 0x00,
   0xff, 0x0f, 0x00, 0x00, 0xff, 0x0f, 0x00, 0x00, 0xff, 0x0f, 0x00, 0x00,
   0xff, 0x1f, 0x00, 0x00, 0xfe, 0x1f, 0x00, 0x00, 0xfe, 0x1f, 0x00, 0x00,
   0xfe, 0x1f, 0x00, 0x00, 0xfc, 0x3f, 0x00, 0x00, 0xf8, 0x3f, 0x00, 0x00,
   0xf8, 0x7f, 0x00, 0x00, 0xf0, 0xff, 0x00, 0x00, 0xe0, 0xff, 0x01, 0x00,
   0x80, 0xff, 0x01, 0x00, 0x00, 0xff, 0x07, 0x00, 0x00, 0xf8, 0x0f, 0x00 };

static unsigned char crescent_old_bits[] = {
   0x00, 0xf8, 0x07, 0x00, 0x00, 0xff, 0x00, 0x00, 0x80, 0x1f, 0x00, 0x00,
   0xe0, 0x0f, 0x00, 0x00, 0xf0, 0x03, 0x00, 0x00, 0xf8, 0x01, 0x00, 0x00,
   0xf8, 0x01, 0x00, 0x00, 0xfc, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00,
   0x7e, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x00,
   0x3f, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x00,
   0x3f, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x00,
   0x3f, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00,
   0x7e, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x00, 0x00, 0xf8, 0x01, 0x00, 0x00,
   0xf8, 0x01, 0x00, 0x00, 0xf0, 0x03, 0x00, 0x00, 0xe0, 0x0f, 0x00, 0x00,
   0x80, 0x1f, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xf8, 0x07, 0x00 };

static unsigned char new_moon_bits[] = {
   0x00, 0xf8, 0x07, 0x00, 0x00, 0x07, 0x38, 0x00, 0x80, 0x00, 0x40, 0x00,
   0x60, 0x00, 0x80, 0x01, 0x10, 0x00, 0x00, 0x02, 0x08, 0x00, 0x00, 0x04,
   0x08, 0x00, 0x00, 0x04, 0x04, 0x00, 0x00, 0x08, 0x02, 0x00, 0x00, 0x10,
   0x02, 0x00, 0x00, 0x10, 0x02, 0x00, 0x00, 0x10, 0x01, 0x00, 0x00, 0x20,
   0x01, 0x00, 0x00, 0x20, 0x01, 0x00, 0x00, 0x20, 0x01, 0x00, 0x00, 0x20,
   0x01, 0x00, 0x00, 0x20, 0x01, 0x00, 0x00, 0x20, 0x01, 0x00, 0x00, 0x20,
   0x01, 0x00, 0x00, 0x20, 0x02, 0x00, 0x00, 0x10, 0x02, 0x00, 0x00, 0x10,
   0x02, 0x00, 0x00, 0x10, 0x04, 0x00, 0x00, 0x08, 0x08, 0x00, 0x00, 0x04,
   0x08, 0x00, 0x00, 0x04, 0x10, 0x00, 0x00, 0x02, 0x60, 0x00, 0x80, 0x01,
   0x80, 0x00, 0x40, 0x00, 0x00, 0x07, 0x38, 0x00, 0x00, 0xf8, 0x07, 0x00 };

static unsigned char crescent_new_bits[] = {
   0x00, 0xf8, 0x07, 0x00, 0x00, 0xc0, 0x3f, 0x00, 0x00, 0x00, 0x7e, 0x00,
   0x00, 0x00, 0xfc, 0x01, 0x00, 0x00, 0xf0, 0x03, 0x00, 0x00, 0xe0, 0x07,
   0x00, 0x00, 0xe0, 0x07, 0x00, 0x00, 0xc0, 0x0f, 0x00, 0x00, 0x80, 0x1f,
   0x00, 0x00, 0x80, 0x1f, 0x00, 0x00, 0x80, 0x1f, 0x00, 0x00, 0x00, 0x3f,
   0x00, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x3f,
   0x00, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x3f,
   0x00, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x80, 0x1f, 0x00, 0x00, 0x80, 0x1f,
   0x00, 0x00, 0x80, 0x1f, 0x00, 0x00, 0xc0, 0x0f, 0x00, 0x00, 0xe0, 0x07,
   0x00, 0x00, 0xe0, 0x07, 0x00, 0x00, 0xf0, 0x03, 0x00, 0x00, 0xfc, 0x01,
   0x00, 0x00, 0x7e, 0x00, 0x00, 0xc0, 0x3f, 0x00, 0x00, 0xf8, 0x07, 0x00 };

static unsigned char first_quarter_bits[] = {
   0x00, 0xfc, 0x07, 0x00, 0x00, 0xf8, 0x3f, 0x00, 0x00, 0xe0, 0x7f, 0x00,
   0x00, 0xe0, 0xff, 0x01, 0x00, 0xc0, 0xff, 0x03, 0x00, 0x80, 0xff, 0x07,
   0x00, 0x00, 0xff, 0x07, 0x00, 0x00, 0xff, 0x0f, 0x00, 0x00, 0xfe, 0x1f,
   0x00, 0x00, 0xfe, 0x1f, 0x00, 0x00, 0xfc, 0x1f, 0x00, 0x00, 0xfc, 0x3f,
   0x00, 0x00, 0xfc, 0x3f, 0x00, 0x00, 0xfc, 0x3f, 0x00, 0x00, 0xfc, 0x3f,
   0x00, 0x00, 0xfc, 0x3f, 0x00, 0x00, 0xfc, 0x3f, 0x00, 0x00, 0xfc, 0x3f,
   0x00, 0x00, 0xfe, 0x3f, 0x00, 0x00, 0xfe, 0x1f, 0x00, 0x00, 0xfe, 0x1f,
   0x00, 0x00, 0xfe, 0x1f, 0x00, 0x00, 0xff, 0x0f, 0x00, 0x00, 0xff, 0x07,
   0x00, 0x80, 0xff, 0x07, 0x00, 0xc0, 0xff, 0x03, 0x00, 0xe0, 0xff, 0x01,
   0x00, 0xe0, 0x7f, 0x00, 0x00, 0xf8, 0x3f, 0x00, 0x00, 0xfc, 0x07, 0x00 };

static unsigned char waxing_gibbous_bits[] = {
   0x00, 0xfc, 0x07, 0x00, 0x00, 0xff, 0x3f, 0x00, 0xc0, 0xff, 0x7f, 0x00,
   0xc0, 0xff, 0xff, 0x01, 0x80, 0xff, 0xff, 0x03, 0x80, 0xff, 0xff, 0x07,
   0x00, 0xff, 0xff, 0x07, 0x00, 0xfe, 0xff, 0x0f, 0x00, 0xfe, 0xff, 0x1f,
   0x00, 0xfc, 0xff, 0x1f, 0x00, 0xfc, 0xff, 0x1f, 0x00, 0xfc, 0xff, 0x3f,
   0x00, 0xfc, 0xff, 0x3f, 0x00, 0xf8, 0xff, 0x3f, 0x00, 0xf8, 0xff, 0x3f,
   0x00, 0xf8, 0xff, 0x3f, 0x00, 0xf8, 0xff, 0x3f, 0x00, 0xf8, 0xff, 0x3f,
   0x00, 0xf8, 0xff, 0x3f, 0x00, 0xfc, 0xff, 0x1f, 0x00, 0xfc, 0xff, 0x1f,
   0x00, 0xfc, 0xff, 0x1f, 0x00, 0xfe, 0xff, 0x0f, 0x00, 0xfe, 0xff, 0x07,
   0x00, 0xff, 0xff, 0x07, 0x00, 0xff, 0xff, 0x03, 0xc0, 0xff, 0xff, 0x01,
   0xc0, 0xff, 0x7f, 0x00, 0x00, 0xff, 0x3f, 0x00, 0x00, 0xfe, 0x07, 0x00 };
//
String nfm = ""; // days to next full moon
//
//
/********************************************************/

void setup(void) {
  Serial.begin(9600);
  Wire.begin();
  RTC.begin();
  if (! RTC.isrunning()) {
    Serial.println("RTC is NOT running!");
  }
  // following line sets the RTC to the date & time this sketch was compiled  
  // un REM the line below to set clock, then re REM it
  // and upload this sketch again
  //RTC.adjust(DateTime(__DATE__, __TIME__));
  //
  //BP180
  // initialize device
  Serial.println("Initializing I2C devices...");
  barometer.initialize();
  // verify connection
  Serial.println("Testing device connections...");
  Serial.println(barometer.testConnection() ? "BMP085 connection successful" : "BMP085 connection failed");
  //
  // joystick
  pinMode(joySwitch, INPUT);
  //
  // setup Interrupt Service Routine
  attachInterrupt(0,joySwitchISR,FALLING); // uses pin 2
  //
  // get temperature and pressure
   getPressure(); // take a first reading 
   localTemp= temperature;
   localPressure = pressure; 
  //  
  u8g.firstPage();  
    do {
      splash(); 
    } while( u8g.nextPage() );
    delay(2000);
  //
  // on board LED
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW); // turn off LED//// clear the data strings
  //  
  // Buzzer
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);  // turn the buzzer ON briefly
  //   
}

/*******************************************************/

void loop() {
  // see which screen to display
  switch (displayScreen){
    case 0:
      // draw an analog screen
      u8g.firstPage();  
      do {
        drawAnalog(); 
      } while( u8g.nextPage() );
      break;
    // 
    case 1:
      // digital display
     u8g.firstPage();  
     do {
       drawDigital(); 
     } while( u8g.nextPage() );
     break;
    //
    case 2:
      // set Alarm display
     u8g.firstPage();  
     do {
       drawSetAlarm(); 
     } while( u8g.nextPage() );
     break;     
    //   
    case 3:
      // timer display
     u8g.firstPage();  
     do {   
       drawTimer(); 
     } while( u8g.nextPage() );
     break;     
    //
    case 4:
      // pressure display
     u8g.firstPage();      
     do {
       drawPressure(); // draw pressure
     } while( u8g.nextPage() ); 
     break;
     //
     case 5:
      // plot pressure display
     u8g.firstPage();     
     do {
       plotPressure(); // plot pressure
     } while( u8g.nextPage() );  
     break;  
     //
     case 6:
      // weather forcast display
     u8g.firstPage();      
     do {
       weatherForcast(); // weather forcast
     } while( u8g.nextPage() ); 
     break;  
     //   
     case 7:
      // temperature display
     u8g.firstPage();      
     do {
       drawTemperature(); // draw temperature
     } while( u8g.nextPage() ); 
     break; 
     //
     case 8:
      // plot temperature display
     u8g.firstPage();      
     do {
       plotTemperature(); // draw temperature
     } while( u8g.nextPage() ); 
     break;  
     //
     case 9:
      // calendar display single day
     u8g.firstPage();      
     do {
       drawMoon(); // draw moon
     } while( u8g.nextPage() ); 
     break;      
     //
     case 10:
      // calendar display single day
     u8g.firstPage();      
     do {
       drawCalendar2(); // draw temperature
     } while( u8g.nextPage() ); 
     break;      
     //
     case 11:
      // calendar display
     u8g.firstPage();      
     do {
       drawCalendar(); // draw temperature
     } while( u8g.nextPage() ); 
     break;             
  } // end of switch loop
 // 
 // check if additional screens have been selected on the pressure plot
  if(displayScreen == 12){
    // show data from 24 hours ago
    u8g.firstPage();      
    do {
      plotPressure_24(); //  pressure data from 24 hours ago
    } while( u8g.nextPage() ); 
  }
  if(displayScreen == 13){
    // show data from 36 hours ago
    u8g.firstPage();      
    do {
      plotPressure_48(); // pressure data from 36 hours ago
    } while( u8g.nextPage() ); 
  } 
  // additional temperature screens
  if(displayScreen == 14){
    // show data from 24 hours ago
    u8g.firstPage();      
    do {
      plotTemperature_24(); //  pressure data from 24 hours ago
    } while( u8g.nextPage() ); 
  }
  if(displayScreen == 15){
    // show data from 36 hours ago
    u8g.firstPage();      
    do {
      plotTemperature_48(); // pressure data from 36 hours ago
    } while( u8g.nextPage() ); 
  }   
 // end of additional screens
 //
 // take a temperature/pressure reading every 10 seconds
 // this should even out the display using the large fonts
  DateTime now = RTC.now();  // read the RTC 
  if(int((now.second()/10)*10) == now.second()){
    getPressure();
    localTemp= temperature;
    localPressure = pressure;
  }
 // 
 // check to see if alarm time reached
  if (timeAlarmSet == true) {
    if(alarmHour == now.hour() && alarmMinute == now.minute() && now.second() < maxAlarmTime){    
      unsigned long buzzerCurrentMillis = millis();
      if(buzzerCurrentMillis - buzzerPreviousMillis >= buzzerInterval) {
        buzzerPreviousMillis = buzzerCurrentMillis;   
        if (ledState == LOW){
        ledState = HIGH;
        buzzerState = LOW;
        }
        else{
          ledState = LOW;
          buzzerState = HIGH;
        } 
        digitalWrite(ledPin, ledState); 
        digitalWrite(buzzerPin, buzzerState);   // sound buzzer
      }
    }
    else{
    timeAlarmSet == false; // make sure alarm is switched off at the end of the alarm time
    digitalWrite(buzzerPin, HIGH); // turn off buzzer
    digitalWrite(ledPin, LOW); // turn off LED        
    }
  }  
  if (timeAlarmSet == false){ // allows the alarm to switch off by pressing joystick
    digitalWrite(buzzerPin, HIGH); // turn off buzzer
    digitalWrite(ledPin, LOW); // turn off LED     
  }
  
 // reset joystick
  if(analogRead(joyPinX) < 600 && analogRead(joyPinX) > 400){
    interrupt_time4 = millis();
    if (interrupt_time4 - last_interrupt_time2 >1000) {  // debounce delay            
      xValid = true;
    }
     last_interrupt_time4 = interrupt_time4;           
  }
 // 
 // read joystick moving to right
  if(xValid == true){// allow reading to stabilise 
    if(analogRead(joyPinX) > 900) {
      xValid = false; // wait for the joystick to be released
      // reset displayScreen if additional screens were selected
     if(displayScreen == 12 || displayScreen == 13){
     displayScreen = 5;
     }
     if(displayScreen == 14 || displayScreen == 15){
     displayScreen = 8;
     }  
     //         
      displayScreen = displayScreen + 1;
      if (displayScreen > screenMax){
        displayScreen = 0;      
      }
     showData = 1; // reset the plot screens when we move away
     showC = true; // reset to show Centigrade when we move away  
     buttonFlag == false; //stop timers working when we leave screen 
     switchForecast = false; // reset to show current forecast    
    }     
  }   
//
// read joystick moving to left
  if(xValid == true){
    if(analogRead(joyPinX) < 480) {
      xValid = false; // wait for the joystick to be released
     // reset displayScreen if additional screens were selected
     if(displayScreen == 12 || displayScreen == 13){
     displayScreen = 5;
     }
     if(displayScreen == 14 || displayScreen == 15){
     displayScreen = 8;
     }  
     //       
      displayScreen = displayScreen - 1;
      if (displayScreen < 0 ){
        displayScreen = screenMax;
      }      
     showData = 1; // reset the plot screens when we move away
     showC = true; // reset to show Centigrade when we move away
     buttonFlag == false; //stop timers working when we leave screen 
     switchForecast = false; // reset to show current forecast         
    }
  }     
   
  //
  // grab a temperature and pressure reading at the top of each hour

  if(now.minute() == 0 && now.second() == 0){  
    if (doOnce ==false){
      // reset data at midnight  
      if(now.hour() == 0 && now.minute() == 0 && now.second() == 0){
        resetDataStrings(); // save data and clear todays data, reset pointer    
      }
      //
      recordPointer = now.hour();
      recordDataTemp[0][recordPointer] = localTemp;
      recordDataTemp[0][24] = recordPointer; // save the pointer
      recordDataPressure[0][recordPointer] = localPressure/100; // convert to mb
      recordDataPressure[0][24] = recordPointer; // save the pointer
      recordNumber = recordNumber + 1; // increment the pointer
      //
      // REM this next line out if print out not required
      printData(); // send data to serial monitor or could easily be modified to save data to an SD card
     }
     lastForecast = thisForecast; // save the last forecast
     doOnce = true; // stop the temperature being read again in second = 0
   }
  if(now.minute() == 0 && now.second() == 1 && doOnce == true){ 
    doOnce = false; // reset the flag
  } 
}

/*Screen 1 - Analog Clock*************************************/
//
void drawAnalog(void) {  // draws an analog clock face
  //
  DateTime now = RTC.now();
  //
  // Now draw the clock face
  u8g.drawCircle(clockCentreX, clockCentreY, 20); // main outer circle
  u8g.drawCircle(clockCentreX, clockCentreY, 2);  // small inner circle
  //
  //hour ticks
  for( int z=0; z < 360;z= z + 30 ){
  //Begin at 0° and stop at 360°
    float angle = z ;
    angle=(angle/57.29577951) ; //Convert degrees to radians
    int x2=(clockCentreX+(sin(angle)*20));
    int y2=(clockCentreY-(cos(angle)*20));
    int x3=(clockCentreX+(sin(angle)*(20-5)));
    int y3=(clockCentreY-(cos(angle)*(20-5)));
    u8g.drawLine(x2,y2,x3,y3);
  }
  // display second hand
  float angle = now.second()*6 ;
  angle=(angle/57.29577951) ; //Convert degrees to radians  
  int x3=(clockCentreX+(sin(angle)*(20)));
  int y3=(clockCentreY-(cos(angle)*(20)));
  u8g.drawLine(clockCentreX,clockCentreY,x3,y3);
  //
  // display minute hand
  angle = now.minute() * 6 ;
  angle=(angle/57.29577951) ; //Convert degrees to radians  
  x3=(clockCentreX+(sin(angle)*(20-3)));
  y3=(clockCentreY-(cos(angle)*(20-3)));
  u8g.drawLine(clockCentreX,clockCentreY,x3,y3);
  //
  // display hour hand
  angle = now.hour() * 30 + int((now.minute() / 12) * 6 )   ;
  angle=(angle/57.29577951) ; //Convert degrees to radians  
  x3=(clockCentreX+(sin(angle)*(20-11)));
  y3=(clockCentreY-(cos(angle)*(20-11)));
  u8g.drawLine(clockCentreX,clockCentreY,x3,y3);
  //
  // display greeting
  if (now.hour() >= 6 && now.hour() < 12){greetingTime = " Good Morning";}
  if (now.hour() >= 12 && now.hour() < 17){greetingTime = "Good Afternoon";}
  if (now.hour() >= 17 && now.hour() <= 23){greetingTime = " Good Evening";}
  if (now.hour() >= 0 && now.hour() <= 5){greetingTime = "  Sleep well";}
  u8g.setFont(u8g_font_profont15);
  u8g.drawStr(20,10, greetingTime);
  // show alarm state
  if (timeAlarmSet == true){
    u8g.drawStr(25,62, "Alarm is SET");    
  }
  else{
    u8g.drawStr(12,62, "Alarm is not set");
  }
}

/*Screen 2 Digtal Clock****************************************/
//
void drawDigital(){
  // shows time in Digital Format  
  u8g.setFont(u8g_font_profont15);
  if(timeAlarmSet){u8g.drawStr(80,10, "Alarm");} // show alarm is set   
  u8g.setFont(u8g_font_profont29); 
  //
  DateTime now = RTC.now();   
  //
  // display time in digital format
  thisTime="";
  // flash the colon at 1 second interval
  if ((now.second()/2)*2 == now.second()){
    thisTime=String(now.hour()) + ":";
  }
  else {
    thisTime=String(now.hour()) + ".";
  }
  //
  if (now.minute() < 10){ thisTime = thisTime + "0";} // add leading zero if required
  thisTime=thisTime + String(now.minute());
  if (now.hour() < 10){thisTime = "0" + thisTime;} // add leading zero if required 
  const char* newTime = (const char*) thisTime.c_str();
  u8g.drawStr(25,40, newTime);   
// display date at bottom of screen
  u8g.setFont(u8g_font_profont15);
  thisDay = String(now.day(), DEC) + "/"; 
  thisMonth="";
  for (int i=0; i<=2; i++){
    thisMonth += monthString[monthIndex[now.month()-1]+i];
  }  
  thisDay=thisDay + thisMonth + "/"; 
  thisDay=thisDay + String(now.year() , DEC);
  const char* newDay = (const char*) thisDay.c_str(); 
  u8g.drawStr(25,63, newDay);    
}

/*Screen 3 - Set Alarm Time*************************************/
//

 void drawSetAlarm(){
   // set the Alarm time 
   u8g.setFont(u8g_font_profont15);
   u8g.drawStr(15,10, "Set Alarm time:"); 
   if (alarmSetMinutes == true){
     u8g.drawStr(1,60, "CLICK to Set Hours");
   }
   else{
     u8g.drawStr(5,60, "CLICK to Set Mins");
   }     
   u8g.setFont(u8g_font_profont29); 
   alarmThisTime = String(alarmMinute);
   //
   if (alarmMinute < 10){alarmThisTime = "0" + alarmThisTime;} // add leading zero if required 
   alarmThisTime = String(alarmHour) + ":" + alarmThisTime;
   if (alarmHour < 10){alarmThisTime = "0" + alarmThisTime;} // add leading zero if required 
   const char* newTime = (const char*) alarmThisTime.c_str();
   u8g.drawStr(25,40, newTime); // show time
   if (alarmSetMinutes == true){   
     u8g.drawStr(72,41, "__"); // show setting minutes 
   }
   else{
     u8g.drawStr(24,41, "__"); // show setting hours  
   }    
   // read Y joystick moving up and down
   // read joystick moving down
   if(analogRead(joyPinY) > 800) { 
     unsigned long interrupt_time2 = millis();
     if (interrupt_time2 - last_interrupt_time2 >800) { 
       // set minutes
       if (alarmSetMinutes == true){
         alarmMinute = alarmMinute - 1;
         if(alarmMinute < 0){
           alarmMinute = 59;
         }
       } 
       // set hours
       else{  // set hours
         alarmHour = alarmHour - 1;
         if(alarmHour < 0){
           alarmHour = 23;
         }      
       } 
     last_interrupt_time2 = interrupt_time2;       
     } 
   }
 //
 // read joystick moving up
   if(analogRead(joyPinY) < 200) {   
     unsigned long interrupt_time2 = millis();
     if (interrupt_time2 - last_interrupt_time2 >800) { // slow things down
       // set minutes
       if (alarmSetMinutes == true){
         alarmMinute = alarmMinute +1;
         if(alarmMinute >59){
           alarmMinute = 0;
         }
       } 
       // set hours
       else{  // set hours
         alarmHour = alarmHour +1;
         if(alarmHour >23){
           alarmHour = 0;
         }      
       }
     last_interrupt_time2 = interrupt_time2;       
     } 
   } 
 }
/*Screen 4 - Event Timer***************************************/
//
void drawTimer(){
  // count up timer, max 99 minutes  
  u8g.setFont(u8g_font_profont15);
  u8g.drawStr(45,10, "Timer:");  
 // 
  if (buttonFlag == false){  
    u8g.drawStr(20,60, "CLICK to Start");   
    u8g.setFont(u8g_font_profont29);
    u8g.drawStr(25,40, newTimeTimer);
    timerSecs = -1;
    timerMins = 0;    
  }
  else{
    u8g.drawStr(20,60, "CLICK to Stop");
    unsigned long currentMillis = millis();
    if(currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;        
      timerSecs = timerSecs + 1;       
      if (timerSecs == 60){ 
        timerSecs = 0;  
        timerMins = timerMins + 1;
        if (timerMins >= 99){
          buttonFlag = false; // stop counting
        }
      }  
    }  
  // now format the time string  
   thisTimeTimer = String(timerSecs);
   if (timerSecs < 10){thisTimeTimer = "0" + thisTimeTimer;} 
   thisTimeTimer = String(timerMins) + ":" + thisTimeTimer;
   if (timerMins < 10){ thisTimeTimer= "0" + thisTimeTimer;} // add leading zero if required           
   newTimeTimer = (const char*) thisTimeTimer.c_str();
   u8g.setFont(u8g_font_profont29);   
   u8g.drawStr(25,40, newTimeTimer);   
}
}

/*Screen 5 - Show Pressure**************************************/
//
 void drawPressure(){
   // displays current pressure in mb
   float temp = 0;
   u8g.setFont(u8g_font_profont15);
   u8g.drawStr(35,10, "Pressure:");
   //
   // check value against last reading
   u8g.setFont(u8g_font_profont15); 
   if(recordDataPressure[0][recordPointer-1] > 0.00){
     int tempReading = int(recordDataPressure[0][recordPointer]);
     if(int(localPressure/100) > tempReading){ 
       u8g.drawStr(30,60, "**Rising**"); 
     }
     if(int(localPressure/100) < tempReading){ 
       u8g.drawStr(30,60, "**Falling**"); 
     }       
     if(int(localPressure/100) == tempReading){ 
       u8g.drawStr(25,60, "**Constant**"); 
     }  
     if((localPressure/100) < 900){ 
       u8g.drawStr(30,60, " **STORM**"); 
     } 
   }
   else{
     u8g.drawStr(22,60, " **Waiting**");      
   }  
   thisPressure = String(int(localPressure/100)) + "mb"; 
   const char* newPressure = (const char*) thisPressure.c_str();
   u8g.setFont(u8g_font_profont29);  
   u8g.drawStr(20,40, newPressure); 
   // show if the pressure is rising or falling   
 }

/*Screen 6 - Plot Pressure*************************************/
//
void plotPressure(){
 // displays a plot of the pressure over the last 24 hours
 //  
   u8g.setFont(u8g_font_5x7); 
   u8g.drawStr(30,10, "Pressure Today");
   u8g.drawFrame(0,0,128,64);
   u8g.drawLine(19,64,19,59);
   u8g.drawLine(34,64,34,59);
   u8g.drawLine(49,64,49,59);   
   u8g.drawLine(64,64,64,54); // 12 hour mark
   u8g.drawLine(79,64,79,59);    
   u8g.drawLine(94,64,94,59);
   u8g.drawLine(109,64,109,59);
  // horizontal lines
   // the baseline is 900mb
   // the top line is 1020mb
   u8g.drawLine(0,42,128,42); // 1000mb  
   u8g.drawLine(0,21,128,21); // 1010mb
   if(recordDataPressure[0][24] == 0.00){
     u8g.drawStr(19,35, "Data not available");
   }    
   // now plot graph
   // x starts at 4 pixels and each hour is 5 pixels
   // y is 2.1 pixels per 10mb 
   // as 0,0 pixel is top left, value for y should be subtracted from 64
     for(int f = 0; f < recordPointer+1;f++){  
   // limit range to 990 - 1020mb
       tempYvalueP = recordDataPressure[0][f];
       if(tempYvalueP < 990){
         tempYvalueP = 990; // dont allow values below 990mb
       }
       if(tempYvalueP > 1020){
         tempYvalueP = 1020; // dont allow values over 1020mb
       }     
       u8g.drawCircle((5*f)+4,64 -(2.1*(tempYvalueP-990)),1);
     }
}
/**************************************************************/
void plotPressure_24(){
 // displays a plot of the pressure from Yesterday
 // Screen 11
 //  
   u8g.setFont(u8g_font_5x7); 
   u8g.drawStr(19,10, "Pressure Yesterday");
   u8g.drawFrame(0,0,128,64);
   u8g.drawLine(19,64,19,59);
   u8g.drawLine(34,64,34,59);
   u8g.drawLine(49,64,49,59);   
   u8g.drawLine(64,64,64,54); // 12 hour mark
   u8g.drawLine(79,64,79,59);    
   u8g.drawLine(94,64,94,59);
   u8g.drawLine(109,64,109,59);
  // horizontal lines
   // the baseline is 900mb
   // the top line is 1020mb
   u8g.drawLine(0,42,128,42); // 1000mb  
   u8g.drawLine(0,21,128,21); // 1010mb  
   // now plot graph
   // x starts at 4 pixels and each hour is 5 pixels
   // y is 2.1 pixels per 10mb 
   // as 0,0 pixel is top left, value for y should be subtracted from 64
   if(recordDataPressure[1][24] == 0.00){
     u8g.drawStr(19,35, "Data not available");
   }
     for(int f = 0; f < recordDataPressure[1][24]+1;f++){  
     // limit range to 990 - 1020mb
       tempYvalueP = recordDataPressure[1][f];
       if(tempYvalueP < 990){
         tempYvalueP = 990; // dont allow values below 990mb
       }
       if(tempYvalueP > 1020){
         tempYvalueP = 1020; // dont allow values over 1020mb
       }     
       u8g.drawCircle((5*f)+4,64 -(2.1*(tempYvalueP-990)),1);
     }
}
/**************************************************************/
void plotPressure_48(){
 // displays a plot of the pressure from day before Yesterday
 // Screen 12
 //  
   u8g.setFont(u8g_font_5x7); 
   u8g.drawStr(16,10, "Pressure 48 hrs ago");   
   u8g.drawFrame(0,0,128,64);
   u8g.drawLine(19,64,19,59);
   u8g.drawLine(34,64,34,59);
   u8g.drawLine(49,64,49,59);   
   u8g.drawLine(64,64,64,54); // 12 hour mark
   u8g.drawLine(79,64,79,59);    
   u8g.drawLine(94,64,94,59);
   u8g.drawLine(109,64,109,59);
  // horizontal lines
   // the baseline is 900mb
   // the top line is 1020mb
   u8g.drawLine(0,42,128,42); // 1000mb  
   u8g.drawLine(0,21,128,21); // 1010mb  
   // now plot graph
   // x starts at 4 pixels and each hour is 5 pixels
   // y is 2.1 pixels per 10mb 
   // as 0,0 pixel is top left, value for y should be subtracted from 64
   if(recordDataPressure[2][24] == 0.00){
     u8g.drawStr(19,35, "Data not available");
   }
     for(int f = 0; f < recordDataPressure[2][24]+1;f++){  
     // limit range to 990 - 1020mb
       tempYvalueP = recordDataPressure[2][f];
       if(tempYvalueP < 990){
         tempYvalueP = 990; // dont allow values below 990mb
       }
       if(tempYvalueP > 1020){
         tempYvalueP = 1020; // dont allow values over 1020mb
       }     
       u8g.drawCircle((5*f)+4,64 -(2.1*(tempYvalueP-990)),1);
     }
}
/*Screen 7 - Weather Forecast**********************************/
//
void weatherForcast(){
  /*
  Average sea level pressure is 1013mb
  
  12 to 24 hour forecast
  
  Reading       Rising or steady    Slowly falling   Rapidliy falling
  Over 1022mb   Continued Fair      Fair             Cloudy, Warmer
  1009-1022mb   Same as present     Little change    Precipitation likely
  Under 1009mb  Clearing, cooler    Precipitation    Storm
  */
  // uses pressure to forcast weather
   u8g.drawLine(0,50,128,50);    
   u8g.setFont(u8g_font_profont12); 
   u8g.drawStr(20,10, "Weather Forcast"); 
   u8g.drawLine(0,15,128,15); // draw horizontal line   
 // we need at least two readings to make a forecast
 // forecast will not be available until 0100 each day 
  if(recordNumber < 2){
    u8g.drawStr(16,35, "Waiting for Data");
  }    
 // if data available the show forecast
  else{ 
    // report pressure
    if(int(recordDataPressure[0][recordPointer-1]) > int(recordDataPressure[0][recordPointer])){ // pressure is falling  
      //u8g.drawStr(5,40, "Pressure is falling");
      if (int(recordDataPressure[0][recordPointer-1]) - int(recordDataPressure[0][recordPointer]) < 2){ // falling slowly
        if (int(recordDataPressure[0][recordPointer-1]) < 1009){
          //u8g.drawStr(3,52, "Precipitation likely");
        thisForecast = "Precipitation likely";   
        } 
        if(int(recordDataPressure[0][recordPointer]) > 1008 && int(recordDataPressure[0][recordPointer-2]) < 1020){
          //u8g.drawStr(25,52, "Little change.");
        thisForecast = "    Little change";          
         }
        if (int(recordDataPressure[0][recordPointer]) > 1019){
          //u8g.drawStr(25,52, "Remaining Fair");  
        thisForecast = "   Remaining Fair";          
        }         
      }
   //   
      if(int(recordDataPressure[0][recordPointer-1]) - int(recordDataPressure[0][recordPointer]) > 2){ // falling more rapidly
      //u8g.drawStr(5,40, "Pressure is falling"); 
        if (int(recordDataPressure[0][recordPointer]) < 1009){        
          //u8g.drawStr(23,52, "Stormy weather"); 
        thisForecast = "   Stormy weather";           
        }
        if (int(recordDataPressure[0][recordPointer]) > 1008 && int(recordDataPressure[0][recordPointer]) <1020){        
          //u8g.drawStr(3,52, "Precipitation likely"); 
        thisForecast = "Precipitation likely";          
        } 
        if (int(recordDataPressure[0][recordPointer]) >1019){        
          //u8g.drawStr(22,52, "Cloudy, warmer");
        thisForecast = "    Cloudy, warmer";          
        }        
      }      
    }
    //  
    if(int(recordDataPressure[0][recordPointer]) == int(recordDataPressure[0][recordPointer-1])){   // pressure is constant 
      //u8g.drawStr(5,40, "Pressure is constant"); 
      if (int(recordDataPressure[0][recordPointer]) < 1009) {
        //u8g.drawStr(15,52, "Clearing, cooler.");
        thisForecast = "   Clearing, cooler";        
      }
      if(int(recordDataPressure[0][recordPointer]) >1008 and int(recordDataPressure[0][recordPointer]) <1020){
        //u8g.drawStr(8,52, "Continuing the same");
        thisForecast = " Continuing the same";        
      } 
      if(int(recordDataPressure[0][recordPointer]) > 1019){
        //u8g.drawStr(9,52, "Remaining Fair");
        thisForecast = "   Remaining Fair";        
      }     
    }
    //
    if(int(recordDataPressure[0][recordPointer]) > int(recordDataPressure[0][recordPointer-1])){ // pressure is rising   
      //u8g.drawStr(10,40, "Pressure is rising"); 
      if (int(recordDataPressure[0][recordPointer]) < 1009) {
        //u8g.drawStr(17,52, "Clearing, cooler");
        thisForecast = "   Clearing, cooler";        
      }
      if(int(recordDataPressure[0][recordPointer]) >1008 and int(recordDataPressure[0][recordPointer]) <1020){
        //u8g.drawStr(9,52, "Continue same");
        thisForecast = " Continuing the same";        
      } 
      if(int(recordDataPressure[0][recordPointer]) > 1019){
        //u8g.drawStr(9,52, "Remaining Fair"); 
        thisForecast = "   Remaining Fair";        
      }     
    }
  }
    
    if(switchForecast == false){
    // now print current forecast
      const char*newthisForecast = (const char*) thisForecast.c_str();
      u8g.drawStr(3,35, newthisForecast);
      u8g.setFont(u8g_font_5x7);    
      u8g.drawStr(5,60, "Click for last forecast");       
    }
    else{
    // now print last forecast from 2 hours ago
      const char*newlastForecast = (const char*) lastForecast.c_str();    
      if (lastForecast == ""){
        u8g.drawStr(16,35, "Waiting for Data");
      }
      else{
        u8g.drawStr(3,35, newlastForecast);      
      }
      u8g.setFont(u8g_font_5x7);    
      u8g.drawStr(1,60, "Click for latest forecast");       
    }  
}
/*Screen 8 - Show temperature***********************************/
//
 void drawTemperature(){
  // displays local temperature
   u8g.setFont(u8g_font_profont15);
   u8g.drawStr(25,10, "Temperature:");
 //
   if(showC == false){
   localTempF = localTemp * 1.8 + 32.0; 
   thisTemperature = String(int(localTempF)) + "*F";   
   }
   else{
     thisTemperature = String(int(localTemp)) + "*C"; 
   }
   const char* newTemperature = (const char*) thisTemperature.c_str();
   u8g.setFont(u8g_font_profont29);    
   u8g.drawStr(35,40, newTemperature); 
   // add a greeting 
   if(localTemp < 4){greetingTemp = "   Ice Warning!";}
   if(localTemp >= 4 && localTemp <18){greetingTemp = "It's a bit chilly";}
   if(localTemp >=18 && localTemp <25){greetingTemp = "   Just right!";}
   if(localTemp >24){greetingTemp = "Getting too warm!";}
   u8g.setFont(u8g_font_profont15);
   u8g.drawStr(10,60, greetingTemp);  
 }

/*Screen 9 - Plot temperature over 24 hours********************/
//
void plotTemperature(){
  // displays a plot of the temperature over the last 24 hours
   u8g.setFont(u8g_font_5x7); 
   u8g.drawStr(20,10, "Temperature Today");   
   u8g.drawFrame(0,0,128,64);
   u8g.drawLine(19,64,19,59);
   u8g.drawLine(34,64,34,59);
   u8g.drawLine(49,64,49,59);   
   u8g.drawLine(64,64,64,54); // 12 hour mark
   u8g.drawLine(79,64,79,59);    
   u8g.drawLine(94,64,94,59);
   u8g.drawLine(109,64,109,59);
  // horizontal lines
   u8g.drawLine(0,48,128,48); // 10 degrees C  
   u8g.drawLine(0,32,128,32); // 20 degrees C 
   u8g.drawLine(0,16,128,16); // 30 degrees C 
   // now plot graph
   // x starts at 4 pixels and each hour is 5 pixels
   // y is 1.6 pixels per degree centigrade 
   // as 0,0 pixel is top left, value for y should be subtracted from 64
   if(recordDataTemp[0][24] == 0.00){
     u8g.drawStr(19,28, "Data not available");
   } 
   else{
     for(int f = 0; f < recordPointer+1;f++){  
     // limit range to 0 -40 C
       tempYvalue = recordDataTemp[0][f];
       if(tempYvalue <0){
         tempYvalue = 0; // dont allow values below zero
       }
       if(tempYvalue > 40){
         tempYvalue = 40; // dont allow values over 40 centigrade
       }     
       u8g.drawCircle((5*f)+4,64 -(1.6*tempYvalue),1);
       }
   }
}
/********************************************************/ 
void plotTemperature_24(){
  // displays a plot of the temperature over the last 24 hours
   u8g.setFont(u8g_font_5x7); 
   u8g.drawStr(25,10, "Temp. Yesterday");   
   u8g.drawFrame(0,0,128,64);
   u8g.drawLine(19,64,19,59);
   u8g.drawLine(34,64,34,59);
   u8g.drawLine(49,64,49,59);   
   u8g.drawLine(64,64,64,54); // 12 hour mark
   u8g.drawLine(79,64,79,59);    
   u8g.drawLine(94,64,94,59);
   u8g.drawLine(109,64,109,59);
  // horizontal lines
   u8g.drawLine(0,48,128,48); // 10 degrees C  
   u8g.drawLine(0,32,128,32); // 20 degrees C 
   u8g.drawLine(0,16,128,16); // 30 degrees C 
   // now plot graph
   // x starts at 4 pixels and each hour is 5 pixels
   // y is 1.6 pixels per degree centigrade 
   // as 0,0 pixel is top left, value for y should be subtracted from 64
   if(recordDataTemp[1][24] == 0.00){
     u8g.drawStr(19,28, "Data not available");
   } 
   else{
     for(int f = 0; f <recordDataTemp[1][24]+1;f++){  
     // limit range to 0 -40 C
     tempYvalue = recordDataTemp[1][f];
     if(tempYvalue <0){
       tempYvalue = 0; // dont allow values below zero
     }
     if(tempYvalue > 40){
       tempYvalue = 40; // dont allow values over 40 centigrade
     }     
     u8g.drawCircle((5*f)+4,64 -(1.6*tempYvalue),1);
     }
   }
}
/********************************************************/ 
void plotTemperature_48(){
  // displays a plot of the temperature over the last 24 hours
   u8g.setFont(u8g_font_5x7); 
   u8g.drawStr(23,10, "Temp. 48 hrs ago");   
   u8g.drawFrame(0,0,128,64);
   u8g.drawLine(19,64,19,59);
   u8g.drawLine(34,64,34,59);
   u8g.drawLine(49,64,49,59);   
   u8g.drawLine(64,64,64,54); // 12 hour mark
   u8g.drawLine(79,64,79,59);    
   u8g.drawLine(94,64,94,59);
   u8g.drawLine(109,64,109,59);
  // horizontal lines
   u8g.drawLine(0,48,128,48); // 10 degrees C  
   u8g.drawLine(0,32,128,32); // 20 degrees C 
   u8g.drawLine(0,16,128,16); // 30 degrees C 
   // now plot graph
   // x starts at 4 pixels and each hour is 5 pixels
   // y is 1.6 pixels per degree centigrade 
   // as 0,0 pixel is top left, value for y should be subtracted from 64
   if(recordDataTemp[2][24] == 0.00){
     u8g.drawStr(19,28, "Data not available");
   } 
   else{
     for(int f = 0; f < recordDataTemp[2][24]+1;f++){  
     // limit range to 0 -40 C
     tempYvalue = recordDataTemp[2][f];
     if(tempYvalue <0){
       tempYvalue = 0; // dont allow values below zero
     }
     if(tempYvalue > 40){
       tempYvalue = 40; // dont allow values over 40 centigrade
     }     
     u8g.drawCircle((5*f)+4,64 -(1.6*tempYvalue),1);
     }
   }
}

/*Screen 10 - Moon Phase*******************************************************/
//
void drawMoon(void){
  DateTime now = RTC.now();  
  u8g.setFont(u8g_font_profont12);  
  u8g.setFont(u8g_font_5x7); 
  u8g.drawStr(15,10, "Moon Phase Calculator"); 
  u8g.drawLine(0,13,128,13); 
  int mp = moon_phase();
  u8g.setFont(u8g_font_profont15);   
  switch (mp){
    case 0:
      u8g.drawStr(15,61, "  Full Moon  ");
      u8g.drawXBM(45,18,30,30,full_moon_bits);      
    break;
    case 1:
      u8g.drawStr(15,61, "Waning Gibbous");
      u8g.drawXBM(45,18,30,30,waning_gibbous_bits);
    break;
    case 2:
      u8g.drawStr(15,61, " Last Quarter ");
      u8g.drawXBM(45,18,30,30,last_quarter_bits);      
    break;
    case 3:
      u8g.drawStr(15,61, " Old Crescent ");
      u8g.drawXBM(45,18,30,30,crescent_old_bits);      
    break;     
    case 4:
      u8g.drawStr(15,61, "   New Moon   ");
      u8g.drawXBM(45,18,30,30,new_moon_bits);      
    break;  
    case 5:
      u8g.drawStr(15,61, " New Crescent ");
      u8g.drawXBM(45,18,30,30,crescent_new_bits);      
    break; 
    case 6:
      u8g.drawStr(15,61, " First Quarter");
      u8g.drawXBM(45,18,30,30,first_quarter_bits);      
    break;
    case 7:
      u8g.drawStr(15,61, "Waxing Gibbous");
      u8g.drawXBM(45,18,30,30,waxing_gibbous_bits);      
    break;   
  }
 const char* newNfm = (const char*) nfm.c_str();  
 u8g.drawStr(110,30, newNfm); 
}

int moon_phase(){
  // calculates the age of the moon phase(0 to 7)
  // there are eight stages, 0 is full moon and 4 is a new moon
  DateTime now = RTC.now();  
  double jd = 0; // Julian Date
  double ed = 0; //days elapsed since start of full moon
  int b= 0;
  jd = julianDate(now.year(), now.month(), now.day());
  //jd = julianDate(1972,1,1); // used to debug this is a new moon
  jd = int(jd - 2244116.75); // start at Jan 1 1972
  jd /= 29.53; // divide by the moon cycle    
  b = jd;
  jd -= b; // leaves the fractional part of jd
  ed = jd * 29.53; // days elapsed this month
  nfm = String((int(29.53 - ed))); // days to next full moon
  b = jd*8 +0.5;
  b = b & 7; 
  return b;
   
}
double julianDate(int y, int m, int d){
// convert a date to a Julian Date}
  int mm,yy;
  double k1, k2, k3;
  double j;
  
  yy = y- int((12-m)/10);
  mm = m+9;
  if(mm >= 12) {
    mm = mm-12;
  }
  k1 = 365.25 *(yy +4172);
  k2 = int((30.6001 * mm) + 0.5);
  k3 = int((((yy/100) + 4) * 0.75) -38);
  j = k1 +k2 + d + 59;
  j = j-k3; // j is the Julian date at 12h UT (Universal Time)

  return j;
}
/*Screen 11 - Show Date****************************************/
//
 void drawCalendar2(){
  // show todays date  
  DateTime now = RTC.now(); // get date
  for (int i=0; i<=2; i++){
    monthName2[i]=monthString2[monthIndex2[now.month()-1]+i];
  }      
  u8g.setFont(u8g_font_profont22); 
  u8g.drawStr(47,22, monthName2);  
  String dayNow = String(now.day());
  if (now.day() < 10){
    dayNow = "0" + dayNow;
  }
  const char* newCal2 = (const char*) dayNow.c_str();
  u8g.setFont(u8g_font_fub30);  
  u8g.drawStr(41,60, newCal2); 
  // draw boxes
  u8g.drawFrame(30,0,65,64);
  u8g.drawFrame(30,0,65,27);
// draw circles
  u8g.drawCircle(37,10,3);
  u8g.drawCircle(87,10,3);  
 }

/*Screen 12 - Show months calendar*****************************/ 
//
 void drawCalendar(){
  // display a full month on a calendar 
  u8g.setFont(u8g_font_profont12);
  u8g.drawStr(2,9, "Su Mo Tu We Th Fr Sa"); 
  // display this month
  DateTime now = RTC.now();
  //
  startDay = startDayOfWeek(now.year(),now.month(),1); // Sunday's value is 0
  // now build first week string
  switch (startDay){
    case 0:
      // Sunday
      week1 = " 1  2  3  4  5  6  7";
      break;
    case 1:
      // Monday
      week1 = "    1  2  3  4  5  6";
      break;      
     case 2:
      // Tuesday
      week1 = "       1  2  3  4  5";
      break;           
     case 3:
      // Wednesday
      week1 = "          1  2  3  4";
      break;  
     case 4:
      // Thursday
      week1 = "             1  2  3";
      break; 
     case 5:
      // Friday
      week1 = "                1  2";
      break; 
     case 6:
      // Saturday
      week1 = "                   1";
      break;           
  } // end first week
  newWeekStart = (7-(7- startDay))+2;
  const char* newWeek1 = (const char*) week1.c_str();  
  u8g.drawStr(2,19,newWeek1); 
  // display week 2
  week2 ="";
  for (int f = newWeekStart; f < newWeekStart + 7; f++){
    if(f<10){
      week2 = week2 +  " " + String(f) + " ";
    }  
    else{week2 = week2 + String(f) + " ";}    
  }
  const char* newWeek2 = (const char*) week2.c_str();  
  u8g.drawStr(2,29,newWeek2); 
  // display week 3
  newWeekStart = (14-(7- startDay))+2; 
  week3 ="";
  for (int f = newWeekStart; f < newWeekStart + 7; f++){
    if(f<10){
      week3 = week3 +  " " + String(f) + " ";
    }  
    else{week3 = week3 + String(f) + " ";}    
  }
  const char* newWeek3 = (const char*) week3.c_str();  
  u8g.drawStr(2,39,newWeek3);     
  // display week 4
  newWeekStart = (21-(7- startDay))+2; 
  week4 ="";
  for (int f = newWeekStart; f < newWeekStart + 7; f++){
    if(f<10){
      week4 = week4 +  " " + String(f) + " ";
    }  
    else{week4 = week4 + String(f) + " ";}    
    }
   const char* newWeek4 = (const char*) week4.c_str();  
   u8g.drawStr(2,49,newWeek4); 
   // do we need a fifth week
   week5="";
   newWeekStart = (28-(7- startDay))+2;   
   // is is February?
   if(newWeekStart > 28 && now.month() == 2){
   // do nothing unless its a leap year
     if (now.year()==(now.year()/4)*4){ // its a leap year
       week5 = "29";
     }       
   }
   else{ // print up to 30 anyway
     for (int f = newWeekStart; f < 31; f++){
       week5 = week5 + String(f) + " ";
     }
     // are there 31 days
     if (now.month() == 1 || now.month() == 3 || now.month() == 5 || now.month() == 7 || now.month() == 8 || now.month() == 10 || now.month() == 12){
       week5 = week5 + "31"; 
     }  
   }
   const char* newWeek5 = (const char*) week5.c_str();  
   u8g.drawStr(2,59,newWeek5);
 } 
/**************************************************************/
//
// Sub Routines and Functions
//
void resetDataStrings(){
// move data strings down in memory and reset data string 0 to all zeros
  for(int j = 0; j<25;j++){
    recordDataTemp[2][j] = recordDataTemp[1][j];
    recordDataTemp[1][j] = recordDataTemp[0][j];
    recordDataTemp[0][j] = 0.00; 
  }
//
  for(int j = 0; j<25;j++){
    recordDataPressure[2][j] = recordDataPressure[1][j];
    recordDataPressure[1][j] = recordDataPressure[0][j];
    recordDataPressure[0][j] = 0.00;    
  } 
//
  recordPointer = 0; // reset pointer
  recordNumber = 0; // reset number of available data points
  }
/**************************************************************/
 void printData(){
 // send temperature and pressure data to serial monitor
   DateTime now = RTC.now();
   Serial.print("Hour = ");
   Serial.print(recordPointer);
   Serial.println("   " + String(now.day()) + "-" + String(now.month()) + "-" + String(now.year()));
   Serial.println("Todays Temperature data:");
   for(int f = 0; f<25;f++){
     if(recordDataTemp[0][f] == 0.00){
       Serial.print("**");
     }
     else{
      Serial.print(recordDataTemp[0][f]);
     }
     if(f == 24){
       Serial.println(":");
     }
     else{
       Serial.print(",");
     }
   }    
   Serial.println("Yesterdays Temperature data:");
   for(int f = 0; f<25;f++){
     if(recordDataTemp[1][f] == 0.00){
       Serial.print("**");
     } 
     else{    
       Serial.print(recordDataTemp[1][f]);
     }
     if(f == 24){
       Serial.println(":");
     }
     else{
       Serial.print(",");
     }
   }     
   Serial.println("48 hours ago Temperature data:");
   for(int f = 0; f<25;f++){
     if(recordDataTemp[2][f] == 0.00){
       Serial.print("**");
     }     
     else{
      Serial.print(recordDataTemp[2][f]);
     }
     if(f == 24){
       Serial.println(":");
     }
     else{
       Serial.print(",");
     }
   }  
   Serial.println("");
   Serial.println("Todays Pressure data:");
   for(int f = 0; f<25;f++){
     if(recordDataPressure[0][f] == 0.00){
       Serial.print("**");
     }
     else{
       Serial.print(recordDataPressure[0][f]);
     }
     if(f == 24){
       Serial.println(":");
     }
     else{
       Serial.print(",");
     }
   }    
   Serial.println("Yesterdays Pressure data:");
   for(int f = 0; f<25;f++){
     if(recordDataPressure[1][f] == 0.00){
       Serial.print("**");
     }
     else{     
       Serial.print(recordDataPressure[1][f]);
     }
     if(f == 24){
       Serial.println(":");
     }
     else{
       Serial.print(",");
     }
   }     
   Serial.println("48 hours ago Pressure data:");
   for(int f = 0; f<25;f++){
     if(recordDataPressure[2][f] == 0.00){
       Serial.print("**");
     } 
     else{    
       Serial.print(recordDataPressure[2][f]);
     }
     if(f == 24){
       Serial.println(":");
     }
     else{
       Serial.print(",");
     }
   }  
   Serial.println("");         
   // end Debug or send to SD Card section
 }
/**************************************************************/ 
void joySwitchISR(){
  // includes a debounce routine
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time >500) { 
    if (displayScreen == 0 || displayScreen == 1){
      timeAlarmSet = !timeAlarmSet; // change alarm state
    }
    //
    if(displayScreen == 2){
      alarmSet = !alarmSet; // change alarm state 
      alarmSetMinutes = !alarmSetMinutes;     
    }
    //
    if(displayScreen == 3){
      buttonFlag = !buttonFlag;      
    }
    //   
    if(displayScreen == 5 || displayScreen  == 12 || displayScreen == 13){
      // button pressed on plot screens
      showData = showData + 1;
      if(showData > 3){
        showData = 1;
      }
      if(showData ==1){
        displayScreen =5;
      }
      if(showData ==2){
        displayScreen =12;
      }
       if(showData ==3){
        displayScreen =13;
      }     
    }
    //
    if(displayScreen ==6){
     switchForecast = ! switchForecast; // switch between current an last forecast 
    }
    //
    if(displayScreen == 7){
      showC = ! showC; // switch between C and F
    }    
    // 
    if(displayScreen == 8 || displayScreen  == 14 || displayScreen == 15){
      // button pressed on plot screens
      showData = showData + 1;
      if(showData > 3){
        showData = 1;
      }
      if(showData == 1){
        displayScreen = 8;
      }
      if(showData == 2){
        displayScreen = 14;
      }
       if(showData == 3){
        displayScreen = 15;
      }     
    }   
  } 
  last_interrupt_time = interrupt_time;
}
/********************************************************/
// 
 void getPressure(){
   // reads both pressure and temperature
   // request temperature
   barometer.setControl(BMP085_MODE_TEMPERATURE); 
   // wait appropriate time for conversion (4.5ms delay)
   lastMicros = micros();
   while (micros() - lastMicros < barometer.getMeasureDelayMicroseconds());
   // read calibrated temperature value in degrees Celsius
   temperature = barometer.getTemperatureC();
   // request pressure (3x oversampling mode, high detail, 23.5ms delay)
   barometer.setControl(BMP085_MODE_PRESSURE_3);
   while (micros() - lastMicros < barometer.getMeasureDelayMicroseconds());
   // read calibrated pressure value in Pascals (Pa)
   pressure = barometer.getPressure();
 }
/********************************************************/

// calculate first day of month
int startDayOfWeek(int y, int m, int d){
  static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  y -= m < 3;
  return (y +y/4 -y/100 + y/400 + t[m-1] + d)% 7; 
} 

/********************************************************/ 
void splash(){
  u8g.setFont(u8g_font_profont12); 
  u8g.drawStr(7,10, "Joystick Controlled");  
  u8g.drawStr(21,30, "Weather Station");
  u8g.drawLine(0,40,128,40); 
  u8g.drawStr(24,55, "by Chris Rouse");    
}
/********************************************************/

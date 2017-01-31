/***********************************************************
Joystick controlled Clock using OLED Display
by Chris Rouse August 2015

Sketch updated February to include a Humidity Sensor DHT11 or DHT22
A DS3231 RTC Chip should be used instead of the DS1307 to give better time stability

The GitHub page contains all the libraries used with this project

This sketch uses an Arduino Mega to provide more memory

The program uses 73% of dynamic memory.

OLED Analog Clock using U8GLIB Library

visit https://code.google.com/p/u8glib/ for full details of the U8GLIB library and
full instructions for use.

Using a IIC 128x64 OLED with SSD1306 chip and RTC DS1307 

Connections: (All Pins are for the Arduino MEGA)

Wire Buzzer Alarm::
  Vcc connect to Arduino 5 volts
  Input connect to BuzzerPin
  Gnd connect to Arduino Gnd

//
//
Wire Joystick::
  Gnd    ---> Arduino Gnd
  Vcc    ---> Arduino 5 volts
  
  Xout   ---> Arduino A0 with outpu pins on left
  Yout   ---> Arduino A1 with output pins on left
  
  Xout   ---> Arduino A1 with outpu pins on top
  Yout   ---> Arduino A0 with output pins on top  
  
  Switch ---> Arduino Digital pin 2 (must be this pin for ISR to work)
//
//
Wire RTC::
  VCC +5v
  GND GND
  SDA Arduino Mega pin 20
  SCL Arduino Mega pin 21
//
Wire DHT11 humidity Sensor
 VCC +5v
 OUT Arduino pin 2
 GND GND
//
Wire OLED::
  VCC +5v
  GND GND
  SDA Arduino Mega pin 20
  SCL Arduino Mega pin 21
//
//
Wire SD Card:: (pins shown for Arduino Mega)
  GND    ---> Arduino GND
  +3.3V  ---> N/A
  +5V    ---> Arduino 5V
  // next 4 pins must be via a Logic Level Converter, unless there is one on the SD card board
  CS     ---> Arduino Mega pin 53
  MOSI   ---> Arduino Mega pin 51
  SCLK   ---> Arduino Mega pin 52
  MISO   ---> Arduino Mega pin 50
  //
  GND    ---> N/A
//
//
Wire Logic Level Converter::
(if Needed for SD Card)
  Gnd ---> Arduino Gnd
  3.3v NOT CONNECTED IF board has a 3.3v regulator, otherwise Arduino 3.3v
  +5v ---> Arduino 5 v
  5 volt side 
  A1 ---> Arduino pin 51
  A2 ---> Arduino pin 50
  A3 ---> Arduino pin 52
  3.3v side
  B1 ---> SD Card MOSI pin 5
  B2 ---> SD Card MISO pin 7
  B3 ---> SD Card SCLK pin 6
//
//
Wire Resistors, 10k
  Digital Pin 2 to 5v (Joystick Switch)
//
Small LED (green) to show backup data being saved to SD Card
  Connect cathode to pin 49 (blueLed variable), then Anode via a resitor, 470R to Gnd
//
2 x Small LED(blue) behind OLED as backlight if required
  backlight1 cathode to 5 volts, anode via a resitor, 470R to Gnd
  backlight2 cathode to 5 volts, anode via a resitor, 470R to Gnd
//

************************************************************/

// Add libraries
  #include "BMP085.h"  // pressure sensor
  #include <dht11.h> // Humidity Sensor
  #include "I2Cdev.h" // needed with BMP085.h
  #include "RTClib.h"  // Real time clock
  #include <SD.h>  //SD Card
  #include <SPI.h> // used in SPI interface
  #include "U8glib.h"  // graphics library
  #include <Wire.h>  // used in SPI interface
  //
/**********************************************************************/  
// User can change the following  variables as required
//
// Alarm Clock
  int alarmHour = 7; // default alarm hour
  int alarmMinute = 30; // default alarm minute
  int birthDay = 15; // your birthday
  int birthMonth = 6; // your birthday
  int maxAlarmTime = 10; //maximum time alarm sound for, seconds up to 59  
//
/**********************************************************************/
// setup u8g object
  U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE);	// I2C 
//
// Hardware Error check
  boolean errorHardware1 = false; // RealTime Clock
  boolean errorHardware2 = false; // Pressure Sensor
  boolean errorHardware3 = false; // Humidity Sensor
//
// setup DHT11 Humidity Sensor
  dht11 DHT11;
  #define DHT11PIN 12
  int checkDHT11; // used to confirm reading OK
  int dewPointValue = 0;
  const char* greetingHumidity = "";
  boolean humidityUpdate = false; // shows if humidity was manually updated
  float humidityValue = 0.0;
  String thisDewPoint = "";
  String thisHumidity = "";
//
// Setup RTC
  RTC_DS1307 RTC;
  static String monthString[12]= {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
  String thisDay="";
  String thisMonth = "";
  String thisTime = ""; 
  String thisWeekday ="";
//
// setup BMP180
  BMP085 barometer;
  boolean grabFlag = false; // used to ensure only 1 reading is taken
  float lastPressure1 = 0.00;
  float lastPressure2 = 0.00; 
  double localPressure = 0.00;
  double localTemp = 0.00;
  double localTempF = 0.00; // temp in Farenheit
  boolean pascal = false; // flag to show if pascals should be shown
  float pressure;
  boolean showC = true;  // if false show temp in Farenheit
  int showData = 1; // used to show data screen 1 = now, 2 = -24hrs, 3 = -48hrs
  boolean switchForecast = false; // if false then current forecast shown
  int temperature;
  String thisPressure = "Waiting for Data";
  String thisTemperature = "Waiting for Data";
  //
  // Data strings, used to store temperature and pressure for 48 hour
  //
  boolean doOnce = false; // only let temperature be read once for logging
  float recordDataPressure[3][25];  // array to hold Pressure data
  float recordDataTemp[3][25]; // array to hold temperature data
  int recordNumber = 0; // counts data entries in current 24 hours
  int recordPointer = 0; // points to current entry in record
//
// SD Card
  File BackUp; // text file on SD card
  int blueLed = 49; //LED used to show SD Card activity
  File ClockData; // text file on SD Card
  boolean sdPresent = false; // flag to show data can be written to SD Card
  String SDpressure = ""; // and this one with pressure  
  String SDtemperature = ""; // build this string with current day temperature
//
//
// Alarm Clock::
//
  volatile boolean alarmSet = false; // flag to show an alarm has been set
  volatile boolean alarmSetMinutes = true;
  String alarmThisTime = "";
  boolean setMinutes = true; // flag to show whether to set minutes or hours
  
//
// joystick::
//
// calibrate joystick
  volatile boolean buttonFlag = false; 
  int joyX = 0;
  boolean xValid = true;
  int joyY = 0;  
  boolean yValid = false;
//
// timer::
//
  long interval = 1000; 
  const char* newTimeTimer = "00:00";
  unsigned long previousMillis = 0;
  String thisTimeTimer = "";
  int timerMins = 0;
  int timerSecs = 0;
//
// Alarm Buzzer::
//
  const long buzzerInterval = 1000;
  int buzzerPin = 4; // pin the buzzer is connected to
  unsigned long buzzerPreviousMillis = 0; // will store last time LED was updated
  boolean ledState = false;
//
// Set start screen and maximum number of screens::
//
  volatile int displayScreen = 0; // screen number to start at
  int screenMax = 12; // highest screen number (starts at 0)
//
// general delay::
//
  unsigned long interrupt_time1 = 0;
  unsigned long interrupt_time2 = 0;  
  unsigned long last_interrupt_time1 = 0;
  unsigned long last_interrupt_time2 = 0;
  unsigned long lastMicros = 0; // used in BMP180 read routine
//
// Weather Forecast
  String longForecast = "";  // forecast based on last 2 hours
  String rise = "constant";
  float riseAmmount = 0; // ammount of rise or fall in mb
  String thisForecast = ""; // forecast based on last hour
//
// analog and digital clock displays::
//
  const char* greetingTemp = "";
  const char* greetingTime = "";
  volatile boolean timeAlarmSet = false;
//
// random number
  long randNumber;
//
// calendar::
//
  int newWeekStart = 0; // used to show start of next week of the month
  int monthLength = 0;
  boolean rhymeFlag = false; // used to show day rhyme screen if true
  boolean rhymeMonthFlag = false; // used to show month rhyme if true
  int startDay = 0; // Sunday's value is 0, Saturday is 6
  String week1 ="";
  String week2 ="";
  String week3 ="";
  String week4 ="";
  String week5 =""; 
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
  boolean moonName = false; // flag to show which moon screen to display
// 

/********************************************************/

void setup(void) {  
  // The buzzer will sound once with a pause then
  // three times if an SD card and DHT11  and BMP180 are present and working OK
  // 
  Serial.begin(9600);
  Wire.begin();
  //
  pinMode(buzzerPin, OUTPUT); // output
  //
  u8g.firstPage();  
    do {
      splash(); 
    } while( u8g.nextPage() );
  //  
  RTC.begin();
  // following line sets the RTC to the date & time this sketch was compiled  
  // un REM the line below to set clock, then re REM it
  // and upload this sketch again
  //RTC.adjust(DateTime(__DATE__, __TIME__));
  //
  //BP180
  // initialize device
  Serial.println(F("Initializing I2C devices..."));
  barometer.initialize();
  // verify connection
  Serial.println(F("Testing device connections..."));
  if (! RTC.isrunning()) {
   Serial.println(F("RTC is NOT running!"));
   errorHardware1 = true;    
  }
  else{
   Serial.println(F("RTC is running")); 
   errorHardware1 = false;   
  }    
  Serial.println(barometer.testConnection() ? "BMP085 connection successful" : "BMP085 connection failed");
  if(barometer.testConnection()){
    errorHardware2 = false;
  }
  else{
    errorHardware2 = true;
  }
  //
  //check presence of DHTll
  getHumidity(); // check the Humidity sensor
  if(errorHardware3 == false){
    Serial.println("DHT11 reading OK");
    buzzer();
  }
  else{
    Serial.println("There is a problem with the DHT11 Humidity Sensor");
  }
  // led used to show backup being written
  pinMode(blueLed, OUTPUT);
  digitalWrite(blueLed, HIGH); // turn on LED
  delay(500); // test the blue led
  digitalWrite(blueLed, LOW); // turn it off
  //
  buzzer();  
  // 
  // check for SD Card
  //
  pinMode(10, OUTPUT);  // needed to make the SD Library work
  pinMode(53, OUTPUT);  // hardware CS pin
  /* Initiliase the SD card */
  //Serial.println("Checking for SD Card");
  if (!SD.begin(53)){
    // If there was an error output no card is present
    Serial.println(F("No SD Card present"));
    sdPresent = false; // show NO CARD present    
  }
  else{ 
    Serial.println(F("SD Card OK")); 
   //Check if the data file exists
   if(SD.exists("data.csv")){
     Serial.println(F("data.csv exists, new data will be added to this file..."));
     sdPresent = true; // show card can be used      
   }
   else{ 
     Serial.println(F("data.csv does not exist, new file will be created..."));
     // Create a new text file on the SD card
     Serial.println(F("Creating data.csv"));
     ClockData = SD.open("data.csv", FILE_WRITE);
     // If the file was created ok then add come content
      if (ClockData){
        ClockData.println(F("Joystick Weather Clock Data"));
        ClockData.println(F("Date,00:00,01:00,02:00,03:00,04:00,05:00,06:00,07:00,08:00,09:00,10:00,11:00,12:00,13:00,14:00,15:00,16:00,17:00,18:00,19:00,20:00,21:00,22:00,23:00,Time (hrs)"));
        // Close the file
        ClockData.close();
        sdPresent = true; // show card can be used 
      }
      else{
        Serial.println(F("Failed to create file !"));
        sdPresent = false; // ignore the SD Card
        ClockData.close();        
      }
    } 
  if(sdPresent){  //  buzz 
    buzzer();   
    // look for a backup file
    if(SD.exists("backup.dat")){ //        
        Serial.println(F("Uploading Backup Data ...."));      
      
        if (!SD.exists("backup.dat")) {
          BackUp.close();
          Serial.println(F("Failed to open backup.dat"));
        }
        else{ 
          digitalWrite(blueLed, HIGH); // turn on LED to show backup being written          
          // third beep
          digitalWrite(buzzerPin, LOW); // turn on buzzer
          delay(200);
          digitalWrite(buzzerPin, HIGH); // turn buzzer off
          //           
          loadBackup();          
          digitalWrite(blueLed, LOW); // turn on LED to show backup being written
        }
       } 
     else{ // create new backup.dat and fill with blank data
       Serial.println(F("Creating backup.dat"));    
       BackUp = SD.open("backup.dat", FILE_WRITE);
       for(int f = 0; f < 6;f++){
         BackUp.print(F("0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0\r\n"));
       }
       BackUp.close();
     }
   } 
  }   
  //
  // joystick
  pinMode(2, INPUT);
  // calibrate joystick
  joyX = analogRead(A0); // midpoint for X // for pins on top
  joyY = analogRead(A1); // midpoint for Y   
  // setup Interrupt Service Routine
  attachInterrupt(0,joySwitchISR,FALLING); // uses pin 2
  //
  // get temperature and pressure
  getPressure(); // take a first reading 
  localTemp= temperature;
  localPressure = pressure; 
  //
  //
  // on board LED
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW); // turn off LED
  //  
  // get initial time and recordPointer
  DateTime now = RTC.now();  // read the RTC 
  recordPointer = now.hour(); 
  if(errorHardware1 || errorHardware2){
    Serial.println(F("One or more Hardware Errors, Clock will not start!"));
    while(1); // stop 
  }
  else{
    Serial.println(F("Clock now running ...."));
    Serial.println(F(""));
  }
  //
  displayScreen = 0; // reset to home screen 
} 
/*******************************************************/

void loop() {
  
//
/* see which screen to display**************************/
//
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
      // phase of the moon
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
       drawCalendar2(); 
     } while( u8g.nextPage() ); 
     break;      
     //
     case 11:
      // calendar display
     u8g.firstPage();      
     do {
       drawCalendar(); 
     } while( u8g.nextPage() ); 
     break; 
     //
     case 12:
      // humidity display
     greetingTemp = "Click for update";
     u8g.firstPage();      
     do {
       drawHumidity(); 
     } while( u8g.nextPage() ); 
     break;             
  } // end of switch loop
 // 
 // check if additional screens have been selected
 //
  if(displayScreen > 12){
    switch(displayScreen){
      case 13:
        // show data from 24 hours ago
        u8g.firstPage();      
        do {
          plotPressure_24(); //  pressure data from 24 hours ago
        } while( u8g.nextPage() ); 
      break;
      case 14:
        // show data from 36 hours ago
        u8g.firstPage();      
        do {
          plotPressure_48(); // pressure data from 48 hours ago
        } while( u8g.nextPage() ); 
      break; 
      // additional temperature screens
      case 15:
        // show data from 24 hours ago
        u8g.firstPage();      
        do {
          plotTemperature_24(); //  temperature data from 24 hours ago
        } while( u8g.nextPage() ); 
      break;
      case 16:
        // show data from 36 hours ago
        u8g.firstPage();      
        do {
          plotTemperature_48(); // temperature data from 48 hours ago
        } while( u8g.nextPage() );   
      break;  
      case 17:
       // name the moon for each momth
        u8g.firstPage();      
        do {
          nameMoon(); // names the full moon
        } while( u8g.nextPage() );     
      break;
      case 18:
       // day rhyme
        u8g.firstPage();      
        do {
          childDay(); // rhyme for the day
        } while( u8g.nextPage() );     
      break;    
      case 19:
       // month rhyme
        u8g.firstPage();      
        do {
          monthRhyme(); // rhyme for the month
        } while( u8g.nextPage() );     
      break;        
    
    } // end of switch loop for additional screens
  }   // end of additional screens                                      
 //
 //
 /* get data and save data*******************************/
 // take a temperature/pressure reading once only every 10 seconds
 // this should even out the display using the large fonts
  DateTime now = RTC.now();  // read the RTC 
  if(int((now.second()/10)*10) == now.second() && grabFlag == true){
    getPressure();
    localTemp= temperature;
    localPressure = pressure;
    grabFlag = false; // stop value being read more than once
  }
  if(int(((now.second()-1)/10)*10) == (now.second()-1) && grabFlag == false){ // reset the flag
    grabFlag = true;
  } 
 //
 // grab a temperature, humidity and pressure reading at the top of each hour
 //
  if(now.minute() == 0 && now.second() == 0  && doOnce == false){  
    // reset data at midnight  
    if(now.hour() == 0){ // save data and clear todays data, reset pointer at midnight
      resetDataStrings();     
    }
    //
    getHumidity(); // read the current value
    recordPointer = now.hour();
    recordDataTemp[0][recordPointer] = localTemp;
    recordDataTemp[0][24] = recordPointer; // save the pointer
    recordDataPressure[0][recordPointer] = localPressure/100; // convert to mb
    recordDataPressure[0][24] = recordPointer; // save the pointer
    recordNumber = recordNumber + 1; // increment the pointer
    //
    printData(); // build temperature, humidity and pressure data strings
    // if the card is removed it will not be recognised, press
    // the Arduino reset button to re initiate the SD Card 
    // the backup data will then be reloaded.  
    if(SD.exists("data.csv")){ 
      ClockData = SD.open("data.csv", FILE_WRITE);
      Write(); // send data to SD Card if there is one present 
      ClockData.close(); // close the file    
    }
  //}
    if(SD.exists("backup.dat")){ 
      BackUp = SD.open("backup.dat", FILE_WRITE);
      BackUp.seek(0); // rewind
      digitalWrite(blueLed, HIGH); // turn on LED to show backup being written
      saveBackup(); // now save the backup file
      digitalWrite(blueLed, LOW); // turn it off      
      BackUp.close(); // close the file
    }
    else{
      BackUp.close(); // close the file
      Serial.println(F("SD Card not present, or files missing"));   
    }
    doOnce = true; // stop the temperature being read again in second = 0
  }
  //
  if(now.minute() == 0 && now.second() > 0 && doOnce == true){ 
    doOnce = false; // reset the flag
  } 

 //* check to see if the clock alarm time has been reached ************
  if (timeAlarmSet == true) {   
    if(alarmHour == now.hour() && alarmMinute == now.minute() && now.second() < maxAlarmTime){   
      unsigned long buzzerCurrentMillis = millis();
      if(buzzerCurrentMillis - buzzerPreviousMillis >= buzzerInterval) {
        buzzerPreviousMillis = buzzerCurrentMillis;   
        if (ledState == true){
          ledState = false;
          digitalWrite(13, HIGH); 
          digitalWrite(buzzerPin, LOW);   // sound buzzer
        }
        else{
          ledState = true;
          digitalWrite(13, LOW); 
          digitalWrite(buzzerPin, HIGH);   // sound buzzer
        } 
      }
    }
  }
  // make sure the alarm switches off at the end of the alarm period
  if(alarmHour == now.hour() && alarmMinute == now.minute() && now.second() > maxAlarmTime){
    ledState = false;
    digitalWrite(buzzerPin, HIGH); // turn off buzzer  
    digitalWrite(13, LOW); // turn off LED 
  }
  
  if (timeAlarmSet == false){ // allows the alarm to switch off by pressing joystick
    ledState = false;
    digitalWrite(buzzerPin, HIGH); // turn off buzzer  
    digitalWrite(13, LOW); // turn off LED     
  }
//
/* read the joystick************************************/
//
 // reset joystick
  if(analogRead(A0) > (joyX - 150) && analogRead(A0) < (joyX + 150)){
    interrupt_time1 = millis();
    if (interrupt_time1 - last_interrupt_time1 >200) {  // debounce delay   
      xValid = true;
    }
     last_interrupt_time1 = interrupt_time1;           
  }
 // 
 // read joystick moving to right
  if(xValid == true){ // allow reading to stabilise 
  // output pins on left
  //if(analogRead(A1) > (joyX + 150)) { 
    if(analogRead(A0) < (joyX - 150)) {  // output pins on top  
      greetingHumidity = "Click to Update";
      xValid = false; // wait for the joystick to be released
      // reset displayScreen if additional screens were selected
      if(displayScreen == 12){
        humidityUpdate = false;
      }
     if(displayScreen == 13 || displayScreen == 14){
       displayScreen = 5;
     }
     if(displayScreen == 15 || displayScreen == 16){
       displayScreen = 8;
     }  
     if(displayScreen == 17){
       displayScreen = 9;
     }  
     if(displayScreen == 18){
       displayScreen = 10; 
     }  
     if(displayScreen == 19){
       displayScreen = 11; 
     }         
     //         
      displayScreen = displayScreen + 1;
      if(displayScreen > screenMax){
        displayScreen = 0;      
      }
      resetFlags();     
    }     
  }   
//
// read joystick moving to left
  if(xValid == true){
    // output pins on left
    //if(analogRead(A1) < (joyX - 150)) { 
    if(analogRead(A0) > (joyX + 150)) { // output pins on top 
      xValid = false; // wait for the joystick to be released
     // reset displayScreen if additional screens were selected
     greetingHumidity = "Click to Update";
     if(displayScreen == 13 || displayScreen == 14){
       displayScreen = 5;
     }
     if(displayScreen == 15 || displayScreen == 16){
       displayScreen = 8;
     }  
     if(displayScreen == 17){
       displayScreen = 9; 
     } 
     if(displayScreen == 18){
       displayScreen = 10; 
     }      
     if(displayScreen == 19){
       displayScreen = 11; 
     }         
     //       
     displayScreen = displayScreen - 1;
     if (displayScreen < 0 ){
       displayScreen = screenMax;
     }      
     resetFlags();
    }
  } 
} // end of the main loop

/*Screen 1 - Analog Clock*************************************/
//
void drawAnalog(void) {  // draws an analog clock face
  //
  DateTime now = RTC.now();
  //
  // Now draw the clock face
  u8g.drawCircle(64, 32, 20); // main outer circle
  u8g.drawCircle(64, 32, 2);  // small inner circle
  //
  //hour ticks
  for( int z=0; z < 360;z= z + 30 ){
  //Begin at 0° and stop at 360°
    float angle = z ;
    angle=(angle/57.29577951) ; //Convert degrees to radians
    int x2=(64+(sin(angle)*20));
    int y2=(32-(cos(angle)*20));
    int x3=(64+(sin(angle)*(20-5)));
    int y3=(32-(cos(angle)*(20-5)));
    u8g.drawLine(x2,y2,x3,y3);
  }
  // display second hand
  float angle = now.second()*6 ;
  angle=(angle/57.29577951) ; //Convert degrees to radians  
  int x3=(64+(sin(angle)*(20)));
  int y3=(32-(cos(angle)*(20)));
  u8g.drawLine(64,32,x3,y3);
  //
  // display minute hand
  angle = now.minute() * 6 ;
  angle=(angle/57.29577951) ; //Convert degrees to radians  
  x3=(64+(sin(angle)*(20-3)));
  y3=(32-(cos(angle)*(20-3)));
  u8g.drawLine(64,32,x3,y3);
  //
  // display hour hand
  angle = now.hour() * 30 + int((now.minute() / 12) * 6 )   ;
  angle=(angle/57.29577951) ; //Convert degrees to radians  
  x3=(64+(sin(angle)*(20-11)));
  y3=(32-(cos(angle)*(20-11)));
  u8g.drawLine(64,32,x3,y3);
  //
  // display greeting
  if (now.month() == birthMonth && now.day() == birthDay){
    greetingTime = "Happy Birthdy";
  }
  else{
    if (now.hour() >= 6 && now.hour() < 12){greetingTime = " Good Morning";}
    if (now.hour() >= 12 && now.hour() < 17){greetingTime = "Good Afternoon";}
    if (now.hour() >= 17 && now.hour() <= 23){greetingTime = " Good Evening";}
    if (now.hour() >= 0 && now.hour() <= 5){greetingTime = "  Sleep well";}
  }
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
  u8g.setFont(u8g_font_profont12);
  String alarmSetTime = "Alarm set for ";
  if(alarmHour <10){
    alarmSetTime = alarmSetTime + "0" + String(alarmHour);
  }
  else{
    alarmSetTime = alarmSetTime + String(alarmHour);
  }
  if(alarmMinute <10){
    alarmSetTime = alarmSetTime +  ":" + "0" + String(alarmMinute);
  }
  else{
    alarmSetTime = alarmSetTime +  ":" + String(alarmMinute);    
  }
  const char* newalarmSetTime = (const char*) alarmSetTime.c_str();  
  if(timeAlarmSet){u8g.drawStr(8,10, newalarmSetTime);} // show alarm is set   
    u8g.setFont(u8g_font_profont29); 
    DateTime now = RTC.now();   
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
    thisMonth=monthString[now.month()-1]; 
    thisDay=thisDay + thisMonth + "/"; 
    thisDay=thisDay + String(now.year() , DEC);
    const char* newDay = (const char*) thisDay.c_str(); 
    u8g.drawStr(25,60, newDay);    
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
   if(analogRead(A1) > (joyY + 300)) {       
     interrupt_time2 = millis();
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
   if(analogRead(A1) < (joyY - 300)) {   
     interrupt_time2 = millis();
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
    digitalWrite(buzzerPin, HIGH); // turn buzzer off
    digitalWrite(8, LOW); // led off   
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
      if(timerSecs >0){  
        digitalWrite(blueLed, LOW);        
      }          
      if(timerSecs >1){
        digitalWrite(buzzerPin, HIGH); // turn buzzer off       
      }      
      if (timerSecs == 60){
      digitalWrite(blueLed, HIGH);        
        timerSecs = 0;  
        timerMins = timerMins + 1;
        if(int(timerMins/10)*10 == timerMins){  // once every 10 min
          digitalWrite(buzzerPin, LOW); // turn buzzer on`
          digitalWrite(blueLed, HIGH); 
          delay(200);
          digitalWrite(buzzerPin, HIGH); // turn buzzer off`
          digitalWrite(blueLed, LOW);           
        }
      }  
    } 
    if (timerMins >= 99 && timerSecs > 0){ // beep twice when the timer reaches 99 mins
      timerSecs = 0;
      buttonFlag = false; // stop counting
      // sound buzzer twice
      digitalWrite(buzzerPin, LOW); // turn on buzzer
      digitalWrite(blueLed, HIGH);       
      delay(200);
      digitalWrite(buzzerPin, HIGH); // turn buzzer off  
      digitalWrite(blueLed, LOW);       
      delay(200); 
      digitalWrite(buzzerPin, LOW); // turn on buzzer
      digitalWrite(blueLed, HIGH);       
      delay(200);
      digitalWrite(buzzerPin, HIGH); // turn buzzer off 
      digitalWrite(blueLed, LOW);       
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
  float tempReading = 0;
  u8g.setFont(u8g_font_profont15);
  u8g.drawStr(35,10, "Pressure:");
  //
  // check value against last reading
  u8g.setFont(u8g_font_profont15); 
  if(recordDataPressure[0][recordPointer] > 0.00){
    tempReading = recordDataPressure[0][recordPointer];
    if((localPressure/100) > tempReading){ 
      u8g.drawStr(30,60, "**Rising**"); 
    }
    if((localPressure/100) < tempReading){ 
      u8g.drawStr(30,60, "**Falling**"); 
    }       
    if((localPressure/100) == tempReading){ 
      u8g.drawStr(25,60, "**Constant**"); 
    }  
    if((localPressure/100) < 900){ 
      u8g.drawStr(30,60, " **STORM**"); 
    } 
  }
  else{
    u8g.drawStr(22,60, " **Waiting**");      
  }
  if(pascal == false){  
    thisPressure = String(int(localPressure/100)) + "mb"; 
    const char* newPressure = (const char*) thisPressure.c_str();
    u8g.setFont(u8g_font_profont29);  
    u8g.drawStr(20,40, newPressure); 
  }
  else{   
    thisPressure = String(round(localPressure)) + "pa";  
    const char* newPressure = (const char*) thisPressure.c_str();
    u8g.setFont(u8g_font_fub17);  
    u8g.drawStr(15,40, newPressure); 
  }
}

/*Screen 6 - Plot Pressure*************************************/
//
void plotPressure(){
 // displays a plot of the pressure over the last 24 hours
 //  
   double tempYvalueP;    
   double tempYvalue;   
   u8g.setFont(u8g_font_5x7); 
   u8g.drawStr(30,10, "Pressure Today");
   drawPressureGraph(); // outline graph
   if(recordNumber == 0.00){
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
/*Screen 14 = Plot Pressure - 24 hours*************************************************************/
void plotPressure_24(){
 // displays a plot of the pressure from Yesterday
 // 
   double tempYvalueP;    
   double tempYvalue;   
   u8g.setFont(u8g_font_5x7); 
   u8g.drawStr(19,10, "Pressure Yesterday");
   drawPressureGraph(); // outline graph
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
/*Screen 15 = Plot Pressure = 48 hours*************************************************************/
void plotPressure_48(){
  // displays a plot of the pressure from day before Yesterday
  //  
  double tempYvalueP;    
  double tempYvalue;    
  u8g.setFont(u8g_font_5x7); 
  u8g.drawStr(16,10, "Pressure 48 hrs ago");   
  drawPressureGraph(); // outline graph
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
   getForecast(); // get new forecast 
   u8g.drawLine(0,50,128,50);    
   u8g.setFont(u8g_font_profont12); 
   u8g.drawStr(18,10, "Weather Forecast"); 
   u8g.drawLine(0,15,128,15); // draw horizontal line
   if(thisForecast == "  Waiting for Data" && longForecast == "  Waiting for Data"){    
     u8g.drawStr(3,35, "  Waiting for Data");
   }
   else{   
     if(switchForecast == false){
     // now print current forecast
       const char*newthisForecast = (const char*) thisForecast.c_str();     
       u8g.drawStr(3,35, newthisForecast);
       u8g.setFont(u8g_font_5x7);    
       u8g.drawStr(5,60, "Click for long forecast");       
     }
     else{
     // now print forecast based on last 2 hours  
       const char*newthisForecast = (const char*) thisForecast.c_str();    
       u8g.drawStr(3,35, newthisForecast);      
       u8g.setFont(u8g_font_5x7); 
       u8g.drawStr(5,60, "Click for short forecast");       
     } 
     // show direction and ammount
     String ra = "";
     if(riseAmmount > 100 || riseAmmount< -100){
       ra =  rise + String(riseAmmount/100) + "mb";
       riseAmmount = riseAmmount/100;   
     }
     else{
       ra =  rise + String(riseAmmount) + "pa";
     }
     const char*newDirection = (const char*)ra.c_str();
     u8g.setFont(u8g_font_profont12);   
     u8g.drawStr(14,47, newDirection); 
     //
   }
}
/* collect a forecast ******************************************/
// allows a forecast to be collected without printing it
void getForecast(){
  // get the forecast
  // we need at least two readings to make a forecast
  // forecast will not be available until 0100 each day 
  if(recordNumber < 2){
    thisForecast = "  Waiting for Data";
    longForecast = "  Waiting for Data"; 
  }    
  // if data available the show forecast
  else{ 
    lastPressure1 = localPressure/100; // compares earlier pressures to NOW     
    if (switchForecast == false){    
      lastPressure2 = recordDataPressure[0][recordPointer-1];
    }
    else{     
      lastPressure2 = recordDataPressure[0][recordPointer-2];    
    }
    // report pressure
    if(lastPressure2 > lastPressure1){ // pressure is falling 
      rise = "falling "; 
      if ((lastPressure2 - lastPressure1) < 3){ // falling slowly
        if (lastPressure1 < 1009){
          thisForecast = "Precipitation likely";   
        } 
        if(lastPressure1 > 1008 && lastPressure1 < 1020){
          thisForecast = "    Little change";          
         }
        if (lastPressure1 > 1019){  
          thisForecast = "   Remaining Fair";          
        }         
      }
   //   
      if((lastPressure2 - lastPressure1) > 2){ // falling more rapidly 
        if (lastPressure1 < 1009){         
          thisForecast = "   Stormy weather";           
        }
        if (lastPressure1 > 1008 && lastPressure1 <1020){        
          thisForecast = "Precipitation likely";          
        } 
        if (lastPressure1 >1019){        
          thisForecast = "    Cloudy, warmer";          
        }        
      }     
    }
    //  
    if(lastPressure1 == lastPressure2){   // pressure is constant 
      rise = "constant ";
      if (lastPressure1 < 1009) {
        thisForecast = "   Clearing, cooler";        
      }
      if(lastPressure1 >1008 and lastPressure1 <1020){
        thisForecast = " Continuing the same";        
      } 
      if(lastPressure1 > 1019){
        thisForecast = "   Remaining Fair";        
      }     
    }
    //
    if(lastPressure1 > lastPressure2){ // pressure is rising 
      rise = "rising ";  
      if (lastPressure1 < 1009) {
        thisForecast = "Clearing but cooler";        
      }
      if(lastPressure1 > 1008 and lastPressure1 <1020){
        thisForecast = " Little or no change";     
      } 
      if(lastPressure1 > 1019){
        thisForecast = "   Remaining Fair";        
      }   
    }
    lastPressure1 = lastPressure1 *100;
    lastPressure2 = lastPressure2 *100;  
    riseAmmount = lastPressure1 - lastPressure2;    
  }     
// returns thisForecast with the current forecast  
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
    thisTemperature = String(int(localTempF)) + "\260F";  // displays degree symbol 
  }
  else{
    thisTemperature = String(int(localTemp)) + "\260C"; // displays degree symbol
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
  double tempYvalueP;    
  double tempYvalue;    
  u8g.setFont(u8g_font_5x7); 
  u8g.drawStr(20,10, "Temperature Today");   
  drawTemperatureGraph();
  // now plot graph
  // x starts at 4 pixels and each hour is 5 pixels
  // y is 1.6 pixels per degree centigrade 
  // as 0,0 pixel is top left, value for y should be subtracted from 64
  if(recordNumber == 0.00){
    u8g.drawStr(19,28, "Data not available");
  } 
  else{
    for(int f = 0; f < recordPointer+1;f++){  
    // limit range to 0 - 40 C
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
/*Screen 16 - Plot Temperature - 24*******************************************************/ 
void plotTemperature_24(){
  // 
  // displays a plot of the temperature over the last 24 hours
  double tempYvalueP;    
  double tempYvalue;    
  u8g.setFont(u8g_font_5x7); 
  u8g.drawStr(25,10, "Temp. Yesterday");   
  drawTemperatureGraph();
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
/*Screen 17 - Plot Temperature  - 48 hours*******************************************************/ 
void plotTemperature_48(){
  // displays a plot of the temperature over the last 24 hours
  double tempYvalueP;    
  double tempYvalue;    
  u8g.setFont(u8g_font_5x7); 
  u8g.drawStr(23,10, "Temp. 48 hrs ago");   
  drawTemperatureGraph();
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

/*Screen 18 - Name of Moon*****************************************************/
void nameMoon(){
  // names the moon each month, selects a random version each time
  DateTime now = RTC.now();  
  String nameOfMoon ="";
  //
    // these are Celtic and Medieval attributed to Britian  
    switch(now.month()){
      case 1:
        nameOfMoon ="    Woolf Moon";     
      break;
      case 2:
        nameOfMoon ="    Storm Moon";      
      break; 
      case 3:
        nameOfMoon ="   Plough Moon";       
      break; 
      case 4:
        nameOfMoon ="    Seed Moon";     
      break; 
      case 5:
        nameOfMoon ="  Mother's Moon";      
      break; 
      case 6:
        nameOfMoon ="    Mead Moon";       
      break; 
      case 7:
        nameOfMoon ="    Herb Moon";     
      break; 
      case 8:
        nameOfMoon ="    Grain Moon";       
      break; 
      case 9:
        nameOfMoon ="   Harvest Moon";     
      break; 
      case 10:
        nameOfMoon ="    Blood Moon";       
      break; 
      case 11:
        nameOfMoon ="  Mourning Moon";      
      break; 
      case 12:
        nameOfMoon ="     Oak Moon";      
      break;    
    }
  // These are native American Indian 
  /*
    switch(now.month()){
      case 1:
        nameOfMoon ="   Winter Moon";      
      break;
      case 2:
        nameOfMoon ="   Moon of Ice";       
      break; 
      case 3:
        nameOfMoon ="    Crow Moon";      
      break; 
      case 4:
        nameOfMoon ="    Frog Moon";       
      break; 
      case 5:
        nameOfMoon ="   Flower Moon";       
      break; 
      case 6:
        nameOfMoon ="  Planting Moon";      
      break; 
      case 7:
        nameOfMoon ="    Hay Moon";       
      break; 
      case 8:
        nameOfMoon ="  Dog Days Moon";      
      break; 
      case 9:
        nameOfMoon ="   Barley Moon";        
      break; 
      case 10:
        nameOfMoon =" Blackberry Moon";      
      break; 
      case 11:
        nameOfMoon ="   Beaver Moon";       
      break; 
      case 12:
        nameOfMoon =" Long Night Moon";     
      break;    
    }
  */
  // These are Chinese Moons
  /*
    switch(now.month()){
      case 1:
        nameOfMoon ="  Holiday Moon";     
      break;
      case 2:
        nameOfMoon ="   Budding Moon";       
      break; 
      case 3:
        nameOfMoon ="  Sleeping Moon";     
      break; 
      case 4:
        nameOfMoon ="   Peony Moon";       
      break; 
      case 5:
        nameOfMoon ="   Dragon Moon";         
      break; 
      case 6:
        nameOfMoon ="   Lotus Moon";    
      break; 
      case 7:
        nameOfMoon ="   Ghost Moon";     
      break; 
      case 8:
        nameOfMoon ="   Harvest Moon";      
      break; 
      case 9:
        nameOfMoon ="   Flower Moon";         
      break; 
      case 10:
        nameOfMoon ="   Kindly Moon";       
      break; 
      case 11:
        nameOfMoon ="   White Moon";       
      break; 
      case 12:
        nameOfMoon ="   Butter Moon";        
      break;    
    }   
  */
 
  u8g.setFont(u8g_font_5x7);  
  u8g.drawStr(10,10, "The full moon is called"); 
  u8g.drawStr(15,55, "from Medieval Britian");
  //u8g.drawStr(15,55, "North American Indian");
  //u8g.drawStr(15,55, " from Chinese Verse");  
  const char*newMoonName = (const char*) nameOfMoon.c_str();
  u8g.setFont(u8g_font_profont15);   
  u8g.drawStr(5,33, newMoonName); 
}

/*Screen 11 - Show Date****************************************/
//
 void drawCalendar2(){
  // show todays date  
  DateTime now = RTC.now(); // get date
  //monthName = monthString[now.month()-1];      
  u8g.setFont(u8g_font_profont22); 
  const char* newMonthName = (const char*) monthString[now.month()-1].c_str();  
  u8g.drawStr(47,22, newMonthName);  
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
/*Screen 19 - Show day with childs attitude*****************************/  
void childDay(){
  // show the child born on day
  DateTime now = RTC.now(); // get date
  String rhymePart1 = "";
  String rhymePart2 = "";
  String rhymePart3 = ""; 
  switch(now.dayOfWeek()){ 
    case 0:
      rhymePart1 = "The child born on the";
      rhymePart2 = "Sabbath is fair wise";     
      rhymePart3 = "and good in every way";
    break;    
    case 1:
      rhymePart1 = " Monday's child is";
      rhymePart2 = "   fair of face"; 
    break;      
    case 2:
      rhymePart1 = "Tuesday's child is";
      rhymePart2 = "  full of grace"; 
    break;      
    case 3:
      rhymePart1 = "Wednesday's child is";
      rhymePart2 = "    full of woe"; 
    break;  
    case 4:
      rhymePart1 = "Thursday's child has";
      rhymePart2 = "     far to go"; 
    break;  
    case 5:
      rhymePart1 = "Friday's child works";
      rhymePart2 = " hard for a living";
    break; 
    case 6:
      rhymePart1 = "Saturday's child is";
      rhymePart2 = " loving and giving";
    break;  
  }
  u8g.setFont(u8g_font_5x7); 
  const char*newRhyme1 = (const char*) rhymePart1.c_str();
  const char*newRhyme2 = (const char*) rhymePart2.c_str();
  const char*newRhyme3 = (const char*) rhymePart3.c_str();   
  u8g.drawStr(19,60, "English Rhyme 1838"); 
  u8g.setFont(u8g_font_profont12); 
  if(now.dayOfWeek() == 0){ 
    u8g.drawStr(1,15, newRhyme1);
    u8g.drawStr(1,30, newRhyme2); 
    u8g.drawStr(1,45, newRhyme3); 
  }
  else{
    u8g.drawStr(8,20, newRhyme1);
    u8g.drawStr(8,35, newRhyme2); 
  }
} 

/*Screen 12 - Show months calendar*****************************/ 
//
 void drawCalendar(){
  // display a full month on a calendar 
  int f = 0;
  u8g.setFont(u8g_font_profont12);
  u8g.drawStr(2,9, "Su Mo Tu We Th Fr Sa"); 
  // display this month
  DateTime now = RTC.now();
  //
  // get number of days in month
  if (now.month() == 1 || now.month() == 3 || now.month() == 5 || now.month() == 7 || now.month() == 8 || now.month() == 10 || now.month() == 12){
    monthLength = 31;
  }
  else {monthLength = 30;}
  if(now.month() == 2){monthLength = 28;}
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
      if(monthLength == 28 || monthLength == 30){week1 = "                1  2";}      
      if(monthLength == 31){week1 = "31              1  2";}      
      break; 
     case 6:
      // Saturday
      if(monthLength == 28){week1 = "                   1";}
      if(monthLength == 30){week1 = "30                 1";}      
      if(monthLength == 31){week1 = "30 31              1";}       
      
      break;           
  } // end first week
  newWeekStart = (7-startDay)+1;
  const char* newWeek1 = (const char*) week1.c_str();  
  u8g.drawStr(2,19,newWeek1); 
  // display week 2
  week2 ="";
  for (f = newWeekStart; f < newWeekStart + 7; f++){
    if(f<10){
      week2 = week2 +  " " + String(f) + " ";
    }  
    else{week2 = week2 + String(f) + " ";}    
  }
  const char* newWeek2 = (const char*) week2.c_str();  
  u8g.drawStr(2,29,newWeek2); 
  // display week 3
  newWeekStart = (14-startDay)+1; 
  week3 ="";
  for (f = newWeekStart; f < newWeekStart + 7; f++){
    if(f<10){
      week3 = week3 +  " " + String(f) + " ";
    }  
    else{week3 = week3 + String(f) + " ";}    
  }
  const char* newWeek3 = (const char*) week3.c_str();  
  u8g.drawStr(2,39,newWeek3);     
  // display week 4
  newWeekStart = (21-startDay)+1; 
  week4 ="";
  for (f = newWeekStart; f < newWeekStart + 7; f++){
    if(f<10){
      week4 = week4 +  " " + String(f) + " ";
    }  
    else{week4 = week4 + String(f) + " ";}    
    }
   const char* newWeek4 = (const char*) week4.c_str();  
   u8g.drawStr(2,49,newWeek4); 
   // do we need a fifth week
   week5="";
   newWeekStart = (28-startDay)+1;   
   // is is February?
   if(newWeekStart > 28 && now.month() == 2){
   // is it a leap year
     if (now.year()==(now.year()/4)*4){ // its a leap year
       week5 = "29";
     }  
   }
   else{ // print up to 30 anyway
     if(now.month() == 2){  // its February
       for (f = newWeekStart; f < 29; f++){
         week5 = week5 + String(f) + " ";  
       }  
       // is it a leap year
       if (now.year()==(now.year()/4)*4){ // its a leap year
         week5 = week5 + "29";
       }        
     }
     else{
       for (f = newWeekStart; f < 31; f++){
         week5 = week5 + String(f) + " ";
       }
       // are there 31 days
       if (monthLength == 31 && week5.length() <7){
         week5 = week5 + "31"; 
       } 
     } 
   }
   const char* newWeek5 = (const char*) week5.c_str();  
   u8g.drawStr(2,59,newWeek5);
 } 
/*Screen 20 - Month Rhyme*************************************************************/
void monthRhyme(){
  // show the child born on day
  DateTime now = RTC.now(); // get date
  String rhymePart1 = "";
  String rhymePart2 = "";
  String rhymePart3 = ""; 
  switch(now.month()){ 
    case 1:
      rhymePart1 = " January brings the";
      rhymePart2 = "snow, Makes our feet";     
      rhymePart3 = "  and fingers glow";
    break;    
    case 2:
      rhymePart1 = " February brings the";
      rhymePart2 = "  rain, Thaws the"; 
      rhymePart3 = " frozen lake again";      
    break;      
    case 3:
      rhymePart1 = "March brings breezes";
      rhymePart2 = "loud and shrill,Stirs"; 
      rhymePart3 = "the dancing daffodil";       
    break;      
    case 4:
      rhymePart1 = "April brings primrose";
      rhymePart2 = "  sweet, Scatters"; 
      rhymePart3 = " daises at our feet";
    break;  
    case 5:
      rhymePart1 = "May brings flocks of ";
      rhymePart2 = "pretty lambs,Skipping "; 
      rhymePart3 = "by their fleecy damns";
    break;  
    case 6:
      rhymePart1 = "  June brings roses,";
      rhymePart2 = " Fills the children's"; 
      rhymePart3 = "   hand with posies";
    break; 
    case 7:
      rhymePart1 = " July brings cooling";
      rhymePart2 = "  showers, Apricots "; 
      rhymePart3 = "  and gillyflowers";
    break;  
    case 8:
      rhymePart1 = "August brings sheaves";
      rhymePart2 = " of corn, Then the"; 
      rhymePart3 = "harvest home is borne";
    break;      
    case 9:
      rhymePart1 = "Warm September brings";
      rhymePart2 = "the fruit, Sportsmen "; 
      rhymePart3 = "then begin to shoot";
    break;     
    case 10:
      rhymePart1 = "Fresh October brings";
      rhymePart2 = "pheasents, To gather"; 
      rhymePart3 = "   nuts is pleasent";
    break;     
    case 11:
      rhymePart1 = "Dull November brings";
      rhymePart2 = "the blast,Then leaves"; 
      rhymePart3 = "  are whirling fast";
    break;      
    case 12:
      rhymePart1 = "Chill December brings";
      rhymePart2 = "in sleet,Blazing fire"; 
      rhymePart3 = " and Christmas treat";
    break;      
    
  }
  u8g.setFont(u8g_font_5x7); 
  const char*newRhyme1 = (const char*) rhymePart1.c_str();
  const char*newRhyme2 = (const char*) rhymePart2.c_str();
  const char*newRhyme3 = (const char*) rhymePart3.c_str();   
  u8g.drawStr(19,60, " by Sara Coleridge"); 
  u8g.setFont(u8g_font_profont12); 
  u8g.drawStr(1,15, newRhyme1);
  u8g.drawStr(1,30, newRhyme2); 
  u8g.drawStr(1,45, newRhyme3); 
} 

/*Screen 13 - Humidity*******************************************************/
//
void drawHumidity(){
  // displays local Humidity
  if(humidityUpdate == true){
    getHumidity(); // update if the joystick was pressed
    humidityUpdate = false;
  }
  u8g.setFont(u8g_font_profont15);
  u8g.drawStr(25,10, " Humidity:");
  //
  if(errorHardware3 == false){
    thisHumidity = String(int(humidityValue)) + "%"; // displays percentage symbol
  }
  else{
    thisHumidity = "--"; // problem with reading
  }
  const char* newHumidity = (const char*) thisHumidity.c_str();
  u8g.setFont(u8g_font_profont29);    
  u8g.drawStr(35,40, newHumidity);  
  //
  u8g.setFont(u8g_font_profont15);
  u8g.drawStr(10,60, greetingHumidity);
  // now add dewpoint
  u8g.setFont(u8g_font_profont12);
  u8g.drawStr(100,20, "Dew");
  u8g.drawStr(95,30, "Point");
  thisDewPoint = String(dewPointValue) +  "\260C";
  const char* newDewPoint = (const char*) thisDewPoint.c_str();
  u8g.drawStr(100,43,  newDewPoint);
}
/*************************************************************/

//
// Sub Routines and Functions
//
void resetDataStrings(){
// move data strings down in memory and reset data string 0 to all zeros
int j = 0;
  for(j = 0; j<25;j++){
    recordDataTemp[2][j] = recordDataTemp[1][j];
    recordDataTemp[1][j] = recordDataTemp[0][j];
    recordDataTemp[0][j] = 0.00; 
  }
//
  for(j = 0; j<25;j++){
    recordDataPressure[2][j] = recordDataPressure[1][j];
    recordDataPressure[1][j] = recordDataPressure[0][j];
    recordDataPressure[0][j] = 0.00;    
  } 
//
  recordNumber = 0; // reset number of available data points
  }
/**************************************************************/
 void printData(){
 //build temperature and pressure data strings
   DateTime now = RTC.now();
   SDtemperature = "";
   SDpressure = "";
   for(int f = 0; f<25;f++){
     SDtemperature = SDtemperature + String(recordDataTemp[0][f]);
     if(f <24){
       SDtemperature = SDtemperature + ","; // dont put a comma after the last value
     }
     SDpressure = SDpressure + String(recordDataPressure[0][f]); 
     if(f <24){
       SDpressure = SDpressure + ","; // dont pur a comma after the last value
     }     
   }    
 }
/**************************************************************/ 
void joySwitchISR(){
  // includes a debounce routine
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  //Serial.println(displayScreen); // Un REM to debug
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
    if(displayScreen == 5 || displayScreen  == 13 || displayScreen == 14){
      // button pressed on plot screens
      showData = showData + 1;
      if(showData > 3){
        showData = 1;
      }
      if(showData ==1){
        displayScreen =5;
      }
      if(showData ==2){
        displayScreen =13;
      }
       if(showData ==3){
        displayScreen =14;
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
    if(displayScreen == 8 || displayScreen  == 15 || displayScreen == 16){
      // button pressed on plot screens
      showData = showData + 1;
      if(showData > 3){
        showData = 1;
      }
      if(showData == 1){
        displayScreen = 8;
      }
      if(showData == 2){
        displayScreen = 15;
      }
       if(showData == 3){
        displayScreen = 16;
      }     
    }
    if(displayScreen == 4){
      pascal = !pascal;
    }
    if(displayScreen == 9 || displayScreen  == 17 ){
      // button pressed on moon screen
      moonName = !moonName;
      if(moonName){
        displayScreen = 17; // name of moon screen
      }
      else{
        displayScreen = 9;
      }
    } 
    if(displayScreen == 10 || displayScreen  == 18 ){
      // button pressed on date screen
      rhymeFlag = !rhymeFlag;
      if(rhymeFlag){
        displayScreen = 18; // day rhyme
      }
      else{
        displayScreen = 10;
      }
    } 

    if(displayScreen == 11 || displayScreen  == 19 ){
      // button pressed on calendar screen
      rhymeMonthFlag = !rhymeMonthFlag;
      if(rhymeMonthFlag){
        displayScreen = 19; // month rhyme
      }
      else{
        displayScreen = 11;
      }
    }

    if(displayScreen == 12){
      // button pressed on humidity screen
      humidityUpdate = true;
      greetingHumidity = "Reading updated";
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
void getHumidity(){
  // reads the DHT11, checks thst Sensor reading is OK. If not then returns errorHardware3 = true
   checkDHT11 = DHT11.read(DHT11PIN);
   humidityValue = 0; // rest value
   dewPointValue = 0;
   switch (checkDHT11)
   {
     case 0: errorHardware3 = false; break;
     case -1: errorHardware3 = true; break;
     case -2: errorHardware3 = true; break;
     default: errorHardware3 = true; break;
   }
   if(errorHardware3 == false){
    humidityValue = DHT11.humidity; // only return a value if reading was good
    dewPointValue = DHT11.dewPoint();
   }
}

/********************************************************/
//
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
void resetFlags(){
  // reset flags when joystick moves left or right
  showData = 1; // reset the plot screens when we move away
  showC = true; // reset to show Centigrade when we move away  
  buttonFlag == false; //stop timers working when we leave screen 
  switchForecast = false; // reset to show current forecast 
  moonName = false; // reset the moon screen to graphic display
  rhymeFlag = false; // reset to page calendar 
  rhymeMonthFlag = false; // reset to calendar
  pascal = false; // show mb if false   
}
/********************************************************/
void Write(){
  // data will be saved as a CSV file to enable it to be read into excel
  DateTime now = RTC.now();  
  // now append new data file 
  ClockData.println(String(now.year()) + "/" + String(now.month()) + "/" + String(now.day()));
  // now add this hours temperature and pressure strings
  ClockData.println("Temp(C)," + SDtemperature); // moves the data across to column 2
  ClockData.println("Press(mb)," + SDpressure);     
}
/********************************************************/
size_t readField(File* file, char* str, size_t size, char* delim) {
  char ch;
  size_t n = 0;
  while ((n + 1) < size && file->read(&ch, 1) == 1) {
    // Delete CR. 
    if (ch == '\r') {
      continue;
    }
    str[n++] = ch;
    if (strchr(delim, ch)) {
        break;
    }
  }
  str[n] = '\0'; 
  return n;
}
/*********************************************************/
//
 void loadBackup(){ 
   //float array[6][25];// Array for data.
   BackUp = SD.open("backup.dat", FILE_WRITE);
   BackUp.seek(0); // rewind for read  
   int p = 0;     // First array index.
   int q = 0;     // Second array index
   size_t n;      // Length of returned field with delimiter.
   char str[30];  // Must hold longest field with delimiter and zero byte.
   char *ptr;     // Test for valid field. 
   DateTime now = RTC.now();  // read the RTC 
   recordPointer = now.hour(); 
   // Read the file and store the data.
   for (p = 0; p < 6; p++) {
     for (q = 0; q < 25; q++) { 
        n = readField(&BackUp, str, sizeof(str), ",\n");   
        if (n == 0) {
          Serial.println(F("Too few lines"));
        } 
        if(p > 2){
          recordDataPressure[p-3][q] = (strtol(str, &ptr, 10));
          recordDataPressure[p-3][q] = recordDataPressure[p-3][q]/100;  // divide by 100 to get mb
        }
        else{
          recordDataTemp[p][q] = strtol(str, &ptr, 10);        
        }          
      }
    }
    // get the number of data points recorded today
    recordNumber = 0;
    for(p = 0; p <23; p++){
      if(recordDataPressure[0][p] != 0.00){
        recordNumber = recordNumber + 1;
      }
    }
    BackUp.close();
  }
//
/********************************************************/
void saveBackup(){
  // save this hours backup data to backup.dat 
  int j =0;
  int f = 0;
  SDtemperature = "";
  SDpressure = "";
  // save the temperature data
  for(j = 0; j < 3; j++){
    for(f = 0; f < 25; f++){
      SDtemperature = SDtemperature + String(recordDataTemp[j][f]);     
      if(f < 24){
        SDtemperature = SDtemperature + ",";
      }  
    } 
    SDtemperature = SDtemperature + "\r\n";
    BackUp.print(SDtemperature); // print first line  
    SDtemperature = ""; // reset string
  }
   // now save pressure data
  for(j = 0; j < 3; j++){
    for(f = 0; f < 25; f++){
      SDpressure = SDpressure + String((recordDataPressure[j][f]) * 100);     
      if(f < 24){
        SDpressure = SDpressure + ",";
      }   
    } 
    SDpressure = SDpressure + "\r\n";
    BackUp.print(SDpressure); // print first line 
    SDpressure = ""; // reset string
  }  
}
/**********************************************************/
void drawPressureGraph(){
  // draws skeleton graph
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
}
/**********************************************************/
void drawTemperatureGraph(){
  // draws skeleton graph
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
}
/**********************************************************/
void buzzer(){
  // sounds buzzer
  digitalWrite(buzzerPin, LOW);  // turn the buzzer ON briefly
  delay(200);
  digitalWrite(buzzerPin, HIGH); // turn buzzer off
  delay(50);
}
/**********************************************************/


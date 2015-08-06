/***************************************************************
Simple instructions for joystick clock
Chris Rouse Aug 2015

***************************************************************/

void setup() {
 Serial.begin(9600);

}

void loop() {
// Print instructions on the Serial Monitor
  Serial.println("Joystick Weather Clock");
  Serial.println("Chris Rouse July 2015");
  Serial.println("***********************************");  
  Serial.println("");
  Serial.println("This clock uses a joystick to navigate through");
  Serial.println("16 screens. Moving the joystick to the right (X direction)");
  Serial.println("will display the screens in the following order:");
  Serial.println("");
  Serial.println("1. Analog Clock - Displays a greeting, and Alarm state");
  Serial.println("   Press the joystick switch to set or cancel the alarm (state 'toggles'");
  Serial.println("");
  Serial.println("2. Digital Clock - Displays Hours and minutes.");
  Serial.println("   Date (in UK format) and a warning if the alarm is set"); 
  Serial.println("   Press the joystick switch to set or cancel the alarm (state 'toggles'");  
  Serial.println("");
  Serial.println("3. Set Alarm time - Hours and minus shown, with minutes underlined");
  Serial.println("   moving the joystick up will increase the minute, hold it up and");
  Serial.println("   the minutes will increase automatically.Moving the jostick");
  Serial.println("   downwards will decrease the minutes. Once you have the minutes");
  Serial.println("   set press the joystick and the hours will be underlined. Set the");
  Serial.println("   hours in the same way as the minutes (24 hour clock)"); 
  Serial.println("");
  Serial.println("4. Event timer. - Press the joystick to Start/Stop, Counts up to 99 mins");
  Serial.println("");
  Serial.println("5. Pressure display in mb. - Indicates if pressure change direction");
  Serial.println("");
  Serial.println("6. Plot of pressure over 24 hours. - This display is reset at midnight");
  Serial.println("   The large tick at the bottom of the screen shows mid day");
  Serial.println("   Pressure data is collected every hour and 36 hours of data");
  Serial.println("   is stored. This earlier data can be seen by pressing the joystick");
  Serial.println("   Pressing cycles through 3 screens, today, yesterday and the day before");
  Serial.println("   (Data is plotted after 2 pressure readings have been taken)");
  Serial.println("");
  Serial.println("7. Weather Forecast - Uses rate of change and direction of pressure change");
  Serial.println("   to estimate the local weather for the next few hours. Based on the");
  Serial.println("   traditional barometer display");  
  Serial.println("");
  Serial.println("8. Temperature - display in degrees Celcius plus a greeting");
  Serial.println("");
  Serial.println("9. Temperature plot for 24 hours. Identical operation to the Pressure plot");
  Serial.println("");
  Serial.println("10. Moon Phase - shows the phase of the moon and days to next full moon");
  Serial.println("");
  Serial.println("11.Calendar display - single day and month");
  Serial.println("");
  Serial.println("12.Calendar - for current month");
  Serial.println("");  
  Serial.println("The next  screen is back to the home screen.");
  Serial.println("");
  Serial.println("Notes:");
  Serial.println("");  
  Serial.println("Data is collected every hour and stored in memory, if power is removed");
  Serial.println("the data will be lost. The data could be stored on an SD Card");
  Serial.println("but this has not been implimented yet");
  Serial.println("The Weather Forecast is a bit like using seaweed to forecast.");
  Serial.println("Its a rough and ready guide and may need modifying outside of the");
  Serial.println("North East Alantic area (Europe)"); 
  Serial.println("");
  Serial.println("The calendar should be good for a few years to come");
  Serial.println("");
  Serial.println("The last two data readings are used to obtain the direction");
  Serial.println("that the pressure is moving in. As data is collected at");
  Serial.println("the 'top of the hour', the forecast and pressure direction can");
  Serial.println("be up to an hour out");
  Serial.println("");
  Serial.println("The Alarm uses a small Active Buzzer to provide a 1 second sound pulse");
  Serial.println("This is set to last for 10 seconds, but this time and the");
  Serial.println("default alarm time can be changed, look for them in the top of");
  Serial.println("the code before the setup loop."); 
  Serial.println("");
  Serial.println("The contents of the data strings are sent to the Serial Monitor once an hour");
  Serial.println("Be aware that if you wish to see this display then switch the ");
  Serial.println("Serial Monitor on as soon as the clock starts. If the Serial Monitor");
  Serial.println("is started some time later all the stored data will be lost!");  
  // Do nothing
  while(1);
}



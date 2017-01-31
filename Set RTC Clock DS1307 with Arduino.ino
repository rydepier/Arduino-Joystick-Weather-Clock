// Reset the RTC to the time on the PC
// Credits to the Original Author

#include <Wire.h>
#include "RTClib.h"

RTC_DS1307 RTC;

char monthString[37]= {"JanFebMarAprMayJunJulAugSepOctNovDec"};
int  monthIndex[122] ={0,3,6,9,12,15,18,21,24,27,30,33};
char monthName[3]="";
//
void setup () {
    Serial.begin(9600);
    Wire.begin();
    RTC.begin();

    Serial.println("setting RTC to PC time");
    // following line sets the RTC to the date & time this sketch was compiled
    RTC.adjust(DateTime(__DATE__, __TIME__));
  
}
//
void loop () {
    DateTime now = RTC.now();
    
    Serial.print(now.day(), DEC);
    Serial.print('/');
    for (int i=0; i<=2; i++){
    monthName[i]=monthString[monthIndex[now.month()-1]+i];
    }
    Serial.print(monthName);
    Serial.print('/');
    Serial.print(now.year(), DEC);   
    Serial.print(' ');
    Serial.print(now.hour(), DEC);
    printDigits(now.minute());
    printDigits(now.second());
    Serial.println();
    delay(1000);
}
void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}
 

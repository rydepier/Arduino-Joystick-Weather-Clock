/*************************************************
Joystick Test
prints the output from the joystick on the Serial Monitor

Connections:
Gnd on Joystick to Gnd on Arduino
Vcc on Joytick to 5 volts on Arduino
VrX on Joystick to A0 on Arduino
VrY on Joystick to A1 on Arduino
Sw on Joystick to pin 2 on Arduino

*************************************************/
// definitions for pins
# define xPin A0
# define yPin A1
# define joySwitch 2
#define ledPin 13

// variables
boolean switchState = true; // it will be false when pressed
int xValue = 0;
int yValue = 0;

void setup(void){
Serial.begin(9600);
pinMode(joySwitch, INPUT_PULLUP); // turn on pullup resistor
pinMode(ledPin, OUTPUT); // onboard LED
}

void loop (){

// main code will go here and joyStick() is called as a subroutine

joyStick(); // call joystick routine

}

void joyStick(){
// this routine reads the joystick values
xValue = analogRead(xPin);
yValue = analogRead(yPin);
switchState = digitalRead(joySwitch);
// remove following lines if not required
Serial.print("X value = ");
Serial.println(xValue);
Serial.print("Y value = ");
Serial.println(yValue);
Serial.print("Switch state = ");
Serial.println(switchState);
if (switchState == false){ // switch pressed
 digitalWrite(ledPin, HIGH);
}
else{
  digitalWrite(ledPin, LOW);
}
//
delay(1000); // delay to allow data to be seen
}

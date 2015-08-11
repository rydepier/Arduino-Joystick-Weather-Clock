# Arduino-Joystick-Weather-Clock
This clock has 16 screens selected using a joystick. Everything from weather forecast to moon phase on OLED screen. The project requires an Arduino Mega, a real time clock DS1307, a pressure sensor BMP180 a buzzer and a joysticj with switch.

The main code contains connection details for the various sensors. There is a RTC setting sketch and some simple instructions in a sketch. The full project is shown on my blog at  https://rydepier.wordpress.com/2015/07/30/joystick-weather-clock-using-oled-display/ . The moon phase calculator will work for any date after 1972, but in this sketch it is used to calculate the date of the last full moon, then display a simple graphic and description for the moon phase today. The Calendardisplays the current date and a one month calendar in standard format.

Pressure and temperature are plotted over a 24 hour period with 3 days worth of data stored and each 24 hour period can be displayed.

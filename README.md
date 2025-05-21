# Jam Up
![Screenshot 2025-05-21 at 06 54 50](https://github.com/user-attachments/assets/351e3fa3-b109-4108-bf25-42cbb54b13f7)

## A ESP8266 based MP3 Player that has the following:

#### 1) _A DFPlayer Mini Module;_
#### 2) _A 0.98" OLED screen communicating via I2C on 0x3C;_
#### 3) _A real time clock (RTC DS1307) communicating via I2C on 0x68;_
#### 4) _A 3.7V LiPo battery;_
#### 5) _A 3.7V LiPo battery charger;_
#### 6) _A step-up 3V to 5V 1A;_
#### 7) _A GY-521 MPU-6050 3 Axis Gyroscope and Accelerometer communicating via I2C on 0x69;_
#### 8) _A 3V buzzer;_
#### 9) _4 tack buttons;_
#### 10) _2 switch buttons;_
#### 11) _An LC709203F LiPoly / LiIon Fuel Gauge and Battery Monitor communicating via I2C on  0x0B;_
#### 12) _Output for PWM LEDs, with three pins (GND, DATA, VCC);_

## <ins>**The functionality includes the following:**</ins>

### * The first screen displays

#### 1) The clock at the top, with the format hh:mm:ss AM/PM.
   * This is a sample of that line: "_8:09:25 AM_";

#### 2) The second line shows the day of the week and the date.
   * This is a sample of that line: "_Wed, 05/21/2025_";

#### 3) The third line shows the mp3 player (DFPlayer) status, which is initially idle.
   * This is a sample of that line at the time of power up: "_PLAYER: Idle_";

#### 4) The fourth line displays the battery level, as it is being read from LC709203F.
   * This is a sample of that line: "_Bat: 55.60% 3.95V_";

#include "DFRobotDFPlayerMini.h"
#include <SPI.h>
#include <Wire.h>

/*
 * FUEL GAUGE
 */
#include "Adafruit_LC709203F.h"
Adafruit_LC709203F fuelGauge;
bool lowPowerAlertFlag = false;
double batteryVoltage, batteryStateOfCharge;

#if !defined(UBRR1H)
#include <SoftwareSerial.h>
SoftwareSerial softwareSerial(D7, D8); // RX, TX
#endif

/*
 * RTC
 */
#include "RTClib.h"
#define DS3231_I2C_ADDRESS 0x68
RTC_DS1307 rtc;
char daysOfTheWeek[7][12] = {"Sun","Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
DateTime currentDateTime;
String ampmMode = "AM";

/*
 * LEDS
 */
#include <FastLED.h>
FASTLED_USING_NAMESPACE
#define LED_TYPE    WS2812
#define COLOR_ORDER GRB
#define NUM_LEDS    10
CRGB leds[NUM_LEDS];
#define BRIGHTNESS          96
#define FRAMES_PER_SECOND  120
uint8_t gHue = 0; // rotating "base color" used by many of the patterns

/*
 * PINS
 */
#define BUTTON_PLAY_PAUSE   D3   // Play_Pause Button pin - ESP8266
#define BUTTON_NEXT         D5   // Next Button pin - ESP8266
#define BUTTON_VOLUME_UP    A0   // VolUp Button pin - ESP8266
#define BUTTON_VOLUME_DOWN  D6   // VolDown Button pin - ESP8266
#define BUSY_STATUS_PIN     D4   // BUSY pin - DFPLAYER
#define STATUS_LEDS         D0   // LEDS pin - ESP8266

/*
 * PINS STATUS
 */
int buttonPlayPauseStatus = 0;
int buttonNextStatus = 0;
int buttonVolumeUpStatus = 0;
int buttonVolumeDownStatus = 0;
int busyStatus = 0;

/*
 * OLED DISPLAY
 */
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1); // SSD1306 display connected to I2C (SDA, SCL pins)

int displayMode = 0;
/*
  0 - Shows the Time and Date
  1 - Shows file details
*/
String currentCommand = "";
int playPauseCommand = 0;
int nextCommand = 0;
int volumeUpCommand = 0;
int volumeDownCommand = 0;
int currentVolumeLevel = 0;

bool isPlaying = false;
bool isPaused = false;
bool refreshScreen = false;
int currentTrack = 1;
int tracksCount = 0;

#define TOTAL_LINES  7
String lines[TOTAL_LINES] = {"", "", "", "", "", "", ""};
String oldLines[TOTAL_LINES] = {"", "", "", "", "", "", ""};
int chosenSerial = 1;
/*
 * LINE 0 - Status: PLAYING or PAUSED or STOPPED
 * LINE 1 - Filename
 * LINE 2 - Volume Level: XX
 * LINE 3 - Total Files: XX
 * LINE 4 - Command: PLAY or PAUSE or NEXT or PREVIOUS or VOLUME_UP or VOLUME_DOWN
 * LINE 5 - Serial Info: XX\
 */
 
DFRobotDFPlayerMini myDFPlayer;

void setup() {
  initButtons();
  initLeds();
  initFuelGauge();
  initRtc();
  
  Serial.begin(115200);

  #if !defined(UBRR1H)
    softwareSerial.begin(9600);
    myDFPlayer.begin(softwareSerial, true);
    chosenSerial = 1;
  #else
    Serial1.begin(9600);
    myDFPlayer.begin(Serial1, true);
    chosenSerial = 2;
  #endif
  
  delay(500);
  initDisplay();

  if (chosenSerial == 1) {
    displayText(5, "Comm:  ", "softwareSerial", 1);
  } else {
    displayText(5, "Comm:  ", "Serial1", 1);
  }
  //displayText(0, "Status ", "READY", 1);

  //myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD);
  //myDFPlayer.volume(10);//Setting initial volume to 10. Ranges from 0 to 30


  //myDFPlayer.setTimeOut(500); //Set serial communictaion time out 500ms
  //myDFPlayer.EQ(DFPLAYER_EQ_NORMAL); // DFPLAYER_EQ_NORMAL or DFPLAYER_EQ_POP or DFPLAYER_EQ_ROCK or DFPLAYER_EQ_JAZZ or DFPLAYER_EQ_CLASSIC or DFPLAYER_EQ_BASS

  delay(100);
  //displayFileStatus();
  delay(100);
}

void loop() {
  checkButtonStatus();
  //ledsRainbow();
  displayFuelGauge();
  getRtcData();

  switch(displayMode) {
    case 0: displayClock();
            break;
    case 1: displayFileStatus();
            break;
  }
}

void initFuelGauge() {
  Wire.begin();

   // For the Feather ESP32-S2, we need to enable I2C power first! This section can be deleted for other boards
  #if defined(ARDUINO_ADAFRUIT_FEATHER_ESP32S2)
    // turn on the I2C power by setting pin to opposite of 'rest state'
    pinMode(PIN_I2C_POWER, INPUT);
    delay(1);
    bool polarity = digitalRead(PIN_I2C_POWER);
    pinMode(PIN_I2C_POWER, OUTPUT);
    digitalWrite(PIN_I2C_POWER, !polarity);
  #endif

  if (!fuelGauge.begin()) {
    Serial.println(F("Couldnt find Adafruit LC709203F?\nMake sure a battery is plugged in!"));
  }
  Serial.println(F("Found LC709203F"));
  Serial.print("Version: 0x"); Serial.println(fuelGauge.getICversion(), HEX);

  //LC709203F_APA_100MAH, LC709203F_APA_200MAH, LC709203F_APA_500MAH, LC709203F_APA_1000MAH, LC709203F_APA_2000MAH, LC709203F_APA_3000MAH
  fuelGauge.setPackSize(LC709203F_APA_1000MAH);

  fuelGauge.setAlarmVoltage(3.8);
}

void initLeds() {
  FastLED.addLeds<LED_TYPE, STATUS_LEDS, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
}

void ledsRainbow() {
  EVERY_N_MILLISECONDS( 20 ) { gHue++; }
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, gHue, 7);
  FastLED.show();
}

void initButtons() {
  pinMode(BUTTON_PLAY_PAUSE, INPUT);
  pinMode(BUTTON_NEXT, INPUT);
  pinMode(BUTTON_VOLUME_UP, INPUT);
  pinMode(BUTTON_VOLUME_DOWN, INPUT);
  pinMode(BUSY_STATUS_PIN, INPUT);
  //pinMode(STATUS_LEDS, OUTPUT);

  buttonPlayPauseStatus = digitalRead(BUTTON_PLAY_PAUSE);
  buttonNextStatus = digitalRead(BUTTON_NEXT);
  buttonVolumeUpStatus = digitalRead(BUTTON_VOLUME_UP);
  buttonVolumeDownStatus = digitalRead(BUTTON_VOLUME_DOWN);
}

void initRtc() {
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
  } else {
    Serial.println("RTC Initialized");
  }
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // <----------------------SET TIME AND DATE: YYYY,MM,DD,HH,MM,SS
    //rtc.adjust(DateTime(2016, 11, 19, 19, 45, 0));
  } else {
    Serial.println("RTC is running.");
  }
}

void checkButtonStatus() {
  //PRESSING ONLY THE PLAY/PAUSE BUTTON
  if (buttonPlayPauseStatus != digitalRead(BUTTON_PLAY_PAUSE)) {
    buttonPlayPauseStatus = digitalRead(BUTTON_PLAY_PAUSE);
    if (buttonPlayPauseStatus == LOW) {
      displayText(5, "BUTTON_PLAY_PAUSE", " ", 1);
      if (isPlaying) {
        myDFPlayer.pause();
        isPlaying = false;
        isPaused = true;
        displayText(0, "Status: ", "PAUSED", 1);
      } else {
        if (isPaused) {
          myDFPlayer.start();
          displayText(0, "Status: ", "RESUME PLAYING", 1);
          isPaused = false;
        } else {
          myDFPlayer.play(currentTrack);
          displayText(0, "Status: ", "START PLAYING", 1);
        }
        isPlaying = true;
      }
  
      playPauseCommand = 1;
    }
  }

  //PRESSING ONLY THE NEXT BUTTON
  if (buttonNextStatus != digitalRead(BUTTON_NEXT)) {
    buttonNextStatus = digitalRead(BUTTON_NEXT);
    if (buttonNextStatus == LOW) {
      displayText(5, "BUTTON_NEXT", " ", 1);
      myDFPlayer.next();
      myDFPlayer.start();
      isPaused = false;
      displayText(4, "Command: ", "NEXT", 1);
      displayFileStatus();
      nextCommand = 1;
    }
  }

  //PRESSING VOLUME DOWN AND VOLUME UP AT THE SAME TIME
  if (buttonVolumeUpStatus != analogRead(BUTTON_VOLUME_UP) && //VOLUME UP USES THE ANALOG PIN "A0", SO YOU MUST USE "analogRead"
      buttonVolumeDownStatus != digitalRead(BUTTON_VOLUME_DOWN)) {
    buttonVolumeUpStatus = analogRead(BUTTON_VOLUME_UP); //VOLUME UP USES THE ANALOG PIN "A0", SO YOU MUST USE "analogRead"
    buttonVolumeDownStatus = digitalRead(BUTTON_VOLUME_DOWN);
    if (buttonVolumeUpStatus < 200 && //VOLUME UP USES THE ANALOG PIN "A0", SO YOU MUST COMPARE TO AN ANALOG VALUE
        buttonVolumeDownStatus == LOW) {
      if (displayMode == 0) {
        displayMode = 1;
      } else {
        displayMode = 0;        
      }
      return;
    }
  }

  //PRESSING ONLY THE VOLUME DOWN BUTTON
  if (buttonVolumeDownStatus != digitalRead(BUTTON_VOLUME_DOWN)) {
    buttonVolumeDownStatus = digitalRead(BUTTON_VOLUME_DOWN);
    if (buttonVolumeDownStatus == LOW) {
      displayText(5, "BUTTON_VOLUME_DOWN", " ", 1);
      myDFPlayer.volumeDown();
      currentVolumeLevel = myDFPlayer.readVolume();
      displayText(3, "Volume Level: ", String(currentVolumeLevel), 1);
      displayText(4, "Command: ", "VOL DOWN", 1);
      volumeDownCommand = 1;
    }
  }

//PRESSING ONLY THE VOLUME UP BUTTON  
  if (buttonVolumeUpStatus != analogRead(BUTTON_VOLUME_UP)) { //VOLUME UP USES THE ANALOG PIN "A0", SO YOU MUST USE "analogRead"
    buttonVolumeUpStatus = analogRead(BUTTON_VOLUME_UP); //VOLUME UP USES THE ANALOG PIN "A0", SO YOU MUST USE "analogRead"
    if (buttonVolumeUpStatus < 200) { //VOLUME UP USES THE ANALOG PIN "A0", SO YOU MUST COMPARE TO AN ANALOG VALUE
      displayText(5, "BUTTON_VOLUME_UP", " ", 1);
      myDFPlayer.volumeUp();
      currentVolumeLevel = myDFPlayer.readVolume();
      displayText(3, "Volume Level: ", String(currentVolumeLevel), 1);
      displayText(4, "Command: ", "VOL UP", 1);
      //displayText(4, "VALUE: ", String(buttonVolumeUpStatus), 1);
      volumeUpCommand = 1;
    }
  }

  //CHECKING THE DFPLAYER'S BUSY STATUS PIN
  if (busyStatus != digitalRead(BUSY_STATUS_PIN)) {
    busyStatus = digitalRead(BUSY_STATUS_PIN);
    if (busyStatus == LOW) {
      displayText(5, "PLAYER: ", "Busy", 1);
    } else {
      displayText(5, "PLAYER: ", "Idle", 1);
    }
  }
}

String getPlayerDetail(uint8_t type, int value){
  switch (type) {
    case TimeOut:
      return "Time Out!";
      break;
    case WrongStack:
      return "Stack Wrong!";
      break;
    case DFPlayerCardInserted:
      return "Card Inserted!";
      break;
    case DFPlayerCardRemoved:
      return "Card Removed!";
      break;
    case DFPlayerCardOnline:
      return "Card Online!";
      break;
    case DFPlayerPlayFinished:
      return "Play Finished!";
      break;
    case DFPlayerError:
      switch (value) {
        case Busy:
          return "Error - Card not found";
          break;
        case Sleeping:
          return "Sleeping";
          break;
        case SerialWrongStack:
          return "Get Wrong Stack";
          break;
        case CheckSumNotMatch:
          return "Check Sum Not Match";
          break;
        case FileIndexOut:
          return "File Index Out of Bound";
          break;
        case FileMismatch:
          return "Cannot Find File";
          break;
        case Advertise:
          return "In Advertise";
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

void updateTrackData() {
  currentTrack = myDFPlayer.readCurrentFileNumber();
  tracksCount = myDFPlayer.readFileCounts();
}

void initDisplay() {
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64, 0x3C for 128x32
  } else {
    Serial.println("SSD1306 allocation success!");
  }
  display.display();
  delay(1000);
  display.clearDisplay();
  display.display();
  delay(1000);
}

void displayText(int targetLine, String leftValue, String rightValue, int fontSize) {
  currentCommand = rightValue;
  leftValue.concat(rightValue);
  lines[targetLine] = leftValue;

  refreshScreen = false;
  for (int i =0; i < TOTAL_LINES; i++) {
    if (oldLines[i] != lines[i]) {
      refreshScreen = true;
    }
  }

  if (refreshScreen) {
    Serial.println(leftValue);
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    
    for (int j =0; j < TOTAL_LINES; j++) {
      if (j == 0) {
        if (fontSize == 99) {
          display.setTextSize(1);
          display.setCursor(105, 2);
          display.println(ampmMode);
        }
        display.setTextSize(2);
      } else {
        display.setTextSize(1);
      }
      display.setCursor(0, j * 9);
      display.print(lines[j]);
      oldLines[j] = lines[j];
    }

    display.display();
    delay(100);
  }
}

void displayClock() {
  displayText(0, getRtcTime(), " ", 99);
  displayText(2, getRtcDayOfWeek(), getRtcDate(), 1);
  //displayText(3, "Day: ", getRtcDayOfWeek(), 1);  
}

void displayFileStatus() {
  displayText(3, String(currentTrack), " ", 1);
  displayText(4, "Total Files: ", String(tracksCount), 1);
}

void displayFuelGauge() {
  getFuelGaugeData();

  Serial.print("Battery percentage: ");
  Serial.print(batteryStateOfCharge);
  Serial.println("%");
  //displayText(3, "Bat. %: ", String(batteryStateOfCharge), 1);

  Serial.print("Battery voltage: ");
  Serial.print(batteryVoltage);
  Serial.println("V");
  //displayText(4, "Bat. V: ", String(batteryVoltage), 1);
  displayText(6, "Bat: ", String(batteryStateOfCharge) + "% " + String(batteryVoltage) + "V", 1);
}

void getFuelGaugeData() {
  batteryVoltage = fuelGauge.cellVoltage();
  batteryStateOfCharge = fuelGauge.cellPercent();
  lowPowerAlertFlag = 0;//lipo.getAlert();
  
  if (lowPowerAlertFlag) {
    //Serial.println("Beware, Low Power!");
    lowPowerAlertFlag = false;
  }
}

void getRtcData() {
  currentDateTime = rtc.now();
  /*
    YEAR = currentDateTime.year()
    MONTH = currentDateTime.month()
    DAY = currentDateTime.day()
    DAY OF THE WEEK = daysOfTheWeek[currentDateTime.dayOfTheWeek()]
    HOUR = currentDateTime.hour()
    MINUTE = currentDateTime.minute()
    SECOND = currentDateTime.second()
  */
}

char * getRtcDate() {
  static char rtcDateOutput[25] = "";
  memset(rtcDateOutput, 0, sizeof(rtcDateOutput));
  
  strcat(rtcDateOutput, ", ");
  char buffer[2];
  sprintf(buffer, "%02d", currentDateTime.month());
  strcat(rtcDateOutput, buffer);
  strcat(rtcDateOutput, "/");
  sprintf(buffer, "%02d", currentDateTime.day());
  strcat(rtcDateOutput, buffer);
  strcat(rtcDateOutput, "/");
  sprintf(buffer, "%02d", currentDateTime.year());
  strcat(rtcDateOutput, buffer);
  return rtcDateOutput;
}

char * getRtcTime() {
  getRtcData();
  static char rtcTimeOutput[25] = "";
  memset(rtcTimeOutput, 0, sizeof(rtcTimeOutput));
  
  int hourValue = currentDateTime.hour();
  char ampm = 'A';
  if (hourValue > 11) {
      ampm = 'P';
   }
  if (hourValue > 12) {
    hourValue = hourValue - 12;
  }
  char buffer[8];
  sprintf(buffer, "%d:%02d:%02d", hourValue, currentDateTime.minute(), currentDateTime.second());
  strcat(rtcTimeOutput, buffer);

  ampmMode = "";
  ampmMode += ampm;
  ampmMode += "M";
  return rtcTimeOutput;
}

char * getRtcDayOfWeek() {
  return daysOfTheWeek[currentDateTime.dayOfTheWeek()];
}
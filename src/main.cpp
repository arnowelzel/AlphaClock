/*
 * AlphaClock 1.0
 * 
 * Copyright 2021 Arno Welzel / https://arnowelzel.de
 * 
 * Hardware: ATmega328P, Adafruit DS3231 RTC module, 2x DL-2416
 * 
 * This code is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <Arduino.h>
#include <RTClib.h>

RTC_DS3231 rtc;
int rtcFound;
int operationMode;
int menuMode;
int modeTimeout;
int modeTarget;
int displayBrightness;
int doDisplayUpdate;
int buttonState1;
int buttonState2;
int lastButtonState1;
int lastButtonState2;
int buttonHandled1;
int buttonHandled2;
int buttonRepeat1;
int buttonRepeat2;
unsigned long lastDebounceTime1 = 0;
unsigned long lastDebounceTime2 = 0;
unsigned long debounceDelay = 50;
unsigned long blinkTimeout = 0;
unsigned long lastSoftTimeout = 0;

int setHour, setMinute, setSecond;
int setYear, setMonth, setDay;

const int OP_TIME = 1;
const int OP_DATE = 2;
const int OP_YEAR = 3;
const int OP_TEMPERATURE = 4;
const int OP_MENU_DEMO = 5;
const int OP_MENU_SETTIME = 6;
const int OP_MENU_SETDATE = 7;
const int OP_MENU_SETBRIGHTNESS = 8;
const int OP_MENU_EXIT= 9;
const int OP_SET_HOUR = 10;
const int OP_SET_MINUTE = 11;
const int OP_SET_SECOND = 12;
const int OP_SET_YEAR = 13;
const int OP_SET_MONTH = 14;
const int OP_SET_DAY = 15;
const int OP_SET_BRIGHTNESS = 16;

// --------------------------------------------------------------------------
// Pin mapping
// --------------------------------------------------------------------------

// Note about other pins:
// 
// RESET is pin 1
// I2C SDA is pin 27 (used for the RTC module)
// I2C SCL is pin 28 (used for the RTC module)
// TX is pin 3
// RX is pin 2

const int D_D0    = 7;   // pin 13
const int D_D1    = 13;  // pin 19
const int D_D2    = A2;  // pin 25
const int D_D3    = A3;  // pin 26
const int D_D4    = 4;   // pin 6
const int D_D5    = 11;  // pin 17
const int D_D6    = 6;   // pin 12
const int D_A0    = 8;   // pin 14
const int D_A1    = 9;   // pin 15
const int D_WR    = 10;  // pin 16
const int D_CE1   = A0;  // pin 23
const int D_CE2   = A1;  // pin 24
const int BTN1    = 2;   // pin 4
const int BTN2    = 12;  // pin 18
const int RTC_PIN = 3;   // pin 5
const int PWM_OUT = 5;   // pin 11

// --------------------------------------------------------------------------
// Send a single byte to the display array
// --------------------------------------------------------------------------

void sendByte(int chip, int data, int adr)
{
  // set data lines
  
  if(data & 0x01) digitalWrite(D_D0, HIGH);else digitalWrite(D_D0, LOW);
  if(data & 0x02) digitalWrite(D_D1, HIGH);else digitalWrite(D_D1, LOW);
  if(data & 0x04) digitalWrite(D_D2, HIGH);else digitalWrite(D_D2, LOW);
  if(data & 0x08) digitalWrite(D_D3, HIGH);else digitalWrite(D_D3, LOW);
  if(data & 0x10) digitalWrite(D_D4, HIGH);else digitalWrite(D_D4, LOW);
  if(data & 0x20) digitalWrite(D_D5, HIGH);else digitalWrite(D_D5, LOW);
  if(data & 0x40) digitalWrite(D_D6, HIGH);else digitalWrite(D_D6, LOW);

  // set address lines

  if(adr & 0x01) digitalWrite(D_A0, HIGH);else digitalWrite(D_A0, LOW);
  if(adr & 0x02) digitalWrite(D_A1, HIGH);else digitalWrite(D_A1, LOW);

  // if adress is 0-3 use display 1, otherwise display 2
  
  if(adr < 4) {
    digitalWrite(D_CE1, HIGH);
    digitalWrite(D_CE2, LOW);
 
  } else {
    digitalWrite(D_CE1, LOW);
    digitalWrite(D_CE2, HIGH);
  }

  // finally write output to displays

  delayMicroseconds(500);
  digitalWrite(D_WR, LOW);
  delayMicroseconds(500);
  digitalWrite(D_WR, HIGH);
}

// --------------------------------------------------------------------------
// Send text to the display
// --------------------------------------------------------------------------

void sendText(const char* text)
{
  // limit text to 8 characters
  
  unsigned int n=strlen(text);
  if(n>8) n=8;

  // use padded output buffer
  
  char buf[]="        ";
  strncpy(buf, text, n);
  for(unsigned int i=0; i<8; i++) sendByte(0, buf[i], 7-i);
}

// --------------------------------------------------------------------------
// Scroll text on the display
// --------------------------------------------------------------------------

void scrollText(const char* text)
{
  // build string using a padding in front and back
  
  String output;
  output.concat("        ");
  output.concat(text);
  output.concat("        ");
  for(unsigned int i=0; i<output.length()-7; i++) {
    const char *outputBuf = output.c_str();
    sendText(&outputBuf[i]);
    delay(250);
  }
}

// --------------------------------------------------------------------------
// Display current time
// --------------------------------------------------------------------------

void displayTime()
{
  if (!rtcFound) {
    sendText("??:??:??");
    return;
  }
  char lineout[9];
  DateTime now = rtc.now();
  snprintf(lineout, 9, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
  sendText(lineout);
}

// --------------------------------------------------------------------------
// Display current day
// --------------------------------------------------------------------------

void displayDate()
{
  if (!rtcFound) {
    sendText("?? ?\?/??");
    return;
  }
  char lineout[9];
  char *dayoutput = (char *)"???";
  DateTime now = rtc.now();
  switch (now.dayOfTheWeek()) {
    case 0: dayoutput = (char *)"SU";break;
    case 1: dayoutput = (char *)"MO";break;
    case 2: dayoutput = (char *)"TU";break;
    case 3: dayoutput = (char *)"WE";break;
    case 4: dayoutput = (char *)"TH";break;
    case 5: dayoutput = (char *)"FR";break;
    case 6: dayoutput = (char *)"SA";break;
  }
  snprintf(lineout, 9, "%s %02d/%02d", dayoutput, now.day(), now.month());
  sendText(lineout);
}


// --------------------------------------------------------------------------
// Display current year
// --------------------------------------------------------------------------

void displayYear()
{
  if (!rtcFound) {
    sendText("  ????");
    return;
  }
  char lineout[9];
  DateTime now = rtc.now();
  snprintf(lineout, 9, "  %04d", now.year());
  sendText(lineout);
}

// --------------------------------------------------------------------------
// Display current temperatur
// --------------------------------------------------------------------------

void displayTemperature()
{
  if (!rtcFound) {
    sendText("T: ?.?");
    return;
  }
  String temperatureOutput = String(rtc.getTemperature(), 1);
  char lineout[9];
  snprintf(lineout, 9, "T: %s", temperatureOutput.c_str());
  sendText(lineout);
}

// --------------------------------------------------------------------------
// RTC SQW signal interrupt handler
// --------------------------------------------------------------------------

void handleInterruptRTC()
{
  // Reset blink timer only if no button is in repeat state
  if (!buttonRepeat1 && !buttonRepeat2) {
    blinkTimeout = 500;
  }

  // Trigger display update
  doDisplayUpdate = 1;
}

// --------------------------------------------------------------------------
// Setup
// --------------------------------------------------------------------------

void setup() {
  // initialize I/O
  
  pinMode(D_D0, OUTPUT);
  pinMode(D_D1, OUTPUT);
  pinMode(D_D2, OUTPUT);
  pinMode(D_D3, OUTPUT);
  pinMode(D_D4, OUTPUT);
  pinMode(D_D5, OUTPUT);
  pinMode(D_D6, OUTPUT);
  pinMode(D_A0, OUTPUT);
  pinMode(D_A1, OUTPUT);
  pinMode(D_WR, OUTPUT);
  pinMode(D_CE1, OUTPUT);
  pinMode(D_CE2, OUTPUT);
  pinMode(BTN1, INPUT_PULLUP);
  pinMode(BTN2, INPUT_PULLUP);
  pinMode(PWM_OUT, OUTPUT);

  digitalWrite(D_WR, HIGH);
  digitalWrite(D_CE1, HIGH);
  digitalWrite(D_CE2, HIGH);
  analogWrite(PWM_OUT, 255);

  // initialize global stuff

  operationMode = OP_TIME;
  modeTimeout = 0;
  modeTarget = OP_TIME;
  rtcFound = 0;
  doDisplayUpdate = 0;
  displayBrightness = 100;
  buttonState1 = HIGH;
  buttonState2 = HIGH;
  lastButtonState1 = HIGH;
  lastButtonState2 = HIGH;
  buttonHandled1 = 0;
  buttonHandled2 = 0;

  // initialize RTC module
  
  if (rtc.begin()) {
    rtcFound = 1;

    // if RTC lost its power (battery empty/missing) set time
    // to modification time of this file
    
    if (rtc.lostPower()) {
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

    // setup interrupt for time display update
    
    pinMode(RTC_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(RTC_PIN), handleInterruptRTC, RISING);

    // tell the RTC to output a 1 Hz signal for the interrupt trigger
    
    rtc.writeSqwPinMode(DS3231_SquareWave1Hz);
  }

  // display welcome message
  
  sendText("INIT OK");
  delay(1000);
  if (!rtcFound) {
    sendText("NO RTC");
    delay(1000);
  } else {
    sendText("RTC OK");
    delay(1000);
  }

  lastSoftTimeout = millis();
}

// --------------------------------------------------------------------------
// Debounced button reading
// --------------------------------------------------------------------------

void readButtonDebounced(int num)
{
  int port, *lastButtonState, *buttonState, *buttonHandled, *buttonRepeat;
  long unsigned int *lastDebounceTime;
  
  // Set references depending on which button we want to read

  if (1 == num) {
    port = BTN1;
    lastButtonState = &lastButtonState1;
    lastDebounceTime = &lastDebounceTime1;
    buttonState = &buttonState1;
    buttonHandled = &buttonHandled1;
    buttonRepeat = &buttonRepeat1;
  } else {
    port = BTN2;
    lastButtonState = &lastButtonState2;
    lastDebounceTime = &lastDebounceTime2;
    buttonState = &buttonState2;
    buttonHandled = &buttonHandled2;
    buttonRepeat = &buttonRepeat2;
  }
  
  // Read current button state
  
  int reading = digitalRead(port);

  if (reading != *lastButtonState) {
    // State changed, then record current time as last change time
    
    *lastDebounceTime = millis();
    *lastButtonState = reading;
  }

  if ((millis() - *lastDebounceTime) > debounceDelay) {

    // If last change time is older than the desired debounce delay,
    // and button state changed, then keep this state as current button state
    // and reset flags for "handled" and "repeat active"
    
    if (reading != *buttonState)
    {
      *buttonState = reading;
      *buttonHandled = 0;
      *buttonRepeat = 0;
    } else {

      // If state did not change, check if button is already in repeat mode

      if (*buttonState == LOW && millis() - lastDebounceTime1 > 2000) {
        *buttonRepeat = 1;
      }
    }
  }
}

// --------------------------------------------------------------------------
// Handle display update with function call
// --------------------------------------------------------------------------

void handleDisplayUpdateFunction(void (*displayFunc)(void))
{
  if (doDisplayUpdate) {
    doDisplayUpdate = 0;
    displayFunc();
  }
}

// --------------------------------------------------------------------------
// Handle display update with text
// --------------------------------------------------------------------------

void handleDisplayUpdateText(const char *text)
{
  if (doDisplayUpdate) {
    doDisplayUpdate = 0;
    sendText(text);
  }
}

// --------------------------------------------------------------------------
// Set button handled flag and new operation mode
// --------------------------------------------------------------------------

void setButtonHandled(int num, int newOperationMode, int newModeTimeout, int newModeTarget)
{
  int *buttonHandled;

  if (1 == num) {
    buttonHandled = &buttonHandled1;
  } else {
    buttonHandled = &buttonHandled2;
  }

  *buttonHandled = 1;
  operationMode = newOperationMode;
  doDisplayUpdate = 1;
  modeTimeout = newModeTimeout;
  modeTarget = newModeTarget;
}

// --------------------------------------------------------------------------
// Loop in time mode
// --------------------------------------------------------------------------

void loopTime()
{
  handleDisplayUpdateFunction(displayTime);

  if (LOW == buttonState1 && !buttonHandled1) {
    setButtonHandled(1, OP_MENU_DEMO, 10000, OP_TIME);
  } else if (LOW == buttonState2 && !buttonHandled2) {
    setButtonHandled(2, OP_DATE, 5000, OP_TIME);
  }
}

// --------------------------------------------------------------------------
// Loop in date mode
// --------------------------------------------------------------------------

void loopDate()
{
  handleDisplayUpdateFunction(displayDate);

  if (LOW == buttonState1 && !buttonHandled1) {
    setButtonHandled(1, OP_MENU_DEMO, 10000, OP_TIME);
  } else if (LOW == buttonState2 && !buttonHandled2) {
    setButtonHandled(2, OP_YEAR, 5000, OP_TIME);
  }
}

// --------------------------------------------------------------------------
// Loop in year mode
// --------------------------------------------------------------------------

void loopYear()
{
  handleDisplayUpdateFunction(displayYear);

  if (LOW == buttonState1 && !buttonHandled1) {
    setButtonHandled(1, OP_MENU_DEMO, 10000, OP_TIME);
  } else if (LOW == buttonState2 && !buttonHandled2) {
    setButtonHandled(2, OP_TEMPERATURE, 5000, OP_TIME);
  }
}

// --------------------------------------------------------------------------
// Loop in temp mode
// --------------------------------------------------------------------------

void loopTemperature()
{
  handleDisplayUpdateFunction(displayTemperature);

  if (LOW == buttonState1 && !buttonHandled1) {
    setButtonHandled(1, OP_MENU_DEMO, 10000, OP_TIME);
  } else if (LOW == buttonState2 && !buttonHandled2) {
    setButtonHandled(2, OP_TIME, 0, OP_TIME);
  }
}

// --------------------------------------------------------------------------
// Loop for menu - "demo"
// --------------------------------------------------------------------------

void loopMenuDemo()
{
  handleDisplayUpdateText("DEMO");
  
  if (LOW == buttonState1 && !buttonHandled1) {

    setButtonHandled(1, OP_MENU_SETTIME, 10000, OP_TIME);
  } else if (LOW == buttonState2 && !buttonHandled2) {
    buttonHandled2 = 1;
    for(int n=0; n<20; n++) {
      sendText("--------");
      delay(50);
      sendText("\\\\\\\\\\\\\\\\");
      delay(50);
      sendText("11111111");
      delay(50);
      sendText("////////");
      delay(50);
    }
    scrollText("!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_");
    doDisplayUpdate = 1;
    modeTimeout = 10000;
  }
}

// --------------------------------------------------------------------------
// Loop for menu - "set time"
// --------------------------------------------------------------------------

void loopMenuSetTime()
{
  handleDisplayUpdateText("SET TIME");

  if (LOW == buttonState1 && !buttonHandled1) {
    setButtonHandled(1, OP_MENU_SETDATE, 10000, OP_TIME);
  } else if (LOW == buttonState2 && !buttonHandled2) {
    buttonHandled2 = 1;
    if (!rtcFound) {
      sendText("NO RTC");
      delay(1000);
    } else {
      operationMode = OP_SET_HOUR;
      modeTimeout = 0;
      DateTime now = rtc.now();
      setHour = now.hour();
      setMinute = now.minute();
      setSecond = now.second();
      doDisplayUpdate = 1;
    }
  }
}

// --------------------------------------------------------------------------
// Loop for menu - "set date"
// --------------------------------------------------------------------------

void loopMenuSetDate()
{
  handleDisplayUpdateText("SET DATE");

  if (LOW == buttonState1 && !buttonHandled1) {
    setButtonHandled(1, OP_MENU_SETBRIGHTNESS, 10000, OP_TIME);
  } else if (LOW == buttonState2 && !buttonHandled2) {
    buttonHandled2 = 1;
    if (!rtcFound) {
      sendText("NO RTC");
      delay(1000);
    } else {
      operationMode = OP_SET_YEAR;
      modeTimeout = 0;
      DateTime now = rtc.now();
      setYear = now.year();
      setMonth = now.month();
      setDay = now.day();
      doDisplayUpdate = 1;
    }
  }
}

// --------------------------------------------------------------------------
// Loop for menu - "set brightness"
// --------------------------------------------------------------------------

void loopMenuSetBrightness()
{
  handleDisplayUpdateText("LIGHT");

  if (LOW == buttonState1 && !buttonHandled1) {
    setButtonHandled(1, OP_MENU_EXIT, 10000, OP_TIME);
  } else if (LOW == buttonState2 && !buttonHandled2) {
    buttonHandled2 = 1;
    operationMode = OP_SET_BRIGHTNESS;
    modeTimeout = 0;
    doDisplayUpdate = 1;
  }
}

// --------------------------------------------------------------------------
// Loop for menu - "exit"
// --------------------------------------------------------------------------

void loopMenuExit()
{
  handleDisplayUpdateText("EXIT");

  if (LOW == buttonState1 && !buttonHandled1) {
    setButtonHandled(1, OP_MENU_DEMO, 10000, OP_TIME);
  } else if (LOW == buttonState2 && !buttonHandled2) {
    setButtonHandled(2, OP_TIME, 0, OP_TIME);
  }
}

// --------------------------------------------------------------------------
// Loop for time setting
// --------------------------------------------------------------------------

void loopSetTime()
{
  if (doDisplayUpdate) {
    doDisplayUpdate = 0;
    
    char lineout[9];

    snprintf(lineout, 9, "%02d:%02d:%02d", setHour, setMinute, setSecond);

    if(blinkTimeout > 0) {
      int pos = 0;
      if (OP_SET_MINUTE == operationMode) {
        pos = 3;
      } else if (OP_SET_SECOND == operationMode) {
        pos = 6;
      }
      lineout[pos] = ' ';
      lineout[pos+1] = ' ';
    }

    sendText(lineout);
  }

  if (LOW == buttonState1) {
    if (!buttonHandled1 || buttonRepeat1) {
      buttonHandled1 = 1;
      switch (operationMode) {
        case OP_SET_HOUR:
          setHour++;
          if (setHour > 23) {
            setHour = 0;
          }
          break;
        case OP_SET_MINUTE:
          setMinute++;
          if (setMinute > 59) {
            setMinute = 0;
          }
          break;
        case OP_SET_SECOND:
          setSecond++;
          if (setSecond > 59) {
            setSecond = 0;
          }
          break;
      }
      doDisplayUpdate = 1;
      blinkTimeout = 0;

      if (buttonRepeat1) {
        delay(100);
      }
    }
  } else if (LOW == buttonState2 && !buttonHandled2) {
    buttonHandled2 = 1;
    switch (operationMode) {
      case OP_SET_HOUR:
        operationMode = OP_SET_MINUTE;
        doDisplayUpdate = 1;
        blinkTimeout = 0;
        break;
      case OP_SET_MINUTE:
        operationMode = OP_SET_SECOND;
        doDisplayUpdate = 1;
        blinkTimeout = 0;
        break;
      case OP_SET_SECOND:
        operationMode = OP_TIME;
        doDisplayUpdate = 1;
        blinkTimeout = 0;
        DateTime now = rtc.now();
        rtc.adjust(DateTime(now.year(), now.month(), now.day(), setHour, setMinute, setSecond));
        break;
    }
  }
}

// --------------------------------------------------------------------------
// Loop for date setting
// --------------------------------------------------------------------------

void loopSetDate()
{
  if (doDisplayUpdate) {
    doDisplayUpdate = 0;
    
    char lineout[9];

    switch (operationMode) {
      case OP_SET_YEAR:
        snprintf(lineout, 9, "Y: %04d", setYear);
        break;
      case OP_SET_MONTH:
        snprintf(lineout, 9, "M: %02d", setMonth);
        break;
      case OP_SET_DAY:
        DateTime date = DateTime(setYear, setMonth, setDay, 0, 0, 0);
        char *dayoutput = (char *)"??";
        switch (date.dayOfTheWeek()) {
          case 0: dayoutput = (char *)"SU";break;
          case 1: dayoutput = (char *)"MO";break;
          case 2: dayoutput = (char *)"TU";break;
          case 3: dayoutput = (char *)"WE";break;
          case 4: dayoutput = (char *)"TH";break;
          case 5: dayoutput = (char *)"FR";break;
          case 6: dayoutput = (char *)"SA";break;
        }
        snprintf(lineout, 9, "D: %02d %s", setDay, dayoutput);
        break;
    }
    if (blinkTimeout > 0) {
      lineout[3] = 0;
    }

    sendText(lineout);
  }

  int isLeapYear;
  if (setYear % 400 == 0) {
    isLeapYear = 1;
  } else if (setYear % 100 == 0) {
    isLeapYear = 0;
  } else if (setYear % 4) {
    isLeapYear = 1;
  } else {
    isLeapYear = 0;
  }

  if (LOW == buttonState1) {
    if (!buttonHandled1 || buttonRepeat1) {
      buttonHandled1 = 1;
      switch (operationMode) {
        case OP_SET_YEAR:
          setYear++;
          if (setYear > 2037) {
            setYear = 2021;
          }
          break;
        case OP_SET_MONTH:
          setMonth++;
          if (setMonth > 11) {
            setMonth = 1;
          }
          break;
        case OP_SET_DAY:
          setDay++;
          switch (setMonth) {
            case 1:
            case 3:
            case 6:
            case 8:
            case 10:
            case 12:
              if (setDay > 31) {
                setDay = 1;
              }
              break;
            case 2:
              if (isLeapYear) {
                if (setDay > 29) {
                  setDay = 1;
                }
              } else {
                if (setDay > 28) {
                  setDay = 1;
                }
              }
              break;
            default:
              if (setDay > 30) {
                setDay = 1;
              }
              break;
          }
          break;
      }
      doDisplayUpdate = 1;
      blinkTimeout = 0;

      if (buttonRepeat1) {
        delay(100);
      }
    }
  } else if (LOW == buttonState2 && !buttonHandled2) {
    buttonHandled2 = 1;
    switch (operationMode) {
      case OP_SET_YEAR:
        operationMode = OP_SET_MONTH;
        doDisplayUpdate = 1;
        blinkTimeout = 0;
        break;
      case OP_SET_MONTH:
        operationMode = OP_SET_DAY;
        doDisplayUpdate = 1;
        blinkTimeout = 0;
        break;
      case OP_SET_DAY:
        operationMode = OP_TIME;
        doDisplayUpdate = 1;
        blinkTimeout = 0;
        DateTime now = rtc.now();
        rtc.adjust(DateTime(setYear, setMonth, setDay, now.hour(), now.minute(), now.second()));
        break;
    }
  }
}

// --------------------------------------------------------------------------
// Loop for brightness setting
// --------------------------------------------------------------------------

void loopSetBrightness()
{
  if (doDisplayUpdate) {
    doDisplayUpdate = 0;
    
    char lineout[9];

    snprintf(lineout, 9, "L: %03d%%", displayBrightness);

    if(blinkTimeout > 0) {
      lineout[3] = 0;
    }

    sendText(lineout);
  }

  if (LOW == buttonState1) {
    if (!buttonHandled1 || buttonRepeat1) {
      buttonHandled1 = 1;
      displayBrightness += 1;
      if (displayBrightness > 100) {
        displayBrightness = 10;
      }
      analogWrite(PWM_OUT, displayBrightness * 255 / 100);
      doDisplayUpdate = 1;
      blinkTimeout = 0;

      if (buttonRepeat1) {
        delay(100);
      }
    }
  } else if (LOW == buttonState2 && !buttonHandled2) {
    buttonHandled2 = 1;
    operationMode = OP_TIME;
    doDisplayUpdate = 1;
    blinkTimeout = 0;
  }
}

// --------------------------------------------------------------------------
// Main loop
// --------------------------------------------------------------------------

void loop() {
  // Update current button state
  
  readButtonDebounced(1);
  readButtonDebounced(2);
  
  // If no RTC is present, keep display update running in software

  if (!rtcFound) {
    if (millis() - lastSoftTimeout > 999) {
      blinkTimeout = 500;
      doDisplayUpdate = 1;
      lastSoftTimeout = millis();
    }
  }

  // Dot blink timer

  if (blinkTimeout > 1) {
    blinkTimeout--;
  } else if (1 == blinkTimeout) {
    blinkTimeout = 0;
    doDisplayUpdate = 1;
  }

  // Countdown timer for new operation mode if set

  if (modeTimeout > 0) {
    modeTimeout--;
    if (0 == modeTimeout) {
      operationMode = modeTarget;
    }
  }

  // Handle current operation mode
  
  switch (operationMode) {
    case OP_TIME:
      loopTime();
      break;
    case OP_DATE:
      loopDate();
      break;
    case OP_YEAR:
      loopYear();
      break;
    case OP_TEMPERATURE:
      loopTemperature();
      break;
    case OP_MENU_DEMO:
      loopMenuDemo();
      break;
    case OP_MENU_SETTIME:
      loopMenuSetTime();
      break;
    case OP_MENU_SETDATE:
      loopMenuSetDate();
      break;
    case OP_MENU_SETBRIGHTNESS:
      loopMenuSetBrightness();
      break;
    case OP_MENU_EXIT:
      loopMenuExit();
      break;
    case OP_SET_HOUR:
    case OP_SET_MINUTE:
    case OP_SET_SECOND:
      loopSetTime();
      break;
    case OP_SET_YEAR:
    case OP_SET_MONTH:
    case OP_SET_DAY:
      loopSetDate();
      break;
    case OP_SET_BRIGHTNESS:
      loopSetBrightness();
      break;
  }

  delay(1);
}

# AlphaClock

Running old LED displays (DL-2416T) with an ATmega328P and a DS3231 RTC module to display time and temperature.

Copyright (C) 2021 Arno Welzel

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see http://www.gnu.org/licenses/.

## How to use the buttons

### Button 1

This button will change from display mode to the menu mode. When pressing button 1 again in menu mode, it will cycle through all menu items:

- DEMO
- SET TIME
- SET DATE
- LIGHT
- EXIT

The menu mode will automatically return to display mode after 10 seconds when no button was pressed.

When SET TIME, SET DATE or LIGHT was selected, button 1 will increase the selected item. For example: when SET TIME was selected, the time setting will show with a flashing hour. When you now press button 1 the hour will be increased by 1.

### Button 2

In display mode this button will cycle through the following displays:

- Time dispayed as HH:MM:SS
- Date displayed as Day-of-week DD/MM
- Year displayed as YYYY
- Temperature displayed as T: n.n

After 5 seconds the display will automatically return to time display when no button was pressed.

In menu mode button 1 will enter the selected function and change to the active item depending on the menu:

- DEMO - will start the demo (the demo will return to menu mode when finished).
- SET TIME - change from hours to minutes and seconds and finally store the time to the RTC module.
- SET DATE - change from year to month and day and finally store the date to the RTC module.
- LIGHT - store the selected brightness.
- EXIT - will leave the menu and return to time display.

Note: to exit any of the settings without changing anything, press the reset button.

## Schematics

Schematics are included as PDF. A PCB as KiCad project may follow in the future.

## How to build the software

I use [Visual Studio Code](https://code.visualstudio.com) with [PlatformIO](https://platformio.org).

To upload the software to the ATmega328P you need an AVR programmer. If needed, adjust the programmer configuration in `platformio.ini`.

## Changelog

### 1.1

Brightness setting will be stored to the EEPROM when the setting is saved and restored during setup.

### 1.0

Initial version.

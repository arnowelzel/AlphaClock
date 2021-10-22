# AlphaClock

Running old LED displays (DL-2416T) with an ATmega328P and a DS3231 RTC module to display time and temperature.

Copyright (C) 2021 Arno Welzel

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see http://www.gnu.org/licenses/.

## Schematics

Schematics are included as PDF. A PCB as KiCad project may follow in the future.

## How to build the software

I use [Visual Studio Code](https://code.visualstudio.com) with [PlatformIO](https://platformio.org).

To upload the software to the ATmega328P you need an AVR programmer. If needed, adjust the programmer configuration in `platformio.ini`.

/*
 Implements a simple interface to the time function of the PCF8583 RTC chip

 Works around the device's limited year storage by keeping the year in the
 first two bytes of user accessible storage

 Assumes device is attached in the standard location - Analog pins 4 and 5
 Device address is the 8 bit address (as in the device datasheet - normally A0)

 Copyright (c) 2009, Erik DeBill


 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <Arduino.h>
#include <Wire.h>
#include "PCF8583.h"

#define STATUS_REG 0x00
#define ALARM_REG 0x08

#define ALARM_ENABLE (1 << 2)
#define HOLD_LAST_COUNT (1 << 6)
#define STOP_COUNTING (1 << 7)

#define DAILY_ALARM (1 << 4)
#define ALARM_INTERRUPT (1 << 7)

// provide device address as a full 8 bit address (like the datasheet)
PCF8583::PCF8583(int device_address)
{
	address = device_address >> 1;  // convert to 7 bit so Wire doesn't choke
	Wire.begin();
	second = 0;
	minute = 0;
	hour = 0;
	day = 0;
	year = 0;
	month = 0;
	alarm_enabled = 0;
	alarm_hour = 0;
	alarm_minute = 0;
	min_temp = 0;
	max_temp = 0;
}

void PCF8583::get_time()
{
	Wire.beginTransmission(address);
	Wire.write(0x02);
	Wire.endTransmission();
	Wire.requestFrom(address, 5);

	second = bcd_to_byte(Wire.read());
	minute = bcd_to_byte(Wire.read());
	hour = bcd_to_byte(Wire.read());
	byte incoming = Wire.read(); // year/date counter
	day = bcd_to_byte(incoming & 0x3f);
	year = (int) ((incoming >> 6) & 0x03);      // it will only hold 4 years...
	month = bcd_to_byte(Wire.read() & 0x1f);  // 0 out the weekdays part

	//  but that's not all - we need to find out what the base year is
	//  so we can add the 2 bits we got above and find the real year
	Wire.beginTransmission(address);
	Wire.write(0x10);
	Wire.endTransmission();
	Wire.requestFrom(address, 2);
	int yb = Wire.read();
	yb = yb << 8;
	yb = yb | Wire.read();
	year = year + yb;
}

void PCF8583::set_time()
{
	prepare_time();

	Wire.beginTransmission(address);
	Wire.write(STATUS_REG);
	Wire.write(HOLD_LAST_COUNT | STOP_COUNTING);
	Wire.endTransmission();

	Wire.beginTransmission(address);
	Wire.write(0x02);
	Wire.write(int_to_bcd(second));
	Wire.write(int_to_bcd(minute));
	Wire.write(int_to_bcd(hour));
	Wire.write(((byte) (year % 4) << 6) | int_to_bcd(day));
	Wire.write(int_to_bcd(month));
	Wire.endTransmission();

	Wire.beginTransmission(address);
	Wire.write(0x10);
	int yb = year - year % 4;
	Wire.write(yb >> 8);
	Wire.write(yb & 0x00ff);
	Wire.endTransmission();
	reset_alarm();
}

void PCF8583::set_minmax_temp()
{
	Wire.beginTransmission(address);
	Wire.write(STATUS_REG);
	Wire.write(HOLD_LAST_COUNT | STOP_COUNTING);
	Wire.endTransmission();

	Wire.beginTransmission(address);
	Wire.write(0x12);
	Wire.write(min_temp >> 8);
	Wire.write(min_temp & 0x00ff);
	Wire.write(max_temp >> 8);
	Wire.write(max_temp & 0x00ff);
	Wire.endTransmission();
	reset_alarm();
}

void PCF8583::get_minmax_temp()
{
	Wire.beginTransmission(address);
	Wire.write(0x12);
	Wire.endTransmission();
	Wire.requestFrom(address, 4);
	int mt = Wire.read();
	mt = mt << 8;
	mt = mt | Wire.read();
	min_temp = mt;
	mt = Wire.read();
	mt = mt << 8;
	mt = mt | Wire.read();
	max_temp = mt;
}

void PCF8583::get_alarm_time()
{
	Wire.beginTransmission(address);
	Wire.write(0x0b);
	Wire.endTransmission();
	Wire.requestFrom(address, 2);
	alarm_minute = bcd_to_byte(Wire.read());
	alarm_hour = bcd_to_byte(Wire.read());
}

void PCF8583::set_alarm_time()
{
	prepare_alarm_time();
	Wire.beginTransmission(address);
	Wire.write(0x09);
	Wire.write(0);
	Wire.write(0);
	Wire.write(int_to_bcd(alarm_minute));
	Wire.write(int_to_bcd(alarm_hour));
	Wire.write(0);
	Wire.write(0);
	Wire.write(0);
	Wire.endTransmission();
	reset_alarm();
}

void PCF8583::reset_alarm()
{
	Wire.beginTransmission(address);
	Wire.write(STATUS_REG);
	Wire.write(ALARM_ENABLE);
	Wire.endTransmission();

	Wire.beginTransmission(address);
	Wire.write(ALARM_REG);
	Wire.write(0);
	Wire.endTransmission();
	if (alarm_enabled)
	{
		Wire.beginTransmission(address);
		Wire.write(ALARM_REG);
		Wire.write(DAILY_ALARM | ALARM_INTERRUPT);
		Wire.endTransmission();
	}
}

int PCF8583::bcd_to_byte(byte bcd)
{
	return ((bcd >> 4) * 10) + (bcd & 0x0f);
}

byte PCF8583::int_to_bcd(int in)
{
	return ((in / 10) << 4) + (in % 10);
}

int PCF8583::get_num_of_days(int month)
{
	int numOfDays = 0;
	if (month == 2)
	{
		if (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))
		{
			numOfDays = 29;
		}
		else
		{
			numOfDays = 28;
		}
	}
	else if (month == 4 || month == 6 || month == 9 || month == 11)
	{
		numOfDays = 30;
	}
	else
	{
		numOfDays = 31;
	}
	return numOfDays;
}

void PCF8583::prepare_value(int *val, int min, int max)
{
	if (*val > max)
	{
		*val = min;
	}
	if (*val < min)
	{
		*val = max;
	}
}

void PCF8583::prepare_time()
{
	prepare_value(&year, 1970, 2500);
	prepare_value(&month, 1, 12);
	prepare_value(&day, 1, get_num_of_days(month));
	prepare_value(&hour, 0, 23);
	prepare_value(&minute, 0, 59);
	prepare_value(&second, 0, 59);
}

void PCF8583::prepare_alarm_time()
{
	prepare_value(&alarm_hour, 0, 23);
	prepare_value(&alarm_minute, 0, 59);
}

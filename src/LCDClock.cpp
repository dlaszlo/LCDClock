#include <Arduino.h>
#include <IRremote.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <PCF8583.h>
#include "LCD.h"

#define PIN_BACKLIGHT 13
#define PIN_IR 8
#define PIN_ONE_WIRE_BUS 9
#define PIN_BEEP 7

#define PCF8583_ADDRESS 0x0a0

#define MODE_NORMAL 0
#define MODE_SET_TIME 1
#define MODE_SET_ALARM 2
#define MODE_ALARM 3

#define TRUE 1
#define FALSE 0

LCD lcd(12, 11, 10, 5, 4, 3, 2);

IRrecv irrecv(PIN_IR);
decode_results results;

OneWire oneWire(PIN_ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress thermometer;
PCF8583 pcf8583(PCF8583_ADDRESS);

const char * months[] =
{ "jan", "feb", "már", "ápr", "máj", "jún", "júl", "aug", "sze", "okt", "nov",
		"dec" };

volatile uint8_t mode = 0;
volatile uint8_t set_field = 0;
volatile uint8_t ir_rec = 0;
volatile uint32_t alarm_start = 0;
volatile uint32_t alarm_stop = 0;

void setup()
{
	pinMode(PIN_BACKLIGHT, OUTPUT);
	digitalWrite(PIN_BACKLIGHT, HIGH);
	pinMode(A3, INPUT);
	digitalWrite(A3, HIGH);

	irrecv.enableIRIn();

	sensors.getAddress(thermometer, 0);

	pcf8583.set_alarm_time();
	pcf8583.reset_alarm();
	pcf8583.get_time();
	if (pcf8583.year == 0)
	{
		pcf8583.year = 2013;
		pcf8583.set_time();
	}
	pinMode(PIN_BEEP, OUTPUT);
	digitalWrite(PIN_BEEP, LOW);
}

void loop()
{

	int alarm = !digitalRead(A3);
	if (alarm)
	{
		uint32_t now = millis();
		if (alarm_start == 0)
		{
			alarm_start = now;
			alarm_stop = now + 5000;
		}
		if (now > alarm_stop)
		{
			alarm_start = 0;
			alarm_stop = 0;
			pcf8583.reset_alarm();
			digitalWrite(PIN_BEEP, LOW);
		}
		else
		{
			uint32_t beep = (now - alarm_start) >> 9;
			digitalWrite(PIN_BEEP, (beep & 1) ? LOW : HIGH);
		}
	}
	else
	{
		alarm_start = 0;
		alarm_stop = 0;
		digitalWrite(PIN_BEEP, LOW);
	}

	char txt[17] = "";
	char temp[17] = "";

	lcd.clearBuffer();

	if (mode == MODE_NORMAL)
	{
		// elsõ sor
		pcf8583.get_time();
		memset(txt, 0, 17);
		sprintf(txt, "%04d.%s.%02d.", pcf8583.year, months[pcf8583.month - 1],
				pcf8583.day);
		lcd.center(0, txt);

		// második sor
		memset(txt, 0, 17);
		sprintf(txt, "%02d:%02d:%02d", pcf8583.hour, pcf8583.minute,
				pcf8583.second);
		lcd.setText(0, 1, txt);
		sensors.requestTemperatures();
		float tf = sensors.getTempC(thermometer);
		memset(txt, 0, 17);
		memset(temp, 0, 17);
		dtostrf(tf, 1, 1, temp);
		sprintf(txt, "%s C", temp);
		lcd.right(1, txt);
		lcd.setText(14, 1, LCD_DEGREE);
	}
	else if (mode == MODE_SET_TIME)
	{
		// elsõ sor
		memset(txt, 0, 17);
		sprintf(txt, "%04d.%s.%02d.", pcf8583.year, months[pcf8583.month - 1],
				pcf8583.day);
		lcd.center(0, txt);

		// második sor
		memset(txt, 0, 17);
		sprintf(txt, "%02d:%02d:%02d", pcf8583.hour, pcf8583.minute,
				pcf8583.second);
		lcd.center(1, txt);
		switch (set_field)
		{
		case 0:
			lcd.setText(1, 0, LCD_ARROW_RIGHT);
			lcd.setText(6, 0, LCD_ARROW_LEFT);
			break;
		case 1:
			lcd.setText(6, 0, LCD_ARROW_RIGHT);
			lcd.setText(10, 0, LCD_ARROW_LEFT);
			break;
		case 2:
			lcd.setText(10, 0, LCD_ARROW_RIGHT);
			lcd.setText(13, 0, LCD_ARROW_LEFT);
			break;
		case 3:
			lcd.setText(3, 1, LCD_ARROW_RIGHT);
			lcd.setText(6, 1, LCD_ARROW_LEFT);
			break;
		case 4:
			lcd.setText(6, 1, LCD_ARROW_RIGHT);
			lcd.setText(9, 1, LCD_ARROW_LEFT);
			break;
		case 5:
			lcd.setText(9, 1, LCD_ARROW_RIGHT);
			lcd.setText(12, 1, LCD_ARROW_LEFT);
			break;
		}
	}
	else if (mode == MODE_SET_ALARM)
	{
		// elsõ sor
		lcd.center(0, "ébresztõ");

		// második sor
		memset(txt, 0, 17);
		sprintf(txt, "%02d:%02d", pcf8583.alarm_hour, pcf8583.alarm_minute);

		lcd.center(1, txt);
		switch (set_field)
		{
		case 0:
			lcd.setText(4, 1, LCD_ARROW_RIGHT);
			lcd.setText(7, 1, LCD_ARROW_LEFT);
			break;
		case 1:
			lcd.setText(7, 1, LCD_ARROW_RIGHT);
			lcd.setText(10, 1, LCD_ARROW_LEFT);
			break;
		}
	}
	if (pcf8583.alarm_enabled)
	{
		lcd.setText(0, 0, LCD_ALARM);
	}

	lcd.show();

	if (irrecv.decode(&results)) // have we received an IR signal?
	{
		if (!ir_rec)
		{
			ir_rec = TRUE;
			irrecv.resume();
			int val = results.value & 0x7FF;

			switch (val)
			{
			case 0x0510: // fel
				if (mode == MODE_SET_TIME)
				{
					switch (set_field)
					{
					case 0:
						pcf8583.year++;
						break;
					case 1:
						pcf8583.month++;
						break;
					case 2:
						pcf8583.day++;
						break;
					case 3:
						pcf8583.hour++;
						break;
					case 4:
						pcf8583.minute++;
						break;
					case 5:
						pcf8583.second++;
						break;
					}
					pcf8583.prepare_time();
				}
				else if (mode == MODE_SET_ALARM)
				{
					switch (set_field)
					{
					case 0:
						pcf8583.alarm_hour++;
						break;
					case 1:
						pcf8583.alarm_minute++;
						break;
					}
					pcf8583.prepare_alarm_time();
				}
				break;
			case 0x0511: // le
				if (mode == MODE_SET_TIME)
				{
					switch (set_field)
					{
					case 0:
						pcf8583.year--;
						break;
					case 1:
						pcf8583.month--;
						break;
					case 2:
						pcf8583.day--;
						break;
					case 3:
						pcf8583.hour--;
						break;
					case 4:
						pcf8583.minute--;
						break;
					case 5:
						pcf8583.second--;
						break;
					}
					pcf8583.prepare_time();
				}
				else if (mode == MODE_SET_ALARM)
				{
					switch (set_field)
					{
					case 0:
						pcf8583.alarm_hour--;
						break;
					case 1:
						pcf8583.alarm_minute--;
						break;
					}
					pcf8583.prepare_alarm_time();
				}
				break;
			case 0x0520: // jobbra
				if (mode == MODE_SET_TIME)
				{
					set_field++;
					if (set_field > 5)
					{
						set_field = 0;
					}
				}
				else if (mode == MODE_SET_ALARM)
				{
					set_field++;
					if (set_field > 1)
					{
						set_field = 0;
					}
				}
				break;
			case 0x0521: // balra
				if (mode == MODE_SET_TIME)
				{
					set_field--;
					if (set_field < 0)
					{
						set_field = 5;
					}
				}
				else if (mode == MODE_SET_ALARM)
				{
					set_field--;
					if (set_field < 0)
					{
						set_field = 1;
					}
				}
				break;
			case 0x0532: // shift
				break;
			case 0x0534: // sleep
				pcf8583.alarm_enabled ^= 1;
				pcf8583.reset_alarm();
				break;
			case 0x050c: // edit
				if (mode == MODE_NORMAL)
				{
					mode = MODE_SET_ALARM;
					set_field = 0;
				}
				else if (mode == MODE_SET_ALARM)
				{
					pcf8583.set_alarm_time();
					mode = MODE_NORMAL;
				}
				break;
			case 0x0517: // enter
				if (mode == MODE_NORMAL)
				{
					mode = MODE_SET_TIME;
					set_field = 0;
				}
				else if (mode == MODE_SET_TIME)
				{
					pcf8583.set_time();
					mode = MODE_NORMAL;
				}
				break;
			}
		}
		else
		{
			ir_rec = FALSE;
		}
	}
	delay(100);
}

int main(void)
{
	init();
	setup();
	for (;;)
	{
		loop();
	}
}

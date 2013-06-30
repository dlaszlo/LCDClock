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
#define PIN_COOLER 7

#define PCF8583_ADDRESS 0x0a0

#define MODE_NORMAL 0
#define MODE_SET_TIME 1
#define MODE_SET_TEMP 2

#define COOLER_MODE_AUTO0 0
#define COOLER_MODE_ON 1
#define COOLER_MODE_AUTO1 2
#define COOLER_MODE_OFF 3

#define TRUE 1
#define FALSE 0

LCD lcd(12, 11, 10, 5, 4, 3, 2);

IRrecv irrecv(PIN_IR);
decode_results results;

OneWire oneWire(PIN_ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress thermometer;
PCF8583 pcf8583(PCF8583_ADDRESS);

volatile uint8_t cooler = 0;
volatile uint8_t cooler_mode = 0;
volatile uint8_t sleep_mode = 0;
volatile uint8_t mode = 0;
volatile uint8_t set_field = 0;
volatile uint8_t edit_time = 0;
volatile uint8_t ir_rec = 0;

void setup()
{
	pinMode(PIN_BACKLIGHT, OUTPUT);
	digitalWrite(PIN_BACKLIGHT, HIGH);
	pinMode(A3, INPUT);
	digitalWrite(A3, HIGH);

	lcd.clearBuffer();
	lcd.center(0, "akvárium hûtés");
	lcd.center(1, "verzió: 1.0");
	lcd.show();
	delay(500);
	sensors.getAddress(thermometer, 0);
	irrecv.enableIRIn();
	delay(500);

	pcf8583.get_time();
	if (pcf8583.year > 2030 || pcf8583.year < 2000)
	{
		pcf8583.year = 2013;
		pcf8583.set_time();
	}
	pcf8583.get_minmax_temp();
	if (pcf8583.min_temp > 320 || pcf8583.min_temp < 200
			|| pcf8583.max_temp > 320 || pcf8583.max_temp < 200)
	{
		pcf8583.min_temp = 250;
		pcf8583.max_temp = 280;
		pcf8583.set_minmax_temp();
	}

	cooler = 0;
	pinMode(PIN_COOLER, OUTPUT);
	digitalWrite(PIN_COOLER, LOW);
}

void loop()
{

	char txt[17] = "";
	char temp[17] = "";

	lcd.clearBuffer();

	if (!edit_time)
	{
		pcf8583.get_time();
	}

	if (mode == MODE_NORMAL)
	{
		pcf8583.get_minmax_temp();
		// elsõ sor
		memset(txt, 0, 17);
		sprintf(txt, "%04d.%02d.%02d", pcf8583.year, pcf8583.month,
				pcf8583.day);
		lcd.setText(0, 0, txt);

		// második sor
		memset(txt, 0, 17);
		sprintf(txt, "%02d:%02d:%02d", pcf8583.hour, pcf8583.minute,
				pcf8583.second);
		lcd.setText(0, 1, txt);

		sensors.requestTemperatures();
		float tf = sensors.getTempC(thermometer);

		if (pcf8583.hour < 7 || pcf8583.hour > 19)
		{
			sleep_mode = 1;
		}
		else
		{
			sleep_mode = 0;
		}
		switch (cooler_mode)
		{
		case COOLER_MODE_AUTO0:
		case COOLER_MODE_AUTO1:
			if (sleep_mode)
			{
				cooler = 0;
				lcd.right(0, "alvás");
			}
			else
			{
				float mintempf = pcf8583.min_temp / (float) 10;
				float maxtempf = pcf8583.max_temp / (float) 10;
				lcd.right(0, " auto");
				if (mintempf + 0.5 >= maxtempf)
				{
					cooler = 0;
					lcd.right(0, " hiba");
				}
				else if (tf >= maxtempf)
				{
					cooler = 1;
				}
				else if (tf < mintempf)
				{
					cooler = 0;
				}
			}
			break;
		case COOLER_MODE_ON:
			cooler = 1;
			lcd.right(0, "   be");
			break;
		case COOLER_MODE_OFF:
			cooler = 0;
			lcd.right(0, "   ki");
			break;
		}
		digitalWrite(PIN_COOLER, cooler ? HIGH : LOW);

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
		sprintf(txt, "%04d.%02d.%02d", pcf8583.year, pcf8583.month,
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
			lcd.setText(2, 0, LCD_ARROW_RIGHT);
			lcd.setText(7, 0, LCD_ARROW_LEFT);
			break;
		case 1:
			lcd.setText(7, 0, LCD_ARROW_RIGHT);
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
	else if (mode == MODE_SET_TEMP)
	{
		float mintempf = pcf8583.min_temp / (float) 10;
		float maxtempf = pcf8583.max_temp / (float) 10;
		memset(txt, 0, 17);
		lcd.setText(0, 0, "   be      ki");

		memset(temp, 0, 17);
		dtostrf(maxtempf, 1, 1, temp);
		sprintf(txt, "%s C", temp);
		lcd.setText(1, 1, txt);
		lcd.setText(5, 1, LCD_DEGREE);

		memset(temp, 0, 17);
		dtostrf(mintempf, 1, 1, temp);
		sprintf(txt, "%s C", temp);
		lcd.setText(9, 1, txt);
		lcd.setText(13, 1, LCD_DEGREE);

		switch (set_field)
		{
		case 0:
			lcd.setText(0, 1, LCD_ARROW_RIGHT);
			lcd.setText(7, 1, LCD_ARROW_LEFT);
			break;
		case 1:
			lcd.setText(8, 1, LCD_ARROW_RIGHT);
			lcd.setText(15, 1, LCD_ARROW_LEFT);
			break;
		}
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
					edit_time = 1;
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
				else if (mode == MODE_SET_TEMP)
				{
					switch (set_field)
					{
					case 0:
						pcf8583.max_temp++;
						break;
					case 1:
						pcf8583.min_temp++;
						break;
					}
				}
				break;
			case 0x0511: // le
				if (mode == MODE_SET_TIME)
				{
					edit_time = 1;
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
				else if (mode == MODE_SET_TEMP)
				{
					switch (set_field)
					{
					case 0:
						pcf8583.max_temp--;
						break;
					case 1:
						pcf8583.min_temp--;
						break;
					}
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
				else if (mode == MODE_SET_TEMP)
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
				else if (mode == MODE_SET_TEMP)
				{
					set_field--;
					if (set_field < 0)
					{
						set_field = 1;
					}
				}
				break;
			case 0x0532: // shift
				cooler_mode++;
				if (cooler_mode > 3)
				{
					cooler_mode = 0;
				}
				break;
			case 0x0534: // sleep
				if (mode == MODE_SET_TIME)
				{
					edit_time = 0;
					set_field = 0;
					mode = MODE_SET_TEMP;
				}
				else if (mode == MODE_SET_TEMP)
				{
					set_field = 0;
					mode = MODE_NORMAL;
				}
				break;
			case 0x050c: // edit
				break;
			case 0x0517: // enter
				if (mode == MODE_NORMAL)
				{
					mode = MODE_SET_TIME;
					set_field = 0;
				}
				else if (mode == MODE_SET_TIME)
				{
					if (edit_time)
					{
						pcf8583.set_time();
					}
					edit_time = 0;
					set_field = 0;
					mode = MODE_SET_TEMP;
				}
				else if (mode == MODE_SET_TEMP)
				{
					pcf8583.set_minmax_temp();
					set_field = 0;
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

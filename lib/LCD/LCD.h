/*
 * LCD.h
 *
 *  Created on: 2013.05.13.
 *      Author: Dávid
 */

#ifndef LCDUTIL_H_
#define LCDUTIL_H_

#include <Arduino.h>
#include <LiquidCrystal.h>

#define LCD_ARROW_LEFT "{"
#define LCD_ARROW_RIGHT "}"
#define LCD_DEGREE "|"
#define LCD_ALARM "~"

class LCD : public LiquidCrystal
{

public:
	LCD(uint8_t rs, uint8_t rw, uint8_t enable,
		     uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3);
	void clearBuffer();
	void setText(uint8_t col, uint8_t row, const char * txt);
	void center(uint8_t row, const char * txt);
	void right(uint8_t row, const char * txt);
	void show();

private:
	void replaceChars(char * to, const char * from);

};

#endif /* LCDUTIL_H_ */

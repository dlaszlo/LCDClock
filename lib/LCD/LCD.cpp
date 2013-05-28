/*
 * LCD.cpp
 *
 *  Created on: 2013.05.13.
 *      Author: Dávid
 */

#include "LCD.h"
#include <LiquidCrystal.h>

char lcdbuff[2][16] =
{ };

char lcdp[2][16] =
{ };

uint8_t charset_degree[8] =
{ 0b00000, 0b00100, 0b01110, 0b01110, 0b01110, 0b11111, 0b00000 };

uint8_t charset_a1[8] =
{ 0b00010, 0b00100, 0b01110, 0b00001, 0b01111, 0b10001, 0b01111 };

uint8_t charset_e1[8] =
{ 0b00010, 0b00100, 0b01110, 0b10001, 0b11111, 0b10000, 0b01110 };

uint8_t charset_i1[8] =
{ 0b00010, 0b00100, 0b00000, 0b01110, 0b00100, 0b00100, 0b01110 };

uint8_t charset_o1[8] =
{ 0b00100, 0b00100, 0b00000, 0b01110, 0b10001, 0b10001, 0b01110 };

uint8_t charset_o3[8] =
{ 0b01010, 0b01010, 0b00000, 0b01110, 0b10001, 0b10001, 0b01110 };

uint8_t charset_u1[8] =
{ 0b00010, 0b00100, 0b10001, 0b10001, 0b10001, 0b10011, 0b01101 };

uint8_t charset_u3[8] =
{ 0b01010, 0b01010, 0b00000, 0b10001, 0b10001, 0b10011, 0b01101 };

LCD::LCD(uint8_t rs, uint8_t rw, uint8_t enable, uint8_t d0, uint8_t d1,
		uint8_t d2, uint8_t d3) :
		LiquidCrystal(rs, rw, enable, d0, d1, d2, d3)
{
	clearBuffer();
	begin(16, 2);
	createChar(0, charset_degree);
	createChar(1, charset_a1); // á
	createChar(2, charset_e1); // é
	createChar(3, charset_i1); // í
	createChar(4, charset_o1); // ó
	createChar(5, charset_o3); // õ
	createChar(6, charset_u1); // ú
	createChar(7, charset_u3); // û
	// 0x5f = ü
	// 0xfe = ö
	clear();
	setCursor(0, 0);

}

void LCD::replaceChars(char * to, const char * from)
{
	memcpy((void *) to, (void *) from, 16);
	for (uint8_t i = 0; i < 16; i++)
	{
		switch (to[i])
		{
		case '~':
			to[i] = 0;
			break;
		case '|':
			to[i] = 0x0df;
			break;
		case '}':
			to[i] = 0x07e;
			break;
		case '{':
			to[i] = 0x07f;
			break;
		case 'á':
			to[i] = 1;
			break;
		case 'é':
			to[i] = 2;
			break;
		case 'í':
			to[i] = 3;
			break;
		case 'ó':
			to[i] = 4;
			break;
		case 'õ':
			to[i] = 5;
			break;
		case 'ú':
			to[i] = 6;
			break;
		case 'û':
			to[i] = 7;
			break;
		case 'ü':
			to[i] = 0x0f5;
			break;
		case 'ö':
			to[i] = 0x0ef;
			break;
		}
	}
}

void LCD::clearBuffer()
{
	memset(lcdbuff[0], 32, 16);
	memset(lcdbuff[1], 32, 16);
}

void LCD::setText(uint8_t col, uint8_t row, const char * txt)
{
	char * buff;
	if (row == 0)
	{
		buff = lcdbuff[0];
	}
	else
	{
		buff = lcdbuff[1];
	}
	uint8_t i0 = 0;
	for (uint8_t i1 = col; i1 < 16; i1++)
	{
		if (!txt[i0])
		{
			break;
		}
		buff[i1] = txt[i0];
		i0++;
	}
}

void LCD::center(uint8_t row, const char * txt)
{
	char * buff;
	if (row == 0)
	{
		buff = lcdbuff[0];
	}
	else
	{
		buff = lcdbuff[1];
	}
	size_t l = strlen(txt);
	uint8_t p = 0;
	uint8_t i = 0;
	if (l > 16)
	{
		p = (l >> 1) - 8;
	}
	else
	{
		i = (16 - l) >> 1;
	}
	for (; i < 16; i++)
	{
		if (!txt[p])
		{
			break;
		}
		buff[i] = txt[p];
		p++;
	}
}

void LCD::right(uint8_t row, const char * txt)
{
	char * buff;
	if (row == 0)
	{
		buff = lcdbuff[0];
	}
	else
	{
		buff = lcdbuff[1];
	}
	size_t l = strlen(txt);
	uint8_t p = 0;
	uint8_t i = 0;
	if (l > 16)
	{
		p = l - 16;
	}
	else
	{
		i = 16 - l;
	}
	for (; i < 16; i++)
	{
		if (!txt[p])
		{
			break;
		}
		buff[i] = txt[p];
		p++;
	}
}

void LCD::show()
{
	replaceChars(lcdp[0], lcdbuff[0]);
	replaceChars(lcdp[1], lcdbuff[1]);
	setCursor(0, 0);
	write((uint8_t *) lcdp[0], (size_t) 16);
	setCursor(0, 1);
	write((uint8_t *) lcdp[1], (size_t) 16);

}


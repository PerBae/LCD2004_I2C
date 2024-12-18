/*
 * Display.cpp
 *
 *  Created on: Dec 15, 2024
 *      Author: Per
 *
 *  Purpose: Be an interface to the LCD2004 with I2C interface.
 *  		 This version was created for MCUs like NUCLEO-C031C6 from ST.
 *
 *	Code taken from https://github.com/Ckath/lcd2004_i2c and modified
 *	Copyright (c) 2021 Ckat
 *	Copyright (c) of C++ version 2024 Per
 *
 *	Doc on https://www.handsontec.com/dataspecs/I2C_2004_LCD.pdf, http://www.liquidcrystaltechnologies.com/PDF/ks0066.pdf
 *	and https://www.nxp.com/docs/en/data-sheet/PCF8574_PCF8574A.pdf
 *
 *
 *	Pin connections between I2C chip and display:
 *		PCF8574		Display
 *			P0		RS	Register Select, 1 to access internal RAM
 *			P1		RW	Read(1)/Write(0)
 *			P2		E	Enable read and write
 *			P3		BT	Backlight on 1.
 *			P4		D4
 *			P5		D5
 *			P6		D6
 *			P7		D7
 *
 */

#include "stm32c0xx_hal.h"
#include <string.h>
#include <stdio.h>
#include "Display.h"

#define I2C_DISPLAY_ADDRESS (0x27 << 1)

/* general commands */
#define LCD_CLEARDISPLAY 0x01
#define LCD_CURSORSHIFT 0x10
#define LCD_DISPLAYCONTROL 0x08
#define LCD_ENTRYMODESET 0x04
#define LCD_FUNCTIONSET 0x20
#define LCD_RETURNHOME 0x02
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80
/* entrymode flags */
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00
/* displaycontrol flags */
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00
#define LCD_BACKLIGHT 0x08
#define LCD_NOBACKLIGHT 0x00
/* cursorshift flags */
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00
/* functionset flags */
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

#define LCD_ROW0 0x80
#define LCD_ROW1 0xC0
#define LCD_ROW2 0x94
#define LCD_ROW3 0xD4
#define LCD_ENABLE_BIT 0x04

/* write modes for lcd_send_byte */
#define LCD_CMD 0
#define LCD_CHR 1

#define lcd_send_cmd(cmd) lcd_send_byte(cmd, LCD_CMD)
#define lcd_send_char(chr) lcd_send_byte(chr, LCD_CHR)
#define lcd_off() lcd_send_cmd(LCD_DISPLAYCONTROL | LCD_DISPLAYOFF)
#define lcd_on() lcd_send_cmd(LCD_DISPLAYCONTROL | LCD_DISPLAYON)
#define lcd_clear() do { lcd_send_cmd(LCD_CLEARDISPLAY); lcd_send_cmd(LCD_RETURNHOME); } while (0)

/*********************************************
 *  Constructor
 *
 *  hi2c: pointer to the I2C handle.
 *********************************************/
Display::Display(I2C_HandleTypeDef *hi2c) : m_hi2c(hi2c), m_backLight(LCD_BACKLIGHT),m_init(false)
{
	lcd_init();
}

/*********************************************
 *  Destructor
 *
 *********************************************/
Display::~Display()
{
}


/*********************************************
 *  Send
 *
 * pData: points to string to send
 *********************************************/
bool Display::Send(const char *pData)
{
	bool ok=true;

	for (unsigned int i = 0; i < strlen(pData); ++i)
	{
		if(!lcd_send_char((uint8_t)pData[i]))
			ok=false;
	}

	return ok;
}

/*********************************************
 *  Clear display
 *
 *********************************************/
void Display::Clear()
{
	lcd_send_cmd(LCD_CLEARDISPLAY);
	HAL_Delay(1);
	lcd_send_cmd(LCD_RETURNHOME);
	HAL_Delay(1);
}

/*********************************************
 *  write a char on I2C bus
 *
 *********************************************/
bool Display::i2c_write(uint8_t b)
{
	HAL_StatusTypeDef ok = HAL_I2C_Master_Transmit(m_hi2c, I2C_DISPLAY_ADDRESS, &b, 1, 20);

	return ok == HAL_OK;
}

/*******************************************************
 * 	lcd_init
 *
 ******************************************************/
void Display::lcd_init()
{
	/* required magic taken from pi libs */
	bool ok1=lcd_send_cmd(0x03);
	bool ok2=lcd_send_cmd(0x03);
	bool ok3=lcd_send_cmd(0x03);
	bool ok4=lcd_send_cmd(0x02); //return home

	bool ok5=lcd_on();
	lcd_clear();

	m_init = ok1 && ok2 && ok3 && ok4 && ok5;
}

/*******************************************************
 * 	send_nibble
 *
 * val: what to send
 ******************************************************/
bool Display::send_nibble(uint8_t val)
{
	bool ok1=i2c_write(val);
	//delay 40ns
	bool ok2=i2c_write(val  | LCD_ENABLE_BIT);	//enable after RS & RW
	//delay 230ns
	HAL_Delay(1);
	bool ok3=i2c_write(val);					//disable after RS & RW
	//delay 40ns
	return ok1 && ok2 && ok3;
}

/*******************************************************
 * 	lcd_send_byte
 *
 * val: what to send
 * mode:	LCD_CMD or LCD_CHR
 ******************************************************/
bool Display::lcd_send_byte(uint8_t val, uint8_t mode)
{
	/* write first nibble */
	uint8_t buf = mode | (val & 0xF0) | m_backLight;
	bool ok1=send_nibble(buf);

	/* write second nibble */
	buf = mode | ((val << 4) & 0xF0) |  m_backLight;
	bool ok2=send_nibble(buf);

	HAL_Delay(1);

	return ok1 && ok2;
}


/*********************************************
 *
 * 	Move cursor
 *
 *  x: x-coordinate
 *  y: LINE1 to LINE4, LINE1 is at top
 *********************************************/
void Display::Move(int x, LINE y)
{
	uint8_t pos_addr = x;

	switch(y)
	{
		case LINE1:
			pos_addr += LCD_ROW0;
			break;
		case LINE2:
			pos_addr += LCD_ROW1;
			break;
		case LINE3:
			pos_addr += LCD_ROW2;
			break;
		case LINE4:
			pos_addr += LCD_ROW3;
			break;
		default:
		{
			char buf[128];
			sprintf(buf,"Illegal choice %d in Move",y);
			Error(buf);
		}
	}
	lcd_send_cmd(pos_addr);
}

/*********************************************
 *
 * 	Error message
 *
 * msg: what to print
 *********************************************/
void Display::Error(const char * msg)
{
	Move(0,LINE4);
	Send(msg);
}

/*
 * MIT License

Copyright (c) 2021 Ckat

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 * */


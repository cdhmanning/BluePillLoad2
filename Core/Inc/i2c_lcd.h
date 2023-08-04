#ifndef __I2C_LCD_H__
#define __I2C_LCD_H_

#include "main.h"

struct i2c_lcd {
	I2C_HandleTypeDef	*hi2c;
	uint8_t				i2c_addr;
	uint8_t 		  	rows;
	uint8_t				function;
	uint8_t 		  	control;
	uint8_t 		  	mode;
	uint8_t 		  	backlight;
};

/* Command */
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

/* Entry Mode */
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

/* Display On/Off */
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

/* Cursor Shift */
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

/* Function Set */
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

/* Backlight */
#define LCD_BACKLIGHT 0x08
#define LCD_NOBACKLIGHT 0x00

/* Enable Bit */
#define ENABLE 0x04

/* Read Write Bit */
#define RW 0x0

/* Register Select Bit */
#define RS 0x01

/* Device I2C Address */

void HD44780_Init(struct i2c_lcd *lcd, I2C_HandleTypeDef *bus, uint8_t addr_7bits, uint8_t rows);
void HD44780_Clear(struct i2c_lcd *lcd);
void HD44780_Home(struct i2c_lcd *lcd);

void HD44780_SetDisplayVisible(struct i2c_lcd *lcd, int val);
void HD44780_SetCursorVisible(struct i2c_lcd *lcd, int val);
void HD44780_SetBlink(struct i2c_lcd *lcd, int val);

void HD44780_ScrollDisplayLeft(struct i2c_lcd *lcd);
void HD44780_ScrollDisplayRight(struct i2c_lcd *lcd);
void HD44780_PrintLeft(struct i2c_lcd *lcd);
void HD44780_PrintRight(struct i2c_lcd *lcd);
void HD44780_LeftToRight(struct i2c_lcd *lcd);
void HD44780_RightToLeft(struct i2c_lcd *lcd);
void HD44780_ShiftIncrement(struct i2c_lcd *lcd);
void HD44780_ShiftDecrement(struct i2c_lcd *lcd);
void HD44780_AutoScroll(struct i2c_lcd *lcd);
void HD44780_NoAutoScroll(struct i2c_lcd *lcd);

void HD44780_CreateSpecialChar(struct i2c_lcd *lcd, uint8_t, uint8_t[]);
void HD44780_PrintSpecialChar(struct i2c_lcd *lcd, uint8_t);

void HD44780_SetCursor(struct i2c_lcd *lcd, uint8_t, uint8_t);
void HD44780_SetBacklight(struct i2c_lcd *lcd, uint8_t new_val);
void HD44780_LoadCustomCharacter(struct i2c_lcd *lcd, uint8_t char_num, uint8_t *rows);
void HD44780_PrintStr(struct i2c_lcd *lcd, const char[]);

#endif

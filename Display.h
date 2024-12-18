/*
 * Display.h
 *
 *  Created on: Dec 15, 2024
 *      Author: Per
 */

#ifndef DISPLAY_H_
#define DISPLAY_H_

#include "stm32c0xx_hal_i2c.h"
#include "stm32c0xx_hal_def.h"

enum LINE {LINE1=0,LINE2=1,LINE3=2,LINE4=3};

class Display
{
public:
	Display(I2C_HandleTypeDef *hi2c);
	virtual ~Display();
	bool Send(const char *pData);
	void Move(int x, LINE y);
	void Clear();
	void Error(const char * msg);
	bool InitOK(void)
	{
		return m_init;
	}

private:
	Display();
	bool i2c_write(uint8_t b);
	void lcd_init();
	bool lcd_send_byte(uint8_t val, uint8_t mode);
	bool lcd_write(char *data);
	bool send_nibble(uint8_t val);

	I2C_HandleTypeDef *m_hi2c;
	uint8_t m_backLight;
	bool m_init;
};


#endif /* DISPLAY_H_ */

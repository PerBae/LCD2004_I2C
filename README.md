This version was created for MCUs like NUCLEO-C031C6 from ST.
I have used the STM32CubeIDE to generate the driver code.

## functions
	//constructor that needs a handle to the I2C driver.
	Display(I2C_HandleTypeDef *hi2c);
	virtual ~Display();

	//Send a string of ASCII chars
	bool Send(const char *pData);

	//Place the cursor
	void Move(int x, LINE y);

	//Erase screen
	void Clear();

	//Uses last line for error text
	void Error(const char * msg);

	//To check i initialization went ok
	bool InitOK(void)

## example usage

#include "Display.h"

	I2C_HandleTypeDef hi2c1;

	Display disp(&hi2c1);

	disp.Move(5, LINE2);
	disp.Send("line 2");
	disp.Error("Error");

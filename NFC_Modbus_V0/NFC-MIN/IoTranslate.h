/*=====================================================================================
File name:    IoTranslate.h
                    
Originator:   Sun Ran

Description:  Hardware Abstraction Layer (HAL)

Multi-Instance: (not applicable)

Class Methods:

Peripheral Resources:
  
IoTranslate requirements:

Other requirements:
  
=======================================================================================
 History:	
*-------*-----------*---------------------------------------------------*--------------
1.00	06-25-2015  New File											Sun Ran
1.01	11-04-2019	Adapted from CPAM								Anish Venkataraman
1.02	12-09-2019	Removed change of baud rate						Anish Venkataraman
1.03	06-26-2020	Modified macro for I2C delay and UART init		Anish Venkataraman
---------------------------------------------------------------------------------------
*/
#ifndef IOTRANSLATE_H_
#define IOTRANSLATE_H_
#include <driver_init.h>
#include "usart.h"
#include "atmel_start_pins.h"
#include "Build.h"

//UART
#define RX_BUFFER_LENGTH	255
#define ERR					 1
#define NO_ERR				 0
uint8_t USART_RxChar(unsigned char *val);
void USART_SendChar(unsigned char *str);
uint8_t USART_GetCharsInRxBuf(void);
void USART_ClearRxBuffer(void);
void USART_StoreData(void);
void USART_SetTxInterrupt(void);

//GPIO
#define HeartBeatLED_Blink()			LED_toggle_level()

//I2C
void SDA_Write(unsigned char val);
uint8_t SDA_Read(void);
void Hardware_Delay();
void IoTranslate_SendBytes(uint16_t address, uint8_t * data, uint8_t length);

#define NFC								15000 //delay value for 5ms
#define I2C								1
#define TWO_MSEC						2
#define I2C_SDA_OUTPUT()				SDA_set_dir(PORT_DIR_OUT);//Set SDA as output
#define I2C_SCL_OUTPUT()				//Set the SCL as output
#define I2C_SCL_SetVal()				SCL_set_level(TRUE)//Set SCL to high
#define I2C_SDA_SetVal()				SDA_set_level(TRUE)//Set SDA line to high
#define I2c_Delay()						Hardware_Delay(I2C)//I2c Delay
#define I2C_SDA_ClrVal()				SDA_set_level(FALSE)//Set SDA line to low
#define I2C_SCL_ClrVal()				SCL_set_level(FALSE)//Set SCL line to low
#define I2C_SDA_GetVal()				SDA_Read()//Get value from SDA line
#define I2C_SDA_INPUT()					SDA_set_dir(PORT_DIR_IN);//Set SDA as input to read
#define I2C_SDA_PutVal(x)				SDA_Write(x)//Send data


//MIN
#define HardwareUart_Init()				USART_0_init()
#define HardwareUart_RecvChar(x)		USART_RxChar(&x)
#define HardwareUart_SendChar(x)		USART_SendChar(&x)
#define HardwareUart_GetCharsInRxBuf() (uint8_t)USART_GetCharsInRxBuf()
#define HardwareUart_clearRxBuf()		USART_ClearRxBuffer()
#define minRxEnable()					Tx_Enable_set_level(FALSE)
#define minTxEnable()					Tx_Enable_set_level(TRUE)
#define MinTurnAroundDelayMAC()			Hardware_Delay(TWO_MSEC)
#define Enable_TXInterrupt()			USART_SetTxInterrupt()

//ISR
void DriverISR_clearParseFLF(void);
void DriverISR_setParseFLF(void);

#endif /* IOTRANSLATE_H_ */
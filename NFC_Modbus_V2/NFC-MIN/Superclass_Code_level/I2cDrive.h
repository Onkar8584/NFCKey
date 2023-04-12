/*
=======================================================================================
File name:    I2cDrive.h                 
                    
Originator:   Sun Ran

Description:

The I2C driver which using the GPIO simulated also known as bit-banged

Multi-Instance: No

Class Methods:
I2cDrive_Start()				Called from NFC.c
I2cDrive_Stop()					Called from NFC.c
I2cDrive_ReceiveByte			Called from NFC.c
I2cDrive_SendByte				Called from NFC.c
I2cDrive_AckSend()
I2cDrive_NAckSend()

Peripheral Resources:

IoTranslate requirements:
SDA_set_dir(PORT_DIR_OUT);		//Set SDA as output
SCL_set_level(TRUE)				//Set SCL to high
SDA_set_level(TRUE)				//Set SDA line to high
Hardware_Delay(I2C)				//I2c Delay
SDA_set_level(FALSE)			//Set SDA line to low
SCL_set_level(FALSE)			//Set SCL line to low
SDA_Read()						//Get value from SDA line
SDA_set_dir(PORT_DIR_IN);		//Set SDA as input to read
SDA_Write(x)					//Send dataOther requirements:



=======================================================================================
 History:	(Identify methods that changed)
*-------*-----------*---------------------------------------------------*--------------
1.00    4-24-2019   New File											  Sun Ran
1.01	11-04-2019	Modified slave address for NFC					Anish Venkataraman
 ---------------------------------------------------------------------------------------
*/
#ifndef I2CDRIVE_H_
#define I2CDRIVE_H_
#include "IoTranslate.h"


#define TIMEOUT        5  // trials waiting ACK
#define NFC_ADDRESS_Write 0xA6
#define NFC_ADDRESS_Read 0xA7
/*
=======================================================================================
Method name:  I2cDrive_Start()

Originator:   Sun Ran

Description:  Place an I2C start condition
           
                    ____      
              SDA       |    
                        |___
                    ________
              SCL   

=======================================================================================
 History:
*-------*-----------*---------------------------------------------------*--------------
1.00    4-24-2019   Original code                                       Sun Ran
---------------------------------------------------------------------------------------
*/
extern void I2cDrive_Start(void);


/*
=======================================================================================
Method name:  i2c_Stop()

Originator:   Sun Ran

Description:  Place an I2C stop condition
           
                         ___
              SDA       |    
                    ____|
                    ________
              SCL   

=======================================================================================
 History:
*-------*-----------*---------------------------------------------------*--------------
1.00    4-24-2019   Original code                                       Sun Ran
---------------------------------------------------------------------------------------
*/
extern void I2cDrive_Stop(void);


/*
=======================================================================================
Method name:  I2cDrive_AckSend()

Originator:   Sun Ran

Description:  Place an ACK signal after one byte data received
           
                    ____          ____
              SDA       |        |
                        |________|
                      ____      ____
              SCL    |    |    |    |
                    _|    |____|    |_
=======================================================================================
 History:
*-------*-----------*---------------------------------------------------*--------------
1.00    4-24-2019   Original code                                       Sun Ran
---------------------------------------------------------------------------------------
*/
extern void I2cDrive_AckSend(void);


/*
=======================================================================================
Method name:  I2cDrive_NAckSend()

Originator:   Sun Ran

Description:  Place an NACK signal after one byte data received
           
                    __________________
              SDA      
                        
                      ____      ____
              SCL    |    |    |    |
                    _|    |____|    |_
=======================================================================================
 History:
*-------*-----------*---------------------------------------------------*--------------
1.00    4-24-2019   Original code                                       Sun Ran
---------------------------------------------------------------------------------------
*/
extern void I2cDrive_NAckSend(void);


/*
=======================================================================================
Method name:  I2cDrive_ReceiveByte()

Originator:   Sun Ran

Description:  Receive one byte data
           

=======================================================================================
 History:
*-------*-----------*---------------------------------------------------*--------------
1.00    4-24-2019   Original code                                       Sun Ran
---------------------------------------------------------------------------------------
*/
extern uint8_t I2cDrive_ReceiveByte(void);


/*
=======================================================================================
Method name:  I2cDrive_SendByte()

Originator:   Sun Ran

Description:  Send one byte data

Input:  data - the data which is about to be sent
           
Return: err - did not get the ACK back

=======================================================================================
 History:
*-------*-----------*---------------------------------------------------*--------------
1.00    4-24-2019   Original code                                       Sun Ran
---------------------------------------------------------------------------------------
*/
extern uint8_t I2cDrive_SendByte(uint8_t data);

#endif /* I2CDRIVE_H_ */
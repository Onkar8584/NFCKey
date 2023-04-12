/*=====================================================================================
File name:    I2cDrive.c

Originator:   Sun Ran

Description:  The I2C driver which using the GPIO simulated

=======================================================================================
History:
*-------*-----------*--------------------------------------------------*---------------
1.00    4-24-2019   New File                                           Sun Ran
---------------------------------------------------------------------------------------
*/
#include "I2cDrive.h"

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
void I2cDrive_Start ( void )
{
    I2C_SDA_OUTPUT();
    I2C_SCL_OUTPUT();
    I2C_SCL_SetVal();
    I2C_SDA_SetVal();
    I2c_Delay();   
    I2C_SDA_ClrVal();
    I2c_Delay();
    I2C_SCL_ClrVal();
	
}

/*
=======================================================================================
Method name:  I2cDrive_Stop()

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
void I2cDrive_Stop ( void )
{
    I2C_SCL_ClrVal();
    I2C_SDA_OUTPUT();
    I2C_SDA_ClrVal();
    I2c_Delay();
    I2C_SCL_SetVal();
    I2c_Delay();
    I2C_SDA_SetVal();
	

}

/*
=======================================================================================
Method name:  I2cDrive_Ack()

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
void I2cDrive_AckSend ( void )
{
    I2C_SDA_OUTPUT();
    I2C_SDA_ClrVal();
    I2C_SCL_SetVal(); 
    I2c_Delay();    
    I2C_SCL_ClrVal();
    I2c_Delay();
}

/*
=======================================================================================
Method name:  I2cDrive_NAck()

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
void I2cDrive_NAckSend ( void )
{
    I2C_SDA_OUTPUT();
    I2C_SDA_SetVal();
    I2C_SCL_SetVal(); 
    I2c_Delay();    
    I2C_SCL_ClrVal();
    I2c_Delay();
}

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
uint8_t e = 0;
uint8_t et = 1;
uint8_t I2cDrive_SendByte ( uint8_t data )
{
    uint8_t cnt;
    uint8_t data_buffer = data;
    uint8_t timeOut = TIMEOUT;
    uint8_t err = 0;
	
    I2C_SDA_OUTPUT();
    for(cnt = 0; cnt < 8; cnt++) {
        I2C_SCL_ClrVal();                    
        I2C_SDA_PutVal(data_buffer & 0x80); // Set or clear SDA line before SCL line turned high
        I2c_Delay();
        I2C_SCL_SetVal();
        I2c_Delay();
        data_buffer <<= 1;
    }
    I2C_SCL_ClrVal();
    I2c_Delay();
	I2C_SDA_SetVal();
    I2C_SCL_SetVal();
    I2C_SDA_INPUT();
	I2c_Delay();
	    // Wait for the ACK signal
	 do{
    	timeOut--;
		} while(I2C_SDA_GetVal() && timeOut);
    
	I2C_SCL_ClrVal();
	I2c_Delay();	
    if(timeOut == 0) {
        err = 1;
    }

    return err;
}

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
uint8_t I2cDrive_ReceiveByte ( void )
{
    uint8_t data = 0;
    uint8_t mask = 0;
    I2C_SDA_INPUT();
    for(mask = 0x80; mask > 0; mask >>= 1) {
        I2C_SCL_SetVal();
        I2c_Delay();
        if(I2C_SDA_GetVal()) {  // Read SDA line when SCL line is high
            data |= mask;
        }

        I2C_SCL_SetVal();
        I2c_Delay();
        I2C_SCL_ClrVal();
        I2c_Delay();     
    }
	
    return data;
}
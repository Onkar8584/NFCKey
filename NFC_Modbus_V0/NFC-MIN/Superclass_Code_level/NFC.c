/*
=======================================================================================
File name:    NFC.c                  

Originator:   Anish Venkataraman



=======================================================================================
 History:	(Identify methods that changed)
 *-------*-----------*---------------------------------------------------*--------------
1.0		9-10-2019	Initial Code									 Anish Venkataraman
1.01	11-21-2019	Modified name of methods and created two new	 Anish Venkataraman
					methods that act as helper to the methods. 
1.02	06-04-2020  Re-factored code, removed NFC_Write function,	 Anish Venkataraman
					created a new method for init and instantaneous
					write 
1.03	06-15-2020  Added support for writing BLK226				 Anish Venkataraman
					NFC_SequentilaWrite() 1.02	
1.04	06-16-2020  Added helper function for reducing code size	 Anish Venkataraman
					NFC_SequentilaWrite() 1.03	
					NFC_InstantaneousWrite() 1.01	
					NFC_WriteDataPRIV() 1.00	
1.05	06-26-2020  NFC_SequentilalRead() 1.00						 Anish Venkataraman
					NFC_SequentilaWrite() 1.04
					Removed NFC_read()	 
1.06	07-07-2020  NFC_SequentilalRead() 1.01						 Anish Venkataraman
1.07	08-12-2020  Removed NFC_WriteDataPRIV() 					 Anish Venkataraman
					NFC_SequentialWrite() 1.05
					NFC_Init() 1.01,NFC_InstantaneousWrite() 1.02
1.08	08-28-2020  Removed NFC_SequentialWrite() 					 Anish Venkataraman
					Added NFC_PageWrite(),NFC_getBroadcastBlkAddress()
					NFC_getUpdateBlkAddress() 
1.09	09-10-2020  Updated NFC_Init() 1.02		 					 Anish Venkataraman
 ---------------------------------------------------------------------------------------
 */
#include "I2cDrive.h"
#include "MinSlave.h"
#include "stdint-gcc.h"
#include "NFC.h"


extern MinSlave_STYP oMinSlave;


/*=======================================================================================
Method name:  NFC_WriteByte(uint16_t address, unsigned char data)

Originator:   Anish Venkataraman

Description:It takes a 16-bit address and a one byte of data. It performs a 
			random read access to the NFC chip. This process is done as follows
			1. Send I2C Start Signal
			2. Slave Select + R/W(set to 0) -> Device Address
			3. Send MSB-Byte address to be read followed by LSB-Byte address.
			4. Send data by calling I2cDrive_SendByte method(function).
			5.Send I2C Stop Signal


=======================================================================================
History:
 *-------*-----------*---------------------------------------------------*--------------
1.00    9-10-2019   Original code                                   Anish Venkataraman
1.01	11-21-2019	Changed the name of the function from NFC_Write Anish Venkataraman
					to NFC_WriteByte
---------------------------------------------------------------------------------------*/
void NFC_WriteByte(uint16_t address, unsigned char data) {
	I2cDrive_Start();
	(void)I2cDrive_SendByte(NFC_ADDRESS_Write);
	(void)I2cDrive_SendByte(address >> MSB_MASK);
	(void)I2cDrive_SendByte(address & LSB_MASK);
	(void)I2cDrive_SendByte(data);
	I2cDrive_Stop();
}


/*=======================================================================================
Method name:uint8_t NFC_InstantaneousWrite(NFC_STYP *nfc, uint16_t address, uint16_t data)
Originator:   Anish Venkataraman

Description: This method writes a 2 bytes of data for block 2 and block 11 data. 


=======================================================================================
History:
 *-------*-----------*---------------------------------------------------*--------------
1.00    06-04-2019   Original code                                   Anish Venkataraman
1.01    06-16-2020  Added a helper function to reduce code size      Anish Venkataraman
1.02    08-12-2020  Modified the parameter passed to the method and  Anish Venkataraman
					refactored the code
1.03    08-28-2020  Function was returning nothing so modified it to  Anish Venkataraman
					void
---------------------------------------------------------------------------------------*/

void NFC_InstantaneousWrite(NFC_STYP *nfc, uint16_t address, uint16_t data) {
	I2cDrive_Start();
	(void)I2cDrive_SendByte(NFC_ADDRESS_Write);
	(void)I2cDrive_SendByte(address >> MSB_MASK);
	(void)I2cDrive_SendByte(address & LSB_MASK);
	//Store Block 2 register data
//	(void)I2cDrive_SendByte(data << 8);
	(void)I2cDrive_SendByte(data >> 8);
	(void)I2cDrive_SendByte((uint8_t)data);
	I2cDrive_Stop();
}
/*=======================================================================================
Method name:  NFC_ReadByte(uint16_t address)

Originator:   Anish Venkataraman

Description: It takes a 16-bit address and returns one byte of data. It performs a 
			random read access to the NFC chip. This process is done as follows
			1. Send I2C Start Signal
			2. Slave Select + R/W(set to 0) -> Device Address
			3. Send MSB-Byte address to be read followed by LSB-Byte address.
			4. Send another I2C Start Signal.
			5. Slave Select + R/W(set to 1) -> Device Address
			6. Read the SDA line to receive data
			7. Send I2C Stop Signal

=======================================================================================
History:
 *-------*-----------*---------------------------------------------------*--------------
1.00    9-10-2019   Original code                                   Anish Venkataraman
---------------------------------------------------------------------------------------*/
uint8_t NFC_ReadByte(uint16_t address) {
	uint8_t data = 0;
	I2cDrive_Start();
	(void)I2cDrive_SendByte(NFC_ADDRESS_Write);
	(void)I2cDrive_SendByte(address >> MSB_MASK);
	(void)I2cDrive_SendByte(address & LSB_MASK);
	I2cDrive_Start();
	(void)I2cDrive_SendByte(NFC_ADDRESS_Read);
	data = I2cDrive_ReceiveByte();
	I2cDrive_NAckSend();
	I2cDrive_Stop();
	return data;
}

/*=======================================================================================
Method name:  NFC_CurrentRead()

Originator:   Anish Venkataraman

Description:The method returns a returns one byte of data. It performs a 
			current read access to the NFC chip. This process is done as follows
			1. Send I2C Start Signal
			2. Slave Select + R/W(set to 1) -> Device Address
			3. Read the SDA line to receive data
			4. Send I2C Stop Signal

Note: It has a internal counter that increments after each current read.
=======================================================================================
History:
 *-------*-----------*---------------------------------------------------*--------------
1.00    9-10-2019   Original code                                   Anish Venkataraman
---------------------------------------------------------------------------------------*/

uint8_t NFC_CurrentRead(void){
	uint8_t data = 0;
	I2cDrive_Start();
	(void)I2cDrive_SendByte(NFC_ADDRESS_Read);
	data = I2cDrive_ReceiveByte();
	I2cDrive_Stop();
	return data;
}


/*=======================================================================================
Method name:  NFC_init()

Originator:   Anish Venkataraman

Description: Initialization method for NFC. Reads the configuration code from NFC and
			 stores the value in the nfc object and Minslave object as well.	 


=======================================================================================
History:
 *-------*-----------*---------------------------------------------------*--------------
1.00    11-21-2019   Original code                                   Anish Venkataraman
1.01    08-12-2020   After reading config code set the EEPROM_INIT   Anish Venkataraman
					 to 0x5a5a
1.02    09-10-2020   Updated the NFC config assignment LSB first     Anish Venkataraman
					 and then MSB
---------------------------------------------------------------------------------------*/

void NFC_init(NFC_STYP *nfc) {
	nfc->configuration = NFC_ReadByte(NFC_CONFIG_CODE_MSB) << 8;			//MSB location for configuration
	nfc->configuration |= NFC_ReadByte(NFC_CONFIG_CODE_LSB);				//LSB location for configuration
	//ToDo create a look up table for checking if the config code is valid or not
	if(nfc->configuration > 0){
		oMinSlave.slaveRegisters[MIN_SLAVE_MODEL_CONFIGURATION_CODE] = nfc->configuration; //save configuration
		oMinSlave.slaveRegisters[MIN_SLAVE_EEPROM_INIT_VALUE_RNUM] = INITIALIZED; 
	}
	else{
		oMinSlave.slaveRegisters[MIN_SLAVE_MODEL_CONFIGURATION_CODE] = 0; 
		oMinSlave.slaveRegisters[MIN_SLAVE_EEPROM_INIT_VALUE_RNUM] = 0;
	}
}

/*=======================================================================================
Method name:  NFC_SequentialRead

Originator:   Anish Venkataraman

Description: It takes a 16-bit address and a pointer to the sequence of data. It performs a
sequential read access to the NFC chip. This process is done as follows
1. Send I2C Start Signal
2. Slave Select + R/W(set to 0) -> Device Address
3. Send MSB-Byte address to be read followed by LSB-Byte address.
4. Start signal along with Device address R/W set to 1
5. Receive data by calling I2cDrive_ReceiveByte method(function).
6. Receive data for the specified length
7. Send I2C Stop Signal

Note: Max 256 characters can be written in one sequential write command
=======================================================================================
History:
 *-------*-----------*-----------------------------------------------*-------------------
1.00    06-26-2020   Original code                                   Anish Venkataraman
1.01    07-07-2020   Removed NFC_STYP parameter passed				 Anish Venkataraman
---------------------------------------------------------------------------------------*/

void NFC_SequentialRead(uint16_t address, uint8_t length, uint8_t *data) {
	uint8_t i;
	I2cDrive_Start();
	(void)I2cDrive_SendByte(NFC_ADDRESS_Write);
	(void)I2cDrive_SendByte(address >> MSB_MASK);
	(void)I2cDrive_SendByte(address & LSB_MASK);
	//Read Data
	I2cDrive_Start();
	(void)I2cDrive_SendByte(NFC_ADDRESS_Read);
	for(i = 0; i < length; i++){
		*data++ = I2cDrive_ReceiveByte();
		if(i < length-1) {
			I2cDrive_AckSend();	//ACK all the bytes received
		}
		else{
			I2cDrive_NAckSend();//NAK the last byte received
		}
	}
	I2cDrive_Stop();
}

/*=======================================================================================
Method name:  NFC_getUpdateBlkAddress()

Originator:   Anish Venkataraman

Description: Getter method for obtaining the address for the block to update 	 


=======================================================================================
History:
 *-------*-----------*---------------------------------------------------*--------------
1.00    08-28-2020   Original code                                   Anish Venkataraman
---------------------------------------------------------------------------------------*/
uint16_t NFC_getUpdateBlkAddress(NFC_STYP *nfc){
	/*
	if(nfc->update.blockNumber == BLOCK2){
		return (BLOCK2_OFFSET + (nfc->update.registerNumber *2));
	}
	else if(nfc->update.blockNumber == BLOCK11){
		return (BLOCK11_OFFSET + (nfc->update.registerNumber *2));
	}
	else{
		return (BLOCK226_OFFSET + (nfc->update.registerNumber *2));
	}*/
	return (HOLDING_REG_OFFSET + (nfc->update.registerNumber *2));
}
/*=======================================================================================
Method name:  NFC_getBroadcastBlkAddress()

Originator:   Anish Venkataraman

Description: Getter method for obtaining the address for the block to update 	 


=======================================================================================
History:
 *-------*-----------*---------------------------------------------------*--------------
1.00    08-28-2020   Original code                                   Anish Venkataraman
---------------------------------------------------------------------------------------*/
uint16_t NFC_getBroadcastBlkAddress(NFC_STYP *nfc){
	/*
	if(nfc->broadcast.blockNumber == BLOCK2){
//		return (BLOCK2_OFFSET + (nfc->broadcast.registerNumber *2));
		return (HOLDING_REG_OFFSET + (nfc->broadcast.registerNumber *2));
	}
	else if(nfc->broadcast.blockNumber == BLOCK11){
//		return (BLOCK11_OFFSET + (nfc->broadcast.registerNumber *2));
		return (HOLDING_REG_OFFSET + (nfc->broadcast.registerNumber *2));
	}
	else{
//		return (BLOCK226_OFFSET + (nfc->broadcast.registerNumber *2));
		return (HOLDING_REG_OFFSET + (nfc->broadcast.registerNumber *2));
	}
	*/
	return (HOLDING_REG_OFFSET + (nfc->broadcast.registerNumber *2));
}

/*=======================================================================================
Method name:  NFC_PageWrite()

Originator:   Anish Venkataraman

Description: Method for writing one page i.e 32 bytes to the NFC
=======================================================================================
History:
 *-------*-----------*---------------------------------------------------*--------------
1.00    08-28-2020   Original code                                   Anish Venkataraman
---------------------------------------------------------------------------------------*/
void NFC_PageWrite(uint16_t address, uint8_t *data) {
	uint8_t i = 0;
	I2cDrive_Start();
	(void)I2cDrive_SendByte(NFC_ADDRESS_Write);
	(void)I2cDrive_SendByte(address >> MSB_MASK);
	(void)I2cDrive_SendByte(address & LSB_MASK);
	for(i = 0; i < BYTES_IN_PAGE; i++){
		I2cDrive_SendByte(*data++);
	}
	I2cDrive_Stop();
}
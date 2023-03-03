/*
=======================================================================================
File name:    NFC.h                 

Originator:   Anish Venkataraman

Description:

This module uses the I2cDrive driver module for communicating with the NFC chip.
This is a higher level module for the low level I2c driver.

Multi-Instance: No

Class Methods:
NFC_WriteByte(uint16_t address, unsigned char data)		//Called from Scheduler.c
NFC_ReadByte(uint16_t address);							//Called from Scheduler.c and MinSlave.c
NFC_CurrentRead();				
NFC_Write(NFC_STYP *nfc);								//Called from Scheduler.c
NFC_Read(NFC_STYP *nfc);								//Called from Scheduler.c
Peripheral Resources:
Assume as UART is available for 19,200 up to 115,200 baud

IoTranslate requirements:

Other requirements:



=======================================================================================
 History:	(Identify methods that changed)
 *-------*-----------*---------------------------------------------------*--------------
1.00	11-04-2019	Initial Code									 Anish Venkataraman
1.01	11-21-2019	Modified class structure and added macros		 Anish Venkataraman
					Added status and error enums
1.02	06-15-2020	Modified class structure and added macros		 Anish Venkataraman
					to support BLK 226
1.03	06-26-2020	Modified structure -refactored code				 Anish Venkataraman
1.04	07-07-2020	Removed NFC_WriteProduct_Info_FLG variable		 Anish Venkataraman
1.05	08-12-2020	Restructured structure to make it generic		 Anish Venkataraman
1.06	08-17-2020	Updated the NFC address location				 Anish Venkataraman
1.07	08-28-2020	Added new structure block_STYP and thus 		 Anish Venkataraman
					modified the nfc_STYP structure.
					Added NFC_PageWrite,NFC_getBroadcastBlkAddress
					NFC_getUpdateBlkAddress helper methods
1.08	09-10-2020	Updated the NFC_CONFIG_CODE_MSB & LSB address	 Anish Venkataraman
1.09	05-10-2021	Updated the NFC_CONFIG_CODE_MSB & LSB address	 Anish Venkataraman
					added productInfoData and length to the class
					structure and updated the defaults for the same.
 ---------------------------------------------------------------------------------------
 */

#ifndef NFC_H_
#define NFC_H_

#include "Build.h"


//Macros
#define UNKNOWN_WATER_HEATER	0x0000	//config code for unkown water heater

#define BLOCK2_OFFSET			0x120  //block 2 offset
#define BLOCK11_OFFSET			0x220	//block 11 offset
#define BLOCK226_OFFSET			0x320	//block 226 offset

#define HOLDING_REG_OFFSET		0x0020//0x120  //Input only register block offset(FC04)
//#define HOLDING_REG_OFFSET			0x520  //Holding register block offset(FC03)

#define BLOCK_NVM_OFFSET		0x020	//block NVM information offset
#define NFC_CONFIG_CODE_MSB		0x099	//MSB byte of config code
#define NFC_CONFIG_CODE_LSB		0x098	//LSB byte of config code
#define NFC_MAX_MEM				256	    //Max Memory
#define NFC_WRITE_TIME			5	    //5ms per write cycle
#define MSB_MASK 8
#define LSB_MASK 0xFF
#define MAX_INSTANT_WRITE_BYTES 8
#define BYTES_IN_PAGE			4


#define BLK_DEFAULTS	{0,0,0,{0}}
#define NFC_DEFAULTS				\
		{FALSE,FALSE,FALSE,FALSE,	\
		FALSE,FALSE,				\
		FALSE,						\
		UNKNOWN_WATER_HEATER,		\
		0,{0},						\
		BLK_DEFAULTS,BLK_DEFAULTS}			


//Class Structure
typedef struct{
	uint8_t blockNumber;
	uint16_t registerNumber;
	uint8_t length;
	uint8_t nfcBuffer[NFC_MAX_MEM];
}block_STYP;

typedef struct{
	bool block2WriteFLG;
	bool block11WriteFLG;
	bool block226WriteFLG;
	bool instantWriteFLG;
	bool productInfoFLG;
	bool storeBroadcastFLG;
	volatile bool schedulerNFCWriteWait;	//Set every 5ms
	uint16_t configuration;
	uint16_t productInfoLength;
	uint8_t productInfoData[NFC_MAX_MEM];
	block_STYP broadcast;
	block_STYP update;
}NFC_STYP;

//Public Methods for Class
void NFC_WriteByte(uint16_t address, unsigned char data);
void NFC_InstantaneousWrite(NFC_STYP *nfc, uint16_t address, uint16_t data);
uint8_t NFC_ReadByte(uint16_t address);
uint8_t NFC_CurrentRead(void);
byte NFC_Write(NFC_STYP *nfc);
void NFC_init(NFC_STYP *nfc);
void NFC_SequentialRead(uint16_t address, uint8_t length,uint8_t *data);
uint16_t NFC_getUpdateBlkAddress(NFC_STYP *nfc);
uint16_t NFC_getBroadcastBlkAddress(NFC_STYP *nfc);
void NFC_PageWrite(uint16_t address, uint8_t *data);
//Status
enum{
	NFC_IDLE_STATE = 0,
	NFC_ERROR_STATE,
	NFC_WRITE_WAIT_STATE,
	NFC_WRITE_CHECK_STATE,
	NFC_RETRY_STATE
};

//Error Write
enum{
	NO_WRITE_ERR = 0,
	NFC_WRITE_ERR,
};

//Error Read
enum{
	NO_READ_ERR = 0,
	NFC_READ_ERR
};

//Block Inf
enum{
	NFC_BLK_2 = 0,
	NFC_BLK_11,
	NFC_BLK_226
};


#endif /* NFC_H_ */
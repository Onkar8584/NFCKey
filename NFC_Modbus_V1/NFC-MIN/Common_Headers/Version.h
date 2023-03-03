/*=====================================================================================
File name:    Version.h

Originator:   Anish Venkataraman

Description:  Software Project Revision History

Multi-Instance: N/A

Instance Methods: N/A

Peripheral Resources: N/A

IoTranslate requirements: N/A

Other requirements: N/A

=====================================================================================
 History:
-*-----*-----------*--------------------------------------------------*--------------
 1.00  9-10-2019  NFC-key with MIN protocol						   Anish Venkataraman
-------------------------------------------------------------------------------------
*/

#ifndef VERSION_H_
#define VERSION_H_

#define VERSION			0x01	// 1
#define REVISION		0x03	// 03
#define BUILDREVISION	0x00    // 00
#define FIRMWARE_VERSION_REVISION     ((VERSION << 8) + REVISION)  // i.e. 1.00.00



/*
Update firmware version information here (not above) Latest information first.

Date          FirmWare  File Changed      		File V/R  	Programmer        	Description
*---------------*-------*-----------------------*-----------*-------------------*---------------------------------------------------------------------------------------
05-10-2021		1.03.00	Scheduler.c				1.05		Anish Venkataraman	Replaced references of productInfoNVM structure with NFC super class structure.
						MinSlave.c				1.14							Updated FC69 and 70 methods to support writing any size of productInfo data
						NFC.h					1.09							Added 
						
02-19-2021		1.02.00	MinSlave.c				1.13		Anish Venkataraman	Added check for permission to slave
						driver_isr.c											Updated the schedulerwaitCntr variable intial value to 0 from 1.

01-08-2021		1.01.00	Scheduler.c				1.05		Anish Venkataraman	Updated code to only write after permission to save has been received from TRC.

09-??-2020		1.00.14										Anish Venkataraman	Software release for field test
08-28-2020		1.00.13 NFC.h					1.08		Anish Venkataraman	Updated the NFC_CONFIG_CODE_MSB &LSB adddress
						NFC.c					1.09							Updated NFC_Init() 1.02	that reads the NFC config	
08-28-2020		1.00.12 NFC.h					1.07		Anish Venkataraman	Added new structure block_STYP and thus the nfc_STYP structure.Added NFC_PageWrite,NFC_getBroadcastBlkAddress,NFC_getUpdateBlkAddress helper methods
						NFC.c					1.08							Removed NFC_SequentialWrite()Added NFC_PageWrite(),NFC_getBroadcastBlkAddress()	NFC_getUpdateBlkAddress()
						MinSlave.c				1.12							MinSlave_writeRegistersPRIV() 1.06
						ProductInfoNVM.h		1.02							Updated MAX_SIZE_PRODUCT_INFO to match with	TRC1000 processor padding for sizeof
						MinUart.h				1.03							Updated NUMBER_REQUEST_BYTES_FC70 enum
						Scheduler.c				1.04							Modified Scheduler_ManageMessage() to support block write. Added helper methods	Scheduler_writeProductInfo()Scheduler_writeInstant()Scheduler_writeBroadcast()						
08-17-2020		1.00.11 NFC.h					1.06		Anish Venkataraman	Updated address location for NFC's memory
						MinSlave.h				1.02							Added slave specific register and modified the max baudrate enum to appl1cation, and defaults updated
08-12-2020		1.00.10 NFC.c					1.07		Anish Venkataraman  Refactored code to use less memory and cleaned up code
						MyMain.c				1.03							Added check for permitting the NFC to save block info
						MinSlave.c				1.11							Refactored flow to save blcok to NFC. Added support for instant write only if addressed to NFC
07-07-2020		1.00.09 NFC.c					1.06		Anish Venkataraman	NFC_SequentilalRead() 1.01; 
						NFC.h					1.04							Removed NFC_WriteProduct_Info_FLG variable
						ProductInfoNVM.h		1.03							Added current and original product info	parameters for display
						MinSlave.c				1.10							MinSlave_storeProductInfoPRIV() 1.01;MinSlave_getProductInfoPRIV()1.00;MinSlave_manageMessages() 1.03
06-26-2020		1.00.08 NFC.c					1.05		Anish Venkataraman	NFC_SequentilalRead() 1.00; NFC_SequentilaWrite() 1.04; Removed NFC_read()
						NFC.h					1.03							Modified structure -refactored code
						IoTranslate.h			1.03							Modified macro for I2C delay and UART init
						MinSlave.c				1.09							MinSlave_writeRegisterPRIV() 1.04
06-16-2020		1.00.07	NFC.c					1.04		Anish Venkataraman	Added helper function for reducing code size; NFC_SequentilaWrite() 1.03; NFC_InstantaneousWrite() 1.01	; NFC_WriteDataPRIV() 1.00
06-15-2020		1.00.06	MinSlave.c				1.08		Anish Venkataraman	MinSlave_replyRegisterPRIV() 1.03;MinSlave_writeRegisterPRIV() 1.03
						BlockStrucures.h		1.03							Modified enums to support BLK11,BLK226
						Scheduler.c				1.03							Removed function Scheduler_checkNFCWrite and modified Scheduler_serviceNFCWrite()								
						NFC.c					1.03							Added support for writing BLK226; NFC_SequentilaWrite() 1.02
06-04-2020		1.00.05 MinSlave.c				1.07		Anish Venkataraman	MinSlave_writeOneRegisterPRIV() 1.01; MinSlave_writeRegistersPRIV() 1.02
						NFC.c					1.02							Re-factored code, removed NFC_Write function,created a new method for init and instantaneous write
04-15-2020		1.00.04 MinSlave.c				1.06		Anish Venkataraman	MinSlave_storeProductInfoPRIV() 1.0;MinSlave_replyRegistersPRIV() 1.02
						BlockStructures.h		1.02							Added auto test complete and Block 11 enum								
11-21-2019		1.00.03 MyMain.c				1.03		Anish Venkataraman  Changed scheduling from 1m to 10min and added a check for every completion of NFC write.
						Scheduler.c				1.02						    Added a function Scheduler_checkNFCWrite	and modified Scheduler_serviceNFCWrite().
						LedCtrl.c				1.02						    Modified parameter type to uint16_t of LedCtrl_HeartBeatSetInterval method
						NFC.c					1.01						    Changed the name methods, modified NFC_Sequential_Write function and created additional 2 methods NFC_Write and NFC_Read
						NFC.h					1.01						    Modified class structure and added macros. Added status and error enums
						Scheduler.h				1.02							Added a new function Scheduler_checkNFCWrite & added a macro SCHEDULER_NFC_WAIT_TIME
						MyMain.h				1.03							Changed scheduling from 1m to 10min and	updated class structure.
11-5-2019		1.00.02 MyMain.c				1.02		Anish Venkataraman  Changed scheduling from 5msec to 1min

11-4-2019		1.00.01 MyMain.c				1.01		Anish Venkataraman  Adapted from CPAM code and modified as per the NFC project 
						Scheduler.c				1.01							Adapted from CPAM and added Scheduler_serviceNFCWrite() function.
						LedCtrl.c				1.01							Modified as per project specification
						MinSlave.c				1.04							Ported from CPAM code and Added code to read from NFC and append it to the 
																				MIN_SLAVE_MODEL_CONFIGURATION_CODE. Added code to store Block2 parameters
						MinUart.c				1.00							Ported from CPAM code
						
9-10-2019		1.00.00 Version.h				1.00		Anish Venkataraman  Initial file
						NFC.c					1.00							Created a higher level module for lower level I2c driver.
						I2cDrive.c				1.00							Ported from AOS library
						main.c					1.00							IDE generated file
						IoTranslate.c			1.00                            Created a file to map hardware related modules.                                                                                                         
*/	

#endif /* VERSION_H_ */




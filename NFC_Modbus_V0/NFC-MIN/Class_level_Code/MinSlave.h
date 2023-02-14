/*
=======================================================================================
File name:    MinSlave.h
                    
Originator:   Tom Van Sistine

Description:

    Header file for MIN Slave handler class (MinSlave.c) which handles the high level
    processing of mIN messaging. It includes a low level MIN protocol component
    class MinUart.h

Use:
	This assumes a low level driver for the UART such as Processor Expert's AsynchroSerial
	component to buffer incoming characters and which supports a method to check if
	there are characters in the buffer to read as well as reading and sending
	characters.  The buffer should be of sufficient length so as handle all the incoming
	characters between periodic calls to MinSlave_Manage(). When MinSlave_Manage() is called
	it checks for characters in the buffer in a while loop and calls MinUart_ServiceRx()
	for each character until the buffer is empty.

	MinUart_ServiceRx() parses the message as characters are received and when a message is
	complete and passes the CRC, a process message flag is set that MinSlave_Manage()
	checks and processes the message and replies with a response. The message is sent to the
	Tx buffer before enabling the UART so the output buffer must be sized for the maximum
	reply length which is likely the slave register broadcast message.

	MinSlave_Manage() must be called by OnRxChar() interrupt to insure response to poll request
	meets the 30 msec specification.

	At 115,200 baud (max bus speed)	though that represents 113 characters. Therefore the buffer size must
	take the frequency of characters coming and the delay between calls to MinSlave_Manage().

	When the low level interrupt  see a completed transmission, it calls MinUart_ServiceTx().
	This is only a few lines of code to handle the UART and TxEna pin so is done in the interrupt
	itself.

	#define MIN_SLAVE_SPECIFIC_RNUMS SlaveSpecificRegisterNumberLish.h (#included here) with
	partial enum list register numbers for slave device specific registers.
	See: enum slaveRegisterNumbers.

Multi-Instance: No
 
Class Methods:
  	MinSlave_init();
	MinSlave_ManageMessage();
	MinGetData();
	void MinSlave_SetScratchPadData();


Resources:
	IoTranslate Requirements:

	HardwareUart_RxOff()            // Disables UART from receiving characters
	HardwareUart_SendChar()         // Sends one character out UART
	minTxEnable()                   // Sets TxEnable digital output
	minRxEnable()                   // Clears TxEnable digital output
	HardwareUart_GetCharsInRxBuf()  // Get number of characters in the receive buffer.
	WAIT_MSEC()						// For wait macro for 2 msec replay delay
=======================================================================================
 History:	
-*-----*------------*---------------------------------------------------*--------------
1.00	07-10-2019	New	file											Tom Van Sistine
1.01	08-02-2019  Moved communicationTimeoutCNTR to public and set    Tom Van Sistine
					default to ONE_SECOND_TIME.					
1.02	08-17-2020  Added slave specific register and modified the	 Anish Venkataraman
					max baudrate enum to appl1cation, and defaults updated
---------------------------------------------------------------------------------------
*/


#ifndef MINSLAVE_H_
#define MINSLAVE_H_

#include "MinUart.h" //component class of MIN.  Note MinUart.h #includes BlockStructures.h
#include "Version.h"
// List of slave parameters (needed before object structure defined for SLAVE_NUMBER_OF_PARAMETERS)
enum slaveRegisterNumbers {
  MIN_SLAVE_CURRENT_FW_VER_REV_RNUM = 0,		// First 15 Required of all MIN slave modules
  MIN_SLAVE_CURRENT_HW_VER_REV_RNUM,			// Firmware should determine this from hardware if possible.
  MIN_SLAVE_APPLICATION_RNUM,					// (0=unassigned, 1=Comm Gas Resideo)
  MIN_SLAVE_EEPROM_INIT_VALUE_RNUM,				// Copied from NVM byte 0 to indicate if initialized yet
  MIN_SLAVE_ORIGL_FW_VER_REV_RNUM,				// Written by CCB from original factory startup.
  MIN_SLAVE_ORIG_HW_VER_REV_RNUM,				// Written by CCB from original factory startup
  MIN_SLAVE_CURRENT_FW_BUILD_RNUM,
  MIN_SLAVE_MODEL_CONFIGURATION_CODE,
  MIN_SLAVE_RESERVED8_RNUM,
  MIN_SLAVE_RESERVED9_RNUM,
  MIN_SLAVE_RESERVED10_RNUM,
  MIN_SLAVE_RESERVED11_RNUM,
  MIN_SLAVE_RESERVED12_RNUM,
  MIN_SLAVE_RESERVED13_RNUM,
  MIN_SLAVE_RESERVED14_RNUM,
  MIN_SLAVE_RESERVED15_RNUM,
  //Slave specific registers
  MIN_SLAVE_PERMIT_TO_SAVE_DATA_RNUM,
  MIN_SLAVE_NUMBER_OF_REGISTERS
  
};


//CLASS OBJECT DEFINITION

typedef struct MinSlave_STYP{

// Public Variables
    uint16_t slaveRegisters[MIN_SLAVE_NUMBER_OF_REGISTERS];		// Slave registers for slave device See above.
    uint16_t communicationTimeoutCNTR;	// track AIN communications. Set Alarm after 30 seconds of no communications.

// Private Variables (Multiple-instance methods only)
    uint8_t blockToChange;  			// Set by setupSetpointChange()
    uint8_t regToChange;  				// Set by setupSetpointChange()
    uint16_t* pDataToChange;   			// Set by setupSetpointChange()
    uint8_t numRegsToChange;			// Set by setupSetpointChange()
    bool changeDataFlag;      			// Set by setupSetpointChange()
    bool validCommunicationReceivedFLG;	//signal communication is still active.
    bool delayDoneFLG;		  			// Set by interrupt.
    bool updateConfigurationsFLG;		// Set when new slave command to change a slave parameter received.
    uint16_t slavePollBroadcastInterval; // Number of calls between broadcasts.
// Component class
    struct MinUart_STYP uart;

}MinSlave_STYP;

//  PUBLIC CLASS METHOD PROTOTYPES DECLARATION
void MinSlave_init(struct MinSlave_STYP *minSlave);                                                              	// Initialize any variables
void MinSlave_manageMessages();                                                    						  	// MIN message processing
uint8_t MinSlave_setupSetpointChange(uint8_t block, uint8_t registerNum, uint16_t *pData, uint8_t numRegs); // Called to setup change in a setpoint in the master (controller).
uint8_t MinSlave_getMasterData(uint8_t block, uint8_t reg, uint16_t *returnDataW);                        	// Getter function for MIN Master registers
uint8_t MinSlave_getSlaveData(uint8_t reg, uint16_t *returnDataW);                                        	// Getter function for MIN slave registers
uint8_t MinSlave_putSlaveData(uint8_t index, uint16_t dataW);                                             	// Putter function for MIN slave registers
// DEFINITIONS
#define COMMUNICATION_TIMEOUT

// Fill in known basic information from version.h and build.h.
#define MIN_SLAVE_DEFAULTS				\
		{FIRMWARE_VERSION_REVISION,		\
		CURRENT_HW_VERSION_REVISION,	\
		0,								\
		0,								\
		FIRMWARE_VERSION_REVISION,		\
		CURRENT_HW_VERSION_REVISION,	\
		BUILDREVISION,					\
		0,								\
		0,0,0,0,0,0,0,0,0},				\
		ONE_SECOND_TIME,				\
		0,0,((void*)0),0,0,0,0,0,		\
		ONE_SECOND_TIME

#define MIN_DEFAULTS    {MIN_SLAVE_DEFAULTS,  \
		                 MIN_UART_DEFAULTS    \
						}


// DEFINE BLOCK NUMBERS.
#define NUMBER_OF_BLOCKS 254

// enum of blocks supported
enum {
	BLOCK0 = 0,
	BLOCK1 = 1,
	BLOCK2 = 2,
	BLOCK11 = 11,
	BLOCK226 = 226,
	BLOCK_NVM,
};

// MIN errors enum
enum {
	FAILED_TO_RECV_BLOCKS = 1,
	FAILED_TO_CHANGE_PARAMETER,
	MIN_NOT_INITIALIZED,

};

// MinSlave_getData() return messages
enum {
	MINSLAVE_GETDATA_SUCCESS = 0,
	MINSLAVE_GETDATA_FAIL_NULL_POINTER,
	MINSLAVE_GETDATA_FAIL_INVALID_BLOCK,
	MINSLAVE_GETDATA_FAIL_INVALID_REGISTER,
};

// MinSlave_getSlaveData() return messages
enum {
	MINSLAVE_GET_SLAVE_DATA_SUCCESS = 0,
	MINSLAVE_GET_SLAVE_DATA_FAIL_NULL_POINTER,
	MINSLAVE_GET_SLAVE_DATA_FAIL_INVALID_REGISTER,
};

// MinSlave_putSlaveData() return messages
enum {
	MINSLAVE_PUT_SLAVE_DATA_SUCCESS = 0,
	MINSLAVE_PUT_SLAVE_DATA_FAIL_INVALID_REGISTER,
};

// MIN Communications fault response, 0 - do nothing, 1 - clear fault,  2 - set fault
enum {
	NO_CHANGE = 0,
	CLEAR_FAULT,
	SET_FAULT
};


#endif /* MINSLAVE_H_ */




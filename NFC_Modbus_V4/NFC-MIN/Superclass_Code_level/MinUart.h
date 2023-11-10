/*
=======================================================================================
File name:    MinUart.h
                    
Originator:   Tom Van Sistine

Description:

    Header file for MIN UART driver.  This is intended as a component class to be part
    of the MinSlave class and implements the low level MIN protocol handling.
    See MinSlave.h for more implementation details.

Multi-Instance: No

Class Methods:
	MinUart_Init();					Called from MinSlave_init()
	MinUart_ServiceRx();			Called directly through MinSlave from OnRxChar()
								    in Events.c when a char is received.
	MinUart_ServiceTx(); 			Called from OnTxComplete() in Events.c whenever
	                                the transmission packet is finished being sent out.

Peripheral Resources:
	Assume as UART is available for 19,200 up to 115,200 baud

IoTranslate requirements:
	HardwareUart_RecvChar()         // Fetch 1 character from Uart buffer
	minRxEnable()                   // Clears TxEnable digital output
	minTxEnable()					// Sets TxEnable digital output
	HardwareUart_ClearBuffer()      // Reset UART buffer read and write indexes and clears char in Rx flag and full flag
	HardwareUart_SendChar()         // Sends one character out UART
	HardwareUart_GetCharsInRxBuf()  // Checks for characters in buffer.
	Enable_TXInterrupt()			//Enables Transmission interrupt
Other requirements:



=======================================================================================
 History:	
*-----*-------------*---------------------------------------------------*--------------
1.00	07-10-2019  Initial Write										Tom Van Sistine
1.01	08-01-2019  Add FC65 slave poll and FC66 baud rate enums.		Tom Van Sistine
1.02	08-12-2020  Updated productinfo bytes to receive length.   	 Anish Venkataraman
1.03	08-28-2020	Updated NUMBER_REQUEST_BYTES_FC70 enum			 Anish Venkataraman
---------------------------------------------------------------------------------------
*/

#ifndef MIN_UART_H_
#define MIN_UART_H_

#include "Build.h"



// rxState enum
enum {
	WAIT_FOR_RESYNC,
	PARSE_INCOMING,
};

// Use enum to define MIN constants
enum  {
	MIN_SLAVE_ADDRESS_INDEX = 0,
	MIN_FUNCTION_CODE_INDEX = 1,
	MIN_FC03_BYTES_REPLY_INDEX = 2,
	MIN_FC16_BLOCK_INDEX = 2,
	MIN_FC03_BLOCK_INDEX = 2,
	MIN_FC03_START_ADDR_INDEX = 3,
	MIN_FC16_REGISTER_INDEX = 3,
//	MIN_FC03_NUM_REG_INDEX = 5,
	MIN_FC03_NUM_REG_INDEX = 6,
	MIN_FC16_NUM_REG_INDEX = 5,
	MIN_FC66_BAUDRATE_INDEX = 5,
	MIN_FC06_REG_VALH_INDEX = 4,
	MIN_FC06_REG_VALl_INDEX = 5,
	MIN_FC_CRCL_INDEX = 6,
	MIN_FC_CRCH_INDEX = 7,
	MIN_MASTER_BROADCAST = 0,
	MIN_BYTES_PER_REG = 2,
	MIN_CRC_LENGTH = 2,
	MIN_FC03_DATA_START = 3,
	MIN_FC_WRITE_REPLY_LENGTH = 6,
	MIN_FC16_BYTES_TO_RX_INDEX = 6,
	MIN_FC16_DATA_START_INDEX = 7,
	MIN_FC65_TYPE_CODE_INDEX = 2,
	MIN_FC65_NO_RESPONSE_CODE = 0,
	MIN_FC65_SETPOINT_RESPONSE_CODE = 1,
	MIN_FC65_SLAVE_BROADCAST_RESPONSE_CODE = 2,
	MIN_FC65_SETPOINT_RESPONSE_PREFIX_LENGTH = 6,
	MIN_FC65_BROADCAST_DATA_START_INDEX=6,
	MIN_FC65_NO_RESPONSE_LENGTH = 5,
	MIN_FC69_BYTES_TO_RX_INDEX = 2,
	MIN_FC69_DATA_START_INDEX = 3,
	MIN_FC69_DATA_LENGTH_INDEX = 5,
	MIN_FC70_BYTES_TO_RX_INDEX = 2,
	MIN_FC70_DATA_START_INDEX = 3,
	MIN_FC70_PAYLOAD = 5,
};

// Use enum to define MIN constants
enum  {
	MODBUS_SLAVE_ADDRESS_INDEX = 0,
	MODBUS_FUNCTION_CODE_INDEX = 1,
	MODBUS_FC03_BYTES_REPLY_INDEX = 2,
	MODBUS_FC16_BLOCK_INDEX = 2,
	MODBUS_FC03_BLOCK_INDEX = 2,
	MODBUS_FC03_MSB_ADDR_INDEX = 2,
	MODBUS_FC03_START_ADDR_INDEX = 3,
	MODBUS_FC16_REGISTER_INDEX = 3,
	//	MIN_FC03_NUM_REG_INDEX = 5,
	MODBUS_FC03_NUM_REG_INDEX = 5,
	MODBUS_FC16_NUM_REG_INDEX = 5,
	MODBUS_FC66_BAUDRATE_INDEX = 5,
	MODBUS_FC06_REG_VALH_INDEX = 4,
	MODBUS_FC06_REG_VALl_INDEX = 5,
	MODBUS_FC_CRCL_INDEX = 6,
	MODBUS_FC_CRCH_INDEX = 7,
	MODBUS_MASTER_BROADCAST = 0,
	MODBUS_BYTES_PER_REG = 2,
	MODBUS_CRC_LENGTH = 2,
	MODBUS_FC03_DATA_START = 3,
	MODBUS_FC_WRITE_REPLY_LENGTH = 6,
	MODBUS_FC16_BYTES_TO_RX_INDEX = 6,
	MODBUS_FC16_DATA_START_INDEX = 7,
	MODBUS_FC65_TYPE_CODE_INDEX = 2,
	MODBUS_FC65_NO_RESPONSE_CODE = 0,
	MODBUS_FC65_SETPOINT_RESPONSE_CODE = 1,
	MODBUS_FC65_SLAVE_BROADCAST_RESPONSE_CODE = 2,
	MODBUS_FC65_SETPOINT_RESPONSE_PREFIX_LENGTH = 6,
	MODBUS_FC65_BROADCAST_DATA_START_INDEX=6,
	MODBUS_FC65_NO_RESPONSE_LENGTH = 5,
	MODBUS_FC69_BYTES_TO_RX_INDEX = 2,
	MODBUS_FC69_DATA_START_INDEX = 3,
	MODBUS_FC69_DATA_LENGTH_INDEX = 5,
	MODBSU_FC70_BYTES_TO_RX_INDEX = 2,
	MODBUS_FC70_DATA_START_INDEX = 3,
	MODBUS_FC70_PAYLOAD = 5,
};

// In Build.h #define MIN_SLAVE_ADDRESS for slave module.
#define MIN_RECEIVE_BLOCK_SIZE (uint16_t) 128U // 128 registers is maximum block size
#define RX_BLOCK_BUFFER_SIZE  ((MIN_RECEIVE_BLOCK_SIZE * MIN_BYTES_PER_REG) + 8U) // 8 = SlaveAddress, AddressH, AddressL, NumberRegH, NumberRegL, Byte count, CRCL, CRCH
#define MODBUS_RESYNC_LOAD 3	// Msec interrupt counter.  Reset to this when byte received from MIN master.  Not sychronized with 1msec timer
								// so set to 3 to insure minimum of 2 msec time.

enum {
	MIN_FC03 = 3,
	MIN_FC04 = 4,
	MIN_FC06 = 6,
	MIN_FC16 = 16,
	MIN_FC65_SLAVE_POLL = 65,
	MIN_FC66_BAUDRATE,
	MIN_FC67_DISCOVER,
	MIN_FC69_GET_PRODUCT_INFO = 69,
	MIN_FC70_STORE_PRODUCT_INFO,
};


enum {
	MODBUS_FC03 = 3,
	MODBUS_FC04 = 4,
	MODBUS_FC06 = 6,
	MODBUS_FC16 = 16,
	MODBUS_FC65_SLAVE_POLL = 65,
	MODBUS_FC66_BAUDRATE,
	MODBUS_FC67_DISCOVER,
	MODBUS_FC69_GET_PRODUCT_INFO = 69,
	MODBUS_FC70_STORE_PRODUCT_INFO,
};

//#define NUMBER_REQUEST_BYTES_FC03 8 // read registers
#define NUMBER_REQUEST_BYTES_FC03 8 // read registers
#define NUMBER_REQUEST_BYTES_FC06 8 // write setpoint register
//#define NUMBER_REQUEST_BYTES_FC16 9 // 2 registers but need add number as message comes in. (broadcast block)
#define NUMBER_REQUEST_BYTES_FC16 11 // 2 registers but need add number as message comes in. (broadcast block)
#define NUMBER_REQUEST_BYTES_FC65 4 // Slave poll
#define NUMBER_REQUEST_BYTES_FC66 8 // baud rate
#define NUMBER_REQUEST_BYTES_FC67 8 // Discover
#define NUMBER_REQUEST_BYTES_FC69 8
#define NUMBER_REQUEST_BYTES_FC70 105	//Product Info
#define INITIALIZED				  0x5A5A //Min permission to save

typedef struct MinUart_STYP{

	// Public Variables
	uint8_t modbusReSync;		// 1msec count down timer for resyncing communications.
	uint8_t rxBuffer[RX_BLOCK_BUFFER_SIZE];
	uint8_t moduleAddress;		// Stores the slave address
	uint8_t processPacketFlag;	// Set when at end of message and checksum is OK
	uint8_t baudSelect;			// See Build.h for selection enum

	// Private Variables (Multi-instance methods only)
	uint8_t  badCrcFlag;
	uint16_t dataLength; 		// Expected incoming data length
	uint16_t rxBufferIndex;
	uint8_t functionCode;
	uint8_t  currentState;
} MinUart_STYP;
#define MIN_UART_DEFAULTS  {0,					\
							{0},				\
							NFC_BASE_ADDRESS,	\
							0, 					\
							BR_SELECT_115200,	\
							0,0,0,0,0,			\
						   }

void MinUart_init(MinUart_STYP *);
void MinUart_serviceRx(MinUart_STYP *);
void MinUart_serviceTx(MinUart_STYP *);
uint16_t get_crc_16 (uint16_t start, uint8_t *p, uint16_t n);

#endif /* MIN_UART_H_ */

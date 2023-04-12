/*
---------------------------------------------------------------------------------------
Filename:	MinSlave.c
Originator:	Tom Van Sistine
Description:
			Modbus Internal Network slave library class.

			Handles upper level MIN messages. This class is a hardware independent
			generic MIN slave communication module. CurrentMessageState needs to be
			received and CRC integrity check done by the MinUart component class of
			which MinSlave class is partly composed of.

			MinSlave is also responsible for encapsulating the master broadcasted
			registers it might need. Note: A UIM would save most if not all registers
			but other slaves might save a tiny fraction of them.  This is defined in
			BlockStructures.h.

			Self referencing pointer is used except for MinSlave_init() where the
			its object pointer is passed.

Resources:


=======================================================================================
 History:	
 *-------*-----------*---------------------------------------------------*--------------
1.00	07-10-2019  New file											Tom Van Sistine
1.01	08-02-2019	MinSlave_init() Ver 1.01							Tom Van Sistine
					MinSlave_manageMessages() Ver 1.01
1.02	08-07-2019	MinSlave_manageMessages() Ver 1.02					Tom Van Sistine
1.03	08-12-2019	MinSlave_spBroadcastPRIV() Ver 1.01					Tom Van Sistine
1.04	11-04-2019	Added code to read from NFC and append it to the  Anish Venkataraman
					MIN_SLAVE_MODEL_CONFIGURATION_CODE. Added code to
					store Block2 parameters
1.05	11-05-2019	Removed global variables NFC_WRITE_FLAG			 Anish Venkataraman
1.06	04-15-2020	MinSlave_storeProductInfoPRIV() 1.0				 Anish Venkataraman
					MinSlave_replyRegistersPRIV() 1.02
1.07	06-04-2020	MinSlave_writeOneRegisterPRIV() 1.01			 Anish Venkataraman
					MinSlave_writeRegistersPRIV() 1.02
1.08	06-15-2020	MinSlave_replyRegisterPRIV() 1.03				 Anish Venkataraman
					MinSlave_writeRegistersPRIV() 1.03
1.09	06-26-2020	MinSlave_writeRegisterPRIV() 1.04				 Anish Venkataraman
1.10	07-07-2020	MinSlave_storeProductInfoPRIV() 1.01			 Anish Venkataraman
					MinSlave_getProductInfoPRIV()	1.00
					MinSlave_manageMessages() 1.03	
1.11	08-12-2020	MinSlave_writeRegistersPRIV() 1.05				 Anish Venkataraman
					MinSlave_writeOneRegisterPRIV() 1.02							
1.12	08-28-2020	MinSlave_writeRegistersPRIV() 1.06				 Anish Venkataraman
1.13	02-19-2021	MinSlave_writeRegistersPRIV() 1.07				 Anish Venkataraman
1.14	05-10-2021	_getProductInfoPRIV() 1.01						 Anish Venkataraman
					_storeProductInfoPRIV() 1.03
					deleted productInfoNVM.h reference and
---------------------------------------------------------------------------------------
 */
#include "MinSlave.h"
#include "IoTranslate.h"
#include "assert.h"
#include "NFC.h"

#define EEPBusy		1
#define EEPFree		0

extern MinSlave_STYP oMinSlave;
//Global variables
extern NFC_STYP oNFC;

// Private Method Prototypes
static void MinSlave_slavePollPRIV(void);
static void MinSlave_discoverPRIV(void);
static void MinSlave_writeOneRegisterPRIV(void);
static void MinSlave_writeRegistersPRIV(void);
static uint8_t MinSlave_spSendRegistersPRIV(void);
static uint8_t MinSlave_spNothingToRespondPRIV(void);
static void MinSlave_sendReplyPRIV(uint8_t * txBuf, uint8_t txLength);
static void MinSlave_replyRegisterRequestPRIV(void);
static void MinSlave_storeProductInfoPRIV(void);
static void MinSlave_getProductInfoPRIV(void);

static void ModbusSlave_replyRegisterRequestPRIV(void);
static void ModbusSlave_writeOneRegisterPRIV(void);
static void ModbusSlave_writeRegistersPRIV(void);



static uint8_t txBuf[NFC_MAX_MEM];
//lint -e9029	suppress "Mismatched essential type" PC-Lint 9.00k  Bug in PC-Lint does not like subscripts!

// Create data storage slave block data
static MinSlave_STYP *pMinSlaveSelf = 0;	// On MinSlave_Init() from MyMain.c a pointer to the instantiated object is sent.
// This will be saved here for the MinSlave_GetSlaveData() getter
// method screens use to request slave parameters.

uint8_t FlagEEPBusy = 0;

// Represents longest reply string.

/*
=======================================================================================
Method name:    MinSlave_init()

Originator:		Tom Van Sistine

Description:	Initialize ainSlave object


=======================================================================================
 History: (Identify changes in this method)	
 *-------*-----------*---------------------------------------------------*--------------
1.00	07-10-2019	Initial Write										Tom Van Sistine
1.01	08-02-2019	Add setting baud rate								Tom Van Sistine
---------------------------------------------------------------------------------------
 */

void MinSlave_init(MinSlave_STYP *minSlave) {

	// Initialize all member values

	minSlave->changeDataFlag = FALSE;
	minSlave->communicationTimeoutCNTR = ONE_SECOND_TIME;
	minSlave->delayDoneFLG = FALSE;

	// Save reference to itself so it can support Slave parameter get requests.
	pMinSlaveSelf = minSlave;

	// Initialize the class object
	MinUart_init(&minSlave->uart);

	// Initialize back to default if something went wrong
	minRxEnable();
}

/*
=======================================================================================
Method name:    MinSlave_manageMessages()

Originator:   	Tom Van Sistine

Description:  	Called by Events.c's OnRxChar() when a byte comes in from MIN Master.
				Calls MinUart_ServiceRx() to handle incoming byte.
				If an incoming message is ready, then process it. This may include
				sending a reply or just taking an action based on sent data or command.

				Messages specify function codes (FCxx) that include:


				FC03: Read specified registers at offset for length.

				FC06: Write single register (i.e. like a setpoint)

				FC16: Write Multiple registers i.e. used for Broadcast: Blocks from
				Master that some or all data needs to be saved, or could be used to
				configure a slave device like CPAM-MIN

				FC 65 Slave Poll Request: The Master permits the slave to initiate a
				message.
				These can include broadcasting its block of registers, sending a setpoint
				to the master or another slave device, or sending an OTA packet (UIM
				received from WiFi module)

				FC66 Baud rate Change: Variable baud rates, 19.2K for Discover or higher
				for other message types as commanded.

 				FC67 Discover: Command to slave to send back 16 key registers

 				FC68 Over The Air (OTA) update packet response (Master response to slave
 				sent OTA packet)

 				FC69 Get productInfoNVM (UIM and NFC key send system critical information)

 				FC70 Send productInfoNVM (Master sends system critical information to
 				store)

 				FC71 Send OTA packet to slave device.

=======================================================================================
 History: (Identify changes in this method)	
 *-------*-----------*---------------------------------------------------*--------------
1.00	07-10-2019	Initial Write										Tom Van Sistine
1.01	08-02-2019	Added MIN_FC66_BAUDRATE case.						Tom Van Sistine
1.02	08-07-2019	Refactored to break out command replies				Tom Van Sistine
1.03	07-07-2020	Added get and store method for product info		 Anish Venkataraman
					FC69 and FC70	
---------------------------------------------------------------------------------------
 */



void MinSlave_manageMessages(void) {

	// If pointer to itself has not been initialized, return.
	if (pMinSlaveSelf == 0) {
		return;
	}
	// Poll UART Rx buffer for characters and call MinUart_ServiceRx() for each character
	// received until buffer is empty.
	while (HardwareUart_GetCharsInRxBuf() > (uint16_t) 0) {
		MinUart_serviceRx(&pMinSlaveSelf->uart);
	}
	// Check if a packet of data from Master device (found in AinUart_ServiceRx()) is ready to process.
	if (pMinSlaveSelf->uart.processPacketFlag == FALSE) {
		return;
	}
	pMinSlaveSelf->validCommunicationReceivedFLG = TRUE; //Signal communications timer to reset.

	// Packet is finished and checksum is OK
	pMinSlaveSelf->uart.processPacketFlag = FALSE;

	// Reset no communications timer (if somehow no communications for a while Scheduler will reset for discover baud)
	pMinSlaveSelf->communicationTimeoutCNTR = ONE_SECOND_TIME;

	MinTurnAroundDelayMAC();  // 2 msec delay before reply to give chance for master to turn off it TxEnable.

	switch (pMinSlaveSelf->uart.functionCode) {

		//case MIN_FC03:     				// Requesting one or more Holding Registers. Note: FC03 are always slave specific as it requires reply.
		//MinSlave_replyRegisterRequestPRIV();
		
		case MODBUS_FC03:     				// Requesting one or more Holding Registers. Note: FC03 are always slave specific as it requires reply.
		ModbusSlave_replyRegisterRequestPRIV();
		
		break;
		
		case MIN_FC67_DISCOVER:			// Discover is basically the same as FC03 (subset) so share common code
		MinSlave_discoverPRIV();
		break;

		//case MIN_FC06: // Write 1 Holding Slave Register
		//MinSlave_writeOneRegisterPRIV();
		//break;

		case MIN_FC06: // Write 1 Holding Slave Register
		ModbusSlave_writeOneRegisterPRIV();
		break;

		//case MIN_FC16:	// Write Master broadcast blocks or multiple slave registers
		//MinSlave_writeRegistersPRIV();
		//break;
		case MIN_FC16:	// Write Master broadcast blocks or multiple slave registers
		ModbusSlave_writeRegistersPRIV();
		break;


		case MIN_FC65_SLAVE_POLL:
		MinSlave_slavePollPRIV(); // Reply with one of several possible slave poll responses.
		break;

		case MIN_FC69_GET_PRODUCT_INFO:
		//send contents from NFC requested by Master
		MinSlave_getProductInfoPRIV();
		break;

		case MIN_FC70_STORE_PRODUCT_INFO:
		//store contents to NFC sent by Master
		MinSlave_storeProductInfoPRIV();
		break;

		default:
		break;
	}
}

/*
 ========================================================================================
 Method name:  MinSlave_discover()

 Originator:   Tom Van Sistine

 Description:

 	 	 Handles Discovery command reply
 Resources:

 ========================================================================================
 History:
 *-------*-----------*---------------------------------------------------*---------------
 1.00    08-07-2019  Original code broke out MinSlave_manageMessages()   Tom Van Sistine
 1.01	 11-04-2019  Added code to read from NFC and append it to the  Anish Venkataraman
					 MIN_SLAVE_MODEL_CONFIGURATION_CODE.
 ----------------------------------------------------------------------------------------
 */
static void MinSlave_discoverPRIV(void) {

	uint8_t i;							// Generic index variable used where needed
	uint16_t * registerDataPointer;		// Generic point to either master or slave registers depending on which command processed.
	uint8_t * dataPtr;
	uint16_t crc;
	uint8_t txLength;
	//uint16_t config = 0;
	// Setup reply
	txBuf[MIN_SLAVE_ADDRESS_INDEX] = pMinSlaveSelf->uart.moduleAddress;  	// Slave address
	txBuf[MIN_FUNCTION_CODE_INDEX] = MIN_FC67_DISCOVER;                    			// Function code
	txBuf[MIN_FC03_BYTES_REPLY_INDEX] = pMinSlaveSelf->uart.rxBuffer[MIN_FC03_NUM_REG_INDEX] * MIN_BYTES_PER_REG;
	// Get requested registers

	// Set source pointer to slave data
	registerDataPointer = &pMinSlaveSelf->slaveRegisters[pMinSlaveSelf->uart.rxBuffer[MIN_FC03_START_ADDR_INDEX]];

	// Set destination pointer to txBuf
	dataPtr = &txBuf[MIN_FC03_DATA_START];
	for (i = 0; i < pMinSlaveSelf->uart.rxBuffer[MIN_FC03_NUM_REG_INDEX]; i++) {
		*dataPtr++ = (uint8_t)(*registerDataPointer >> 8);		// Data High
		*dataPtr++ = (uint8_t) (*registerDataPointer++ & 0xFF);	// Data Low
	}

	crc = get_crc_16 (0xFFFF, txBuf, MIN_FC03_START_ADDR_INDEX + (i * MIN_BYTES_PER_REG));         // Calculate the CRC to send with reply
	*dataPtr++ = (uint8_t) (crc & 0xff);   // CRCL
	*dataPtr = (uint8_t) (crc >> 8); // CRCH
	txLength = (MIN_FC03_DATA_START + (pMinSlaveSelf->uart.rxBuffer[MIN_FC03_NUM_REG_INDEX] * MIN_BYTES_PER_REG) + MIN_CRC_LENGTH);

	// Enable the Tx line
	minTxEnable();
	// Send reply
	MinSlave_sendReplyPRIV(txBuf, txLength);

}

/*
 ========================================================================================
 Method name:  MinSlave_writeOneRegisterPRIV()

 Originator:   Tom Van Sistine

 Description:
		Handles FC06 command to write one register

 Resources:

 ========================================================================================
 History:
 *-------*-----------*---------------------------------------------------*---------------
 1.00    08-07-2019  Original code broke out MinSlave_manageMessages()   Tom Van Sistine
 1.01    06-04-2020  Modified the code to write the respective block   Anish Venkataraman
					 register to NFC key
 1.02    08-12-2020  Refactored the code to save permission to		   Anish Venkataraman
					 save register data to NFC key
 ----------------------------------------------------------------------------------------
 */
static void MinSlave_writeOneRegisterPRIV(void)  {
	uint8_t i;							// Generic index variable used where needed
	uint16_t data;
	uint8_t * dataPtr;
	uint16_t crc;
	uint8_t txLength;
	uint16_t address;
	uint8_t registerNumber;
	uint8_t block;
	block = pMinSlaveSelf->uart.rxBuffer[MIN_FC03_BLOCK_INDEX]; //get block info
	data = (((uint16_t) pMinSlaveSelf->uart.rxBuffer[MIN_FC06_REG_VALH_INDEX]) << 8) + (uint16_t) pMinSlaveSelf->uart.rxBuffer[MIN_FC06_REG_VALl_INDEX]; //get data
	registerNumber =  pMinSlaveSelf->uart.rxBuffer[MIN_FC03_START_ADDR_INDEX]; //store register number
	if(block == BLOCK2 || block == BLOCK11 ||block == BLOCK226 ){
		if(block == BLOCK2){
			address = BLOCK2_OFFSET + (registerNumber * 2); //address update
		}
		else if(block == BLOCK11){
			address = BLOCK11_OFFSET + (registerNumber * 2);
		}
		else{
			address = BLOCK226_OFFSET + (registerNumber * 2);
		}
		NFC_InstantaneousWrite(&oNFC,address,data);//write the data
		
	}
	else if (block == MIN_SLAVE_PERMIT_TO_SAVE_DATA_RNUM) { //save permision to min slave specific register
		pMinSlaveSelf->slaveRegisters[MIN_SLAVE_PERMIT_TO_SAVE_DATA_RNUM] = data;
	}	
	else {
		//do nothing
		
	}
	// Reply is required for specific slave writing of a register. Reply is same as request for first 6 bytes received
	dataPtr = &pMinSlaveSelf->uart.rxBuffer[MIN_SLAVE_ADDRESS_INDEX];

	for (i = 0; i < MIN_FC_WRITE_REPLY_LENGTH; i++) {
		txBuf[i] = *dataPtr++;
	}

	crc = get_crc_16 (0xFFFF, txBuf, MIN_FC_WRITE_REPLY_LENGTH);         // Calculate the CRC to send with reply slaveAddr, FC, addrH, addrL, dataH, datal
	txBuf[MIN_FC_CRCL_INDEX] = (uint8_t) (crc & 0xff);             // CRCL
	txBuf[MIN_FC_CRCH_INDEX] = (uint8_t) (crc >> 8);               // CRCH
	txLength = MIN_FC_WRITE_REPLY_LENGTH + (uint8_t) MIN_CRC_LENGTH;

	// Enable the Tx line
	minTxEnable();

	// Send reply
	MinSlave_sendReplyPRIV(txBuf, txLength);

}

/*
 ========================================================================================
 Method name:  ModbusSlave_writeOneRegisterPRIV()

 Originator:   Tom Van Sistine

 Description:
		Handles FC06 command to write one register

 Resources:

 ========================================================================================
 History:
 *-------*-----------*---------------------------------------------------*---------------
 1.00    08-07-2019  Original code broke out MinSlave_manageMessages()   Tom Van Sistine
 1.01    06-04-2020  Modified the code to write the respective block   Anish Venkataraman
					 register to NFC key
 1.02    08-12-2020  Refactored the code to save permission to		   Anish Venkataraman
					 save register data to NFC key
 1.03    10-11-2022  Standard Modbus implemented instead of MIN		   Onkar Raut
					 					 
 ----------------------------------------------------------------------------------------
 */
static void ModbusSlave_writeOneRegisterPRIV(void)  {
	uint8_t i;							// Generic index variable used where needed
	uint16_t data;
	uint8_t * dataPtr;
	uint16_t crc;
	uint8_t txLength;
	uint16_t address;
	uint16_t registerNumber;
	uint8_t block;
	
	FlagEEPBusy = EEPBusy;
	
	block = pMinSlaveSelf->uart.rxBuffer[MODBUS_FC03_BLOCK_INDEX]; //get block info
	data = (((uint16_t) pMinSlaveSelf->uart.rxBuffer[MODBUS_FC06_REG_VALH_INDEX]) << 8) + (uint16_t) pMinSlaveSelf->uart.rxBuffer[MODBUS_FC06_REG_VALl_INDEX]; //get data
	registerNumber =  pMinSlaveSelf->uart.rxBuffer[MODBUS_FC03_START_ADDR_INDEX]; //store register number
	
//	registerNumber =  registerNumber - 0x40;

	registerNumber = (pMinSlaveSelf->uart.rxBuffer[MODBUS_FC03_MSB_ADDR_INDEX]);
	registerNumber = registerNumber << 8;
	registerNumber = registerNumber | (pMinSlaveSelf->uart.rxBuffer[MODBUS_FC03_START_ADDR_INDEX]);
	registerNumber = ((uint16_t)registerNumber - 0x9C40);
	
	address = (uint16_t)registerNumber * 2;

	//if(block == BLOCK2 || block == BLOCK11 ||block == BLOCK226 ){
		//if(block == BLOCK2){
			//address = BLOCK2_OFFSET + (registerNumber * 2); //address update
		//}
		//else if(block == BLOCK11){
			//address = BLOCK11_OFFSET + (registerNumber * 2);
		//}
		//else{
			//address = BLOCK226_OFFSET + (registerNumber * 2);
		//}
		//NFC_InstantaneousWrite(&oNFC,address,data);//write the data
		//
	//}
	//else if (block == MIN_SLAVE_PERMIT_TO_SAVE_DATA_RNUM) { //save permision to min slave specific register
		//pMinSlaveSelf->slaveRegisters[MIN_SLAVE_PERMIT_TO_SAVE_DATA_RNUM] = data;
	//}	
	//else {
		////do nothing
		//
	//}
	
//	address = (0x20 + registerNumber);
		
//	address = (uint16_t)(HOLDING_REG_OFFSET + (registerNumber * 2));

//	address = (uint16_t)(HOLDING_REG_OFFSET + (registerNumber ));

	NFC_InstantaneousWrite(&oNFC,address,data);//write the data
	// Reply is required for specific slave writing of a register. Reply is same as request for first 6 bytes received
	dataPtr = &pMinSlaveSelf->uart.rxBuffer[MODBUS_SLAVE_ADDRESS_INDEX];

	for (i = 0; i < MODBUS_FC_WRITE_REPLY_LENGTH; i++) {
		txBuf[i] = *dataPtr++;
	}

	crc = get_crc_16 (0xFFFF, txBuf, MODBUS_FC_WRITE_REPLY_LENGTH);         // Calculate the CRC to send with reply slaveAddr, FC, addrH, addrL, dataH, datal
	txBuf[MODBUS_FC_CRCL_INDEX] = (uint8_t) (crc & 0xff);             // CRCL
	txBuf[MODBUS_FC_CRCH_INDEX] = (uint8_t) (crc >> 8);               // CRCH
	txLength = MODBUS_FC_WRITE_REPLY_LENGTH + (uint8_t) MODBUS_CRC_LENGTH;

	// Enable the Tx line
	minTxEnable();

	// Send reply
	MinSlave_sendReplyPRIV(txBuf, txLength);
	
	FlagEEPBusy = EEPFree;

}

/*
 ========================================================================================
 Method name:  ModbusSlave_writeRegistersPRIV()

 Originator:   Tom Van Sistine

 Description:
 	 	 Handles write multiple registers (block writes) which can be Master broadcast
 	 	 blocks (no reply) or multiple slave registers (with reply).

 Resources:

 ========================================================================================
 History:
 *-------*-----------*---------------------------------------------------*---------------
 1.00    08-07-2019  Original code broke out MinSlave_manageMessages()   Tom Van Sistine
 1.01	 11-04-2019  Added code to store data in an array and once all Anish Venkataraman
					 data is received set the flag to TRUE.	
 1.02    06-04-2020  Modified the code to store the respective block   Anish Venkataraman
					 register to NFC object					 
 1.03    06-15-2020  Added instantaneous write for max of 8 bytes	   Anish Venkataraman
					 Added code to support BLK 226 	
 1.04    06-26-2020  Refactored code								   Anish Venkataraman
 1.05    08-12-2020  Modified code to write instantaneous only when	   Anish Venkataraman					 
					 addressed to NFC and refactored it.
 1.06    08-28-2020  Added reply when addressed to NFC				   Anish Venkataraman
 1.07    02-19-2021  Added check for permission to save data		   Anish Venkataraman
 1.08	 08-11-2022	 Actual Modbus implemented in Firmware			   Onkar Raut
 ----------------------------------------------------------------------------------------
 */

static void ModbusSlave_writeRegistersPRIV(void) {
	uint8_t i;							// Generic index variable used where needed
	uint8_t txLength;
	uint16_t crc;
	uint8_t * dataPtr;
	uint8_t blockNumber;
	uint16_t registerNumber;
	uint8_t dataLength;
	
	
	FlagEEPBusy = EEPBusy;
	
	
	////blockNumber = pMinSlaveSelf->uart.rxBuffer[MIN_FC16_BLOCK_INDEX]; //get block number
//	registerNumber = pMinSlaveSelf->uart.rxBuffer[MODBUS_FC16_REGISTER_INDEX]; //get the register number
	
//	registerNumber = registerNumber - 0x40;
	
	registerNumber = (uint16_t)(pMinSlaveSelf->uart.rxBuffer[MODBUS_FC03_MSB_ADDR_INDEX]);
	registerNumber = (uint16_t)registerNumber << 8;
	registerNumber = (uint16_t)registerNumber | (pMinSlaveSelf->uart.rxBuffer[MODBUS_FC03_START_ADDR_INDEX]);
	registerNumber = ((uint16_t)registerNumber - 0x9C40);
		
//	registerNumber = (uint16_t)registerNumber * 2;

	
	dataLength = pMinSlaveSelf->uart.rxBuffer[MODBUS_FC16_BYTES_TO_RX_INDEX]; //length of the data to be written
	// Initialize source data pointer to start of register data in rxBuffer
	dataPtr = &pMinSlaveSelf->uart.rxBuffer[MODBUS_FC16_DATA_START_INDEX];	// Set pointer to first incoming register value high byte.
	
	// Check if MASTER is broadcasting a block of its registers (slave address is 0 for master broadcast)
		//if (pMinSlaveSelf->uart.rxBuffer[MODBUS_SLAVE_ADDRESS_INDEX] == MODBUS_MASTER_BROADCAST && pMinSlaveSelf->slaveRegisters[MIN_SLAVE_PERMIT_TO_SAVE_DATA_RNUM] == INITIALIZED) {
		if (pMinSlaveSelf->uart.rxBuffer[MODBUS_SLAVE_ADDRESS_INDEX] == MODBUS_MASTER_BROADCAST)
		 {
			oNFC.broadcast.length = dataLength;
		
		//if(blockNumber == BLOCK2 && oNFC.block2WriteFLG == TRUE){ //store block 2
			//oNFC.broadcast.blockNumber = blockNumber;//store block number
			//oNFC.broadcast.registerNumber = registerNumber;//store register to be written
			//for (i = 0; i < (uint8_t) (dataLength); i++) {
				//oNFC.broadcast.nfcBuffer[i] = *dataPtr++;//store data
			//}
			//oNFC.storeBroadcastFLG = TRUE;
			//
		//}
		//else if(blockNumber == BLOCK11 && oNFC.block11WriteFLG == TRUE){//store block 11
			//oNFC.broadcast.blockNumber = blockNumber;
			//oNFC.broadcast.registerNumber = registerNumber;
			//for (i = 0; i < (uint8_t) (dataLength); i++) {
				//oNFC.broadcast.nfcBuffer[i] = *dataPtr++;
			//}
			//oNFC.storeBroadcastFLG = TRUE;
		//}
		//else if(blockNumber == BLOCK226 && oNFC.block226WriteFLG == TRUE){//store block 226
		//oNFC.broadcast.blockNumber = blockNumber;
		
		oNFC.broadcast.registerNumber = (registerNumber * 2);
		
		//oNFC.broadcast.registerNumber = (registerNumber);
		
		for (i = 0; i < (uint8_t) (dataLength); i++)   {
			oNFC.broadcast.nfcBuffer[i] = *dataPtr++;
			}
			oNFC.storeBroadcastFLG = TRUE;
		//}
		//else{
			////do nothing
		//}		
		
		// Note: there is no reply for master broadcast of a its block.
	}
	//write data to NFC memory instantly
	//else if (pMinSlaveSelf->uart.rxBuffer[MODBUS_SLAVE_ADDRESS_INDEX] == NFC_SLAVE_ADDRESS && pMinSlaveSelf->slaveRegisters[MIN_SLAVE_PERMIT_TO_SAVE_DATA_RNUM] == INITIALIZED) {
	else if (pMinSlaveSelf->uart.rxBuffer[MODBUS_SLAVE_ADDRESS_INDEX] == NFC_SLAVE_ADDRESS)
	 {		
		oNFC.update.length = dataLength;
		//if(blockNumber == BLOCK2 || blockNumber == BLOCK11 || blockNumber == BLOCK226)
		//{//save Block instantly
			//oNFC.update.blockNumber = blockNumber;
			oNFC.update.registerNumber = registerNumber;
			for (i = 0; i < (uint8_t) (dataLength); i++) {
				oNFC.update.nfcBuffer[i] = *dataPtr++;
			}
			oNFC.instantWriteFLG = TRUE;
			// Reply is required for specific slave writing of registers. Reply is same as request for first 6 bytes received
			dataPtr = &pMinSlaveSelf->uart.rxBuffer[MODBUS_SLAVE_ADDRESS_INDEX];

			for (i = 0; i < MODBUS_FC_WRITE_REPLY_LENGTH; i++) {
				txBuf[i] = *dataPtr++;
			}
			crc = get_crc_16 (0xFFFF, txBuf, MODBUS_FC_WRITE_REPLY_LENGTH);         // Calculate the CRC to send with reply slaveAddr, FC, addrH, addrL, dataH, datal
			txBuf[MIN_FC_CRCL_INDEX] = (uint8_t) (crc & 0xff);             // CRCL
			txBuf[MIN_FC_CRCH_INDEX] = (uint8_t) (crc >> 8);               // CRCH
			txLength = MIN_FC_WRITE_REPLY_LENGTH + MIN_CRC_LENGTH;
			// Enable the Tx line
			minTxEnable();

		// Send reply
		MinSlave_sendReplyPRIV(txBuf, txLength);
		}
		
	//}

	FlagEEPBusy = EEPFree;
}

/*
 ========================================================================================
 Method name:  MinSlave_slavePollPRIV()

 Originator:   Tom Van Sistine

 Description:
 	 	 Respond to slave poll command with one of several responses based on priority
 	 	 or scheduling.
 	 	 00 No action needed (needed to limit responses if nothing to do)
 	 	 01 Send register(s) to Master (i.e. setpoints, or data)
 	 	 02 Broadcast registers (can start at a offset and vary in number)
 	 	 03 OTA packet transfer to CCB (UIM with WiFi module or PC over the wire)
 	 	 04 Send setpoint to other slave.

 Resources:

 ========================================================================================
 History:
 *-------*-----------*---------------------------------------------------*---------------
 1.00    08-08-2019  Original code                                       Tom Van Sistine
 1.01    09-25-2020  Modified so that NFC will return nothing when it	Anish Venkataraman
					 time to broadcast
 ----------------------------------------------------------------------------------------
 */
static void MinSlave_slavePollPRIV(void) {
	uint8_t txLength;

	if(pMinSlaveSelf->slavePollBroadcastInterval) {
		pMinSlaveSelf->slavePollBroadcastInterval--;
	}

	// Setup reply first to bytes common to all responses
	txBuf[MIN_SLAVE_ADDRESS_INDEX] = pMinSlaveSelf->uart.moduleAddress;  	// Slave address
	txBuf[MIN_FUNCTION_CODE_INDEX] = MIN_FC65_SLAVE_POLL;         // Function code

	// Determine what type of response should be sent.
	if (pMinSlaveSelf->changeDataFlag) {
		// Setpoint(s) are queued up to send
		txLength = MinSlave_spSendRegistersPRIV();
	}

	else if(pMinSlaveSelf->slavePollBroadcastInterval == 0) {
		// Broadcast some slave registers
		txLength = MinSlave_spNothingToRespondPRIV();
	}

	else {
		// Send nothing to respond this time type
		txLength = MinSlave_spNothingToRespondPRIV();
	}

	// Send response - common to all types.
	// Enable the Tx line
	minTxEnable();

	// Send reply
	MinSlave_sendReplyPRIV(txBuf, txLength);
}

/*
static void ModbusSlave_slavePollPRIV(void) {
	uint8_t txLength;

	if(pMinSlaveSelf->slavePollBroadcastInterval) {
		pMinSlaveSelf->slavePollBroadcastInterval--;
	}

	// Setup reply first to bytes common to all responses
	txBuf[MODBUS_SLAVE_ADDRESS_INDEX] = pMinSlaveSelf->uart.moduleAddress;  	// Slave address
	txBuf[MODBUS_FUNCTION_CODE_INDEX] = MODBUS_FC65_SLAVE_POLL;         // Function code

	// Determine what type of response should be sent.
	if (pMinSlaveSelf->changeDataFlag) {
		// Setpoint(s) are queued up to send
		txLength = MinSlave_spSendRegistersPRIV();
	}

	else if(pMinSlaveSelf->slavePollBroadcastInterval == 0) {
		// Broadcast some slave registers
		txLength = MinSlave_spNothingToRespondPRIV();
	}

	else {
		// Send nothing to respond this time type
		txLength = MinSlave_spNothingToRespondPRIV();
	}

	// Send response - common to all types.
	// Enable the Tx line
	minTxEnable();

	// Send reply
	MinSlave_sendReplyPRIV(txBuf, txLength);
}
*/

/*
 ========================================================================================
 Method name:  MinSlave_writeRegistersPRIV()

 Originator:   Tom Van Sistine

 Description:
 	 	 Handles write multiple registers (block writes) which can be Master broadcast
 	 	 blocks (no reply) or multiple slave registers (with reply).

 Resources:

 ========================================================================================
 History:
 *-------*-----------*---------------------------------------------------*---------------
 1.00    08-07-2019  Original code broke out MinSlave_manageMessages()   Tom Van Sistine
 1.01	 11-04-2019  Added code to store data in an array and once all Anish Venkataraman
					 data is received set the flag to TRUE.	
 1.02    06-04-2020  Modified the code to store the respective block   Anish Venkataraman
					 register to NFC object					 
 1.03    06-15-2020  Added instantaneous write for max of 8 bytes	   Anish Venkataraman
					 Added code to support BLK 226 	
 1.04    06-26-2020  Refactored code								   Anish Venkataraman
 1.05    08-12-2020  Modified code to write instantaneous only when	   Anish Venkataraman					 
					 addressed to NFC and refactored it.
 1.06    08-28-2020  Added reply when addressed to NFC				   Anish Venkataraman
 1.07    02-19-2021  Added check for permission to save data		   Anish Venkataraman
 ----------------------------------------------------------------------------------------
 */

static void MinSlave_writeRegistersPRIV(void) {
	uint8_t i;							// Generic index variable used where needed
	uint8_t txLength;
	uint16_t crc;
	uint8_t * dataPtr;
	uint8_t blockNumber;
	uint8_t registerNumber;
	uint8_t dataLength;
	blockNumber = pMinSlaveSelf->uart.rxBuffer[MIN_FC16_BLOCK_INDEX]; //get block number
	registerNumber = pMinSlaveSelf->uart.rxBuffer[MIN_FC16_REGISTER_INDEX]; //get the register number
	dataLength = pMinSlaveSelf->uart.rxBuffer[MIN_FC16_BYTES_TO_RX_INDEX]; //length of the data to be written
	// Initialize source data pointer to start of register data in rxBuffer
	dataPtr = &pMinSlaveSelf->uart.rxBuffer[MIN_FC16_DATA_START_INDEX];	// Set pointer to first incoming register value high byte.
	// Check if MASTER is broadcasting a block of its registers (slave address is 0 for master broadcast)
	if (pMinSlaveSelf->uart.rxBuffer[MIN_SLAVE_ADDRESS_INDEX] == MIN_MASTER_BROADCAST && pMinSlaveSelf->slaveRegisters[MIN_SLAVE_PERMIT_TO_SAVE_DATA_RNUM] == INITIALIZED) {
		oNFC.broadcast.length = dataLength;
		
		if(blockNumber == BLOCK2 && oNFC.block2WriteFLG == TRUE){ //store block 2
			oNFC.broadcast.blockNumber = blockNumber;//store block number
			oNFC.broadcast.registerNumber = registerNumber;//store register to be written
			for (i = 0; i < (uint8_t) (dataLength); i++) {
				oNFC.broadcast.nfcBuffer[i] = *dataPtr++;//store data
			}
			oNFC.storeBroadcastFLG = TRUE;
			
		}
		else if(blockNumber == BLOCK11 && oNFC.block11WriteFLG == TRUE){//store block 11
			oNFC.broadcast.blockNumber = blockNumber;
			oNFC.broadcast.registerNumber = registerNumber;
			for (i = 0; i < (uint8_t) (dataLength); i++) {
				oNFC.broadcast.nfcBuffer[i] = *dataPtr++;
			}
			oNFC.storeBroadcastFLG = TRUE;
		}
		else if(blockNumber == BLOCK226 && oNFC.block226WriteFLG == TRUE){//store block 226
		oNFC.broadcast.blockNumber = blockNumber;
		oNFC.broadcast.registerNumber = registerNumber;
		for (i = 0; i < (uint8_t) (dataLength); i++) {
				oNFC.broadcast.nfcBuffer[i] = *dataPtr++;
			}
			oNFC.storeBroadcastFLG = TRUE;
		}
		else{
			//do nothing
		}		
		// Note: there is no reply for master broadcast of a its block.
	}
	//write data to NFC memory instantly
	else if (pMinSlaveSelf->uart.rxBuffer[MIN_SLAVE_ADDRESS_INDEX] == NFC_SLAVE_ADDRESS && pMinSlaveSelf->slaveRegisters[MIN_SLAVE_PERMIT_TO_SAVE_DATA_RNUM] == INITIALIZED) {
		oNFC.update.length = dataLength;
		if(blockNumber == BLOCK2 || blockNumber == BLOCK11 || blockNumber == BLOCK226){//save Block instantly
			oNFC.update.blockNumber = blockNumber;
			oNFC.update.registerNumber = registerNumber;
			for (i = 0; i < (uint8_t) (dataLength); i++) {
				oNFC.update.nfcBuffer[i] = *dataPtr++;
			}
			oNFC.instantWriteFLG = TRUE;
			// Reply is required for specific slave writing of registers. Reply is same as request for first 6 bytes received
			dataPtr = &pMinSlaveSelf->uart.rxBuffer[MIN_SLAVE_ADDRESS_INDEX];

			for (i = 0; i < MIN_FC_WRITE_REPLY_LENGTH; i++) {
				txBuf[i] = *dataPtr++;
			}
			crc = get_crc_16 (0xFFFF, txBuf, MIN_FC_WRITE_REPLY_LENGTH);         // Calculate the CRC to send with reply slaveAddr, FC, addrH, addrL, dataH, datal
			txBuf[MIN_FC_CRCL_INDEX] = (uint8_t) (crc & 0xff);             // CRCL
			txBuf[MIN_FC_CRCH_INDEX] = (uint8_t) (crc >> 8);               // CRCH
			txLength = MIN_FC_WRITE_REPLY_LENGTH + MIN_CRC_LENGTH;
			// Enable the Tx line
			minTxEnable();

		// Send reply
		MinSlave_sendReplyPRIV(txBuf, txLength);
		}
		
	}

}

/*
 ========================================================================================
 Method name:  ModbusSlave_slavePollPRIV()

 Originator:   Tom Van Sistine

 Description:
 	 	 Respond to slave poll command with one of several responses based on priority
 	 	 or scheduling.
 	 	 00 No action needed (needed to limit responses if nothing to do)
 	 	 01 Send register(s) to Master (i.e. setpoints, or data)
 	 	 02 Broadcast registers (can start at a offset and vary in number)
 	 	 03 OTA packet transfer to CCB (UIM with WiFi module or PC over the wire)
 	 	 04 Send setpoint to other slave.

 Resources:

 ========================================================================================
 History:
 *-------*-----------*---------------------------------------------------*---------------
 1.00    08-08-2019  Original code                                       Tom Van Sistine
 1.01    09-25-2020  Modified so that NFC will return nothing when it	Anish Venkataraman
					 time to broadcast
 ----------------------------------------------------------------------------------------
 */
static void ModbusSlave_slavePollPRIV(void) {
	uint8_t txLength;

	if(pMinSlaveSelf->slavePollBroadcastInterval) {
		pMinSlaveSelf->slavePollBroadcastInterval--;
	}

	// Setup reply first to bytes common to all responses
	txBuf[MODBUS_SLAVE_ADDRESS_INDEX] = pMinSlaveSelf->uart.moduleAddress;  	// Slave address
	txBuf[MODBUS_FUNCTION_CODE_INDEX] = MODBUS_FC65_SLAVE_POLL;         // Function code

	// Determine what type of response should be sent.
	if (pMinSlaveSelf->changeDataFlag) {
		// Setpoint(s) are queued up to send
		txLength = MinSlave_spSendRegistersPRIV();
	}

	else if(pMinSlaveSelf->slavePollBroadcastInterval == 0) {
		// Broadcast some slave registers
		txLength = MinSlave_spNothingToRespondPRIV();
	}

	else {
		// Send nothing to respond this time type
		txLength = MinSlave_spNothingToRespondPRIV();
	}

	// Send response - common to all types.
	// Enable the Tx line
	minTxEnable();

	// Send reply
	MinSlave_sendReplyPRIV(txBuf, txLength);
}

/*
 ========================================================================================
 Method name:  MinSlave_spSendRegistersPRIV()

 Originator:   Tom Van Sistine

 Description:
		Prepare to send a setpoint (or multiple registers) to CCB.

 Resources:

 ========================================================================================
 History:
 *-------*-----------*---------------------------------------------------*---------------
 1.00    08-09-2019  Original code                                       Tom Van Sistine

 ----------------------------------------------------------------------------------------
 */
static uint8_t MinSlave_spSendRegistersPRIV(void) {
	uint8_t i;
	uint16_t crc;
	uint8_t * dataPtr;
	pMinSlaveSelf->changeDataFlag = FALSE;
	dataPtr = &txBuf[MIN_FC65_TYPE_CODE_INDEX];
	*dataPtr++ = MIN_FC65_SETPOINT_RESPONSE_CODE;
	*dataPtr++ = pMinSlaveSelf->blockToChange;
	*dataPtr++ = pMinSlaveSelf->regToChange;
	*dataPtr++ = pMinSlaveSelf->numRegsToChange;
	for (i = 0; i < pMinSlaveSelf->numRegsToChange; i++) {
		*dataPtr++ = (uint8_t) (*pMinSlaveSelf->pDataToChange >> 8);  	// High byte of register
		*dataPtr++ = (uint8_t) (*pMinSlaveSelf->pDataToChange++ & 0xff);	// Low byte of register
	}

	crc = get_crc_16 (0xFFFF, txBuf, MIN_FC65_SETPOINT_RESPONSE_PREFIX_LENGTH + (i * 2));         // Calculate the CRC to send with reply
	*dataPtr++ = (uint8_t) (crc & 0xff);   // CRCL
	*dataPtr++ = (uint8_t) (crc >> 8); // CRCH
	return ((uint8_t) (dataPtr - txBuf));

}

/*
 ========================================================================================
 Method name:  MinSlave_spNothingToRespondPRIV()

 Originator:   Tom Van Sistine

 Description:
 	 	 There is nothing the slave is needing to do so send reply back to that affect.

 Resources:

 ========================================================================================
 History:
 *-------*-----------*---------------------------------------------------*---------------
 1.00    08-09-2019  Original code                                       Tom Van Sistine
----------------------------------------------------------------------------------------
 */
static uint8_t MinSlave_spNothingToRespondPRIV(void) {
	uint16_t crc;
	uint8_t txLength;
	uint8_t * dataPtr;

	txBuf[MIN_FC65_TYPE_CODE_INDEX] = MIN_FC65_NO_RESPONSE_CODE;

	// Set destination pointer to txBuf
	dataPtr = &txBuf[MIN_FC65_TYPE_CODE_INDEX + 1];
	crc = get_crc_16 (0xFFFF, txBuf, (MIN_FC65_NO_RESPONSE_LENGTH - MIN_CRC_LENGTH));         // Calculate the CRC to send with reply
	*dataPtr++ = (uint8_t) (crc & 0xff);   // CRCL
	*dataPtr = (uint8_t) (crc >> 8); // CRCH
	txLength = MIN_FC65_NO_RESPONSE_LENGTH;
	return txLength;
}

/*
 ========================================================================================
 Method name:  MinSlave_sendReplyPRIV()

 Originator:	Tom Van Sistine

 Description: 	Transmits reply back to Master.


 Resources:		HardwareUart_SendChar() needs to be defined in IoTranslate.h

 ========================================================================================
 History:
 *-------*-----------*---------------------------------------------------*---------------
 1.00    07-11-2019  Original code                                       Tom Van Sistine

 ----------------------------------------------------------------------------------------
 */
void MinSlave_sendReplyPRIV(uint8_t * pSrc, uint8_t txLength) {
	uint8_t i;
	unsigned char data;
	assert(txLength);
	assert(pSrc);
	for (i = 0; i < txLength; i++) {
		data = (unsigned char)*pSrc++;
		(void) HardwareUart_SendChar(data);
	}
	if(i == txLength){
		Enable_TXInterrupt(); 
	}
}


/*
=======================================================================================
Method name:	MinSlave__setupSetpointChange()

Originator:		Tom Van Sistine

Description:	Called externally to setup change in a setpoint, by writing  passed
				arguments to its members.


Resources:

=======================================================================================
History:
 *-----*-----------*-----------------------------------------------------*--------------
1.00  07-11-2019  Original code											Tom Van Sistine
---------------------------------------------------------------------------------------
 */
uint8_t MinSlave_setupSetpointChange(uint8_t block, uint8_t registerNum, uint16_t *pData, uint8_t numRegs) {

	// If pointer to itself has not been initialized, return.
	if (pMinSlaveSelf == 0) {
		return MIN_NOT_INITIALIZED;
	}
	pMinSlaveSelf->blockToChange = block;		// Block number of register to change.
	pMinSlaveSelf->regToChange = registerNum;	// Register number start to change.
	pMinSlaveSelf->pDataToChange = pData;		// Pointer to source Data for registers to change.
	pMinSlaveSelf->numRegsToChange = numRegs;	// Number of registers to change
	pMinSlaveSelf->changeDataFlag = TRUE;		// Indicates to MinSlave_ManageMessage() that a
	//   register needs to be sent.
	return MINSLAVE_PUT_SLAVE_DATA_SUCCESS;
}


/*
=======================================================================================
Method name:  	MinSlave__getSlaveData()

Originator:   	Tom Van Sistine

Description:	Getter function for other modules to get protected MIN slave data
				in MinSlave.  Register number passed along with a pointer
				to where to store the uint16_t data.

				Returned values
				MINSLAVE_GET_SLAVE_DATA_SUCCESS = 0,
				MINSLAVE_GET_SLAVE_DATA_FAIL_NULL_POINTER,
				MINSLAVE_GET_SLAVE_DATA_FAIL_INVALID_REGISTER,

Resources:

=======================================================================================
History:
 *-------*-----------*---------------------------------------------------*--------------
1.00	07-11-2019	Original code                                       Tom Van Sistine

---------------------------------------------------------------------------------------
 */
uint8_t MinSlave_getSlaveData(uint8_t reg, uint16_t *returnData) {

	// If pointer to itself has not been initialized, return.
	if (pMinSlaveSelf == 0) {
		return MIN_NOT_INITIALIZED;
	}

	if (returnData == 0) {
		return MINSLAVE_GET_SLAVE_DATA_FAIL_NULL_POINTER;
	}
	else if (reg >= MIN_SLAVE_NUMBER_OF_REGISTERS) {
		return MINSLAVE_GET_SLAVE_DATA_FAIL_INVALID_REGISTER;
	}
	else {
		*returnData = pMinSlaveSelf->slaveRegisters[reg];
		return MINSLAVE_GET_SLAVE_DATA_SUCCESS; // success
	}
}

/*
=======================================================================================
Method name:  	MinSlave__putSlaveData()

Originator:   	Tom Van Sistine

Description:	Putter function for other modules to put protected MIN slave data
				in MinSlave.  Register number passed along with the value to store
				uint16_t data.

				Returned values
				MINSLAVE_PUT_SLAVE_DATA_SUCCESS = 0,
				MINSLAVE_PUT_SLAVE_DATA_FAIL_INVALID_REGISTER,

Resources:

=======================================================================================
History:
 *-------*-----------*---------------------------------------------------*--------------
1.00	07-11-2019	Original code                                       Tom Van Sistine

---------------------------------------------------------------------------------------
 */

uint8_t MinSlave_putSlaveData(uint8_t reg, uint16_t data) {   // Putter function for MIN slave registers

	// If pointer to itself has not been initialized, return.
	if (pMinSlaveSelf == 0) {
		return MIN_NOT_INITIALIZED;
	}

	if (reg >= MIN_SLAVE_NUMBER_OF_REGISTERS) {
		return MINSLAVE_PUT_SLAVE_DATA_FAIL_INVALID_REGISTER;
	}
	else {
		pMinSlaveSelf->slaveRegisters[reg] = data;
		return MINSLAVE_PUT_SLAVE_DATA_SUCCESS; // success
	}
}

/*
 ========================================================================================
 Method name:  MinSlave_replyRegisterRequestPRIV()

 Originator:   Tom Van Sistine

 Description:

 	 	 Replies with the data from slave register requested by master
 Resources:

 ========================================================================================
 History:
 *-------*-----------*---------------------------------------------------*---------------
 1.00    08-07-2019  Original code broke out MinSlave_manageMessages()   Tom Van Sistine
 1.01	 11-04-2019  Added code to read from NFC and append it to the  Anish Venkataraman
					 MIN_SLAVE_MODEL_CONFIGURATION_CODE.
 1.02	 04-15-2020  Added code to read Block 2 and 11				   Anish Venkataraman
 1.03	 06-15-2020  Added code to read Block 226					   Anish Venkataraman
 1.04	 06-26-2020  Modified the code to support sequential read	   Anish Venkataraman
 ----------------------------------------------------------------------------------------
 */
static void MinSlave_replyRegisterRequestPRIV(void) {

	uint8_t i = 0;;							// Generic index variable used where needed
	uint8_t * dataPtr;
	uint16_t crc;
	uint8_t txLength;
	uint8_t blockNumber = 0;
	uint8_t registerNumber;				//Requested Register
	uint16_t address;
	uint8_t length;
	uint8_t read[NFC_MAX_MEM];	//array to store read data
	// Setup reply
	txBuf[MIN_SLAVE_ADDRESS_INDEX] = pMinSlaveSelf->uart.moduleAddress;  	// Slave address
	//txBuf[MIN_FUNCTION_CODE_INDEX] = MIN_FC03;                    			// Function code
	txBuf[MIN_FC03_BYTES_REPLY_INDEX] = pMinSlaveSelf->uart.rxBuffer[MIN_FC03_NUM_REG_INDEX] * MIN_BYTES_PER_REG;
	length = pMinSlaveSelf->uart.rxBuffer[MIN_FC03_NUM_REG_INDEX] * MIN_BYTES_PER_REG; 
	// Get requested registers
	blockNumber = pMinSlaveSelf->uart.rxBuffer[MIN_FC16_BLOCK_INDEX];
	//Set pointer to the Tx Buffer
	dataPtr = &txBuf[MIN_FC03_DATA_START];
	//Store Register Number
	registerNumber = (uint8_t) ((pMinSlaveSelf->uart.rxBuffer[MIN_FC03_START_ADDR_INDEX] * 2));

	////Reply with Block 2 Data
	//if (blockNumber == BLOCK2) {
		//address = (uint16_t)(BLOCK2_OFFSET + registerNumber);
	//}
	////reply with Block 11 Data
	//else if(blockNumber == BLOCK11){
		//address = (uint16_t)(BLOCK11_OFFSET + registerNumber);	
	//}
	//else{
		//address = (uint16_t)(BLOCK226_OFFSET + registerNumber);
	//}
	
	address = (uint16_t)(HOLDING_REG_OFFSET + registerNumber);
	
	NFC_SequentialRead(address,length,read);
	//Copy to txBuf
	for (i = 0; i < length; i++) {
		*dataPtr++ = read[i];
	}
	crc = get_crc_16 (0xFFFF, txBuf, MIN_FC03_START_ADDR_INDEX + length);// Calculate the CRC to send with reply
	*dataPtr++ = (uint8_t) (crc & 0xff);   // CRCL
	*dataPtr = (uint8_t) (crc >> 8); // CRCH
	txLength = (MIN_FC03_DATA_START + length + MIN_CRC_LENGTH);
	// Enable the Tx line
	minTxEnable();
	// Send reply
	MinSlave_sendReplyPRIV(txBuf, txLength);

}

/*
 ========================================================================================
 Method name:  ModbusSlave_replyRegisterRequestPRIV()

 Originator:   Tom Van Sistine

 Description:

 	 	 Replies with the data from slave register requested by master
 Resources:

 ========================================================================================
 History:
 *-------*-----------*---------------------------------------------------*---------------
 1.00    08-07-2019  Original code broke out MinSlave_manageMessages()   Tom Van Sistine
 1.01	 11-04-2019  Added code to read from NFC and append it to the  Anish Venkataraman
					 MIN_SLAVE_MODEL_CONFIGURATION_CODE.
 1.02	 04-15-2020  Added code to read Block 2 and 11				   Anish Venkataraman
 1.03	 06-15-2020  Added code to read Block 226					   Anish Venkataraman
 1.04	 06-26-2020  Modified the code to support sequential read	   Anish Venkataraman
 1.05	 08-11-2022	 Modified the code to support Modbus read		   Onkar Raut
 ----------------------------------------------------------------------------------------
 */
static void ModbusSlave_replyRegisterRequestPRIV(void) {
	
	unsigned char temp;

	uint8_t i = 0;;							// Generic index variable used where needed
	uint8_t * dataPtr;
	uint16_t crc;
	uint8_t txLength;
	uint8_t blockNumber = 0;
	uint16_t registerNumber;				//Requested Register
	uint16_t address;
	uint8_t length;
	uint8_t read[NFC_MAX_MEM];	//array to store read data
	
	
	FlagEEPBusy = EEPBusy;
	
	// Setup reply
	txBuf[MODBUS_SLAVE_ADDRESS_INDEX] = pMinSlaveSelf->uart.moduleAddress;  	// Slave address
	txBuf[MIN_FUNCTION_CODE_INDEX] = MIN_FC03;                    			// Function code
	txBuf[MODBUS_FC03_BYTES_REPLY_INDEX] = pMinSlaveSelf->uart.rxBuffer[MODBUS_FC03_NUM_REG_INDEX] * MIN_BYTES_PER_REG;
	length = pMinSlaveSelf->uart.rxBuffer[MODBUS_FC03_NUM_REG_INDEX] * MIN_BYTES_PER_REG; 
	// Get requested registers
	blockNumber = pMinSlaveSelf->uart.rxBuffer[MODBUS_FC16_BLOCK_INDEX];
	//Set pointer to the Tx Buffer
	dataPtr = &txBuf[MODBUS_FC03_DATA_START];
	//Store Register Number

//	registerNumber = ((uint16_t)(pMinSlaveSelf->uart.rxBuffer[MODBUS_FC03_START_ADDR_INDEX]) - 0x40);

	registerNumber = (uint16_t)(pMinSlaveSelf->uart.rxBuffer[MODBUS_FC03_MSB_ADDR_INDEX]);
	registerNumber = registerNumber << 8;
	registerNumber = registerNumber | (uint16_t)(pMinSlaveSelf->uart.rxBuffer[MODBUS_FC03_START_ADDR_INDEX]);
	registerNumber = ((uint16_t)registerNumber - 0x9C40);
	
	registerNumber = (uint16_t)registerNumber * 2;
	
//	registerNumber = registerNumber ;
//	registerNumber = (uint8_t) ((pMinSlaveSelf->uart.rxBuffer[MODBUS_FC03_START_ADDR_INDEX] * 2));

	////Reply with Block 2 Data
	//if (blockNumber == BLOCK2) {
		//address = (uint16_t)(BLOCK2_OFFSET + registerNumber);
	//}
	////reply with Block 11 Data
	//else if(blockNumber == BLOCK11){
		//address = (uint16_t)(BLOCK11_OFFSET + registerNumber);	
	//}
	//else{
		//address = (uint16_t)(BLOCK226_OFFSET + registerNumber);
	//}
	
//	address = (0x20 + registerNumber);
	address = (uint16_t)registerNumber;
	
	NFC_SequentialRead(address,length,read);
	//Copy to txBuf
	for (i = 0; i < length; i++) {
		*dataPtr++ = read[i];
	}
	crc = get_crc_16 (0xFFFF, txBuf, MODBUS_FC03_START_ADDR_INDEX + length);// Calculate the CRC to send with reply
	*dataPtr++ = (uint8_t) (crc & 0xff);   // CRCL
	*dataPtr = (uint8_t) (crc >> 8); // CRCH
	txLength = (MODBUS_FC03_DATA_START + length + MODBUS_CRC_LENGTH);
	// Enable the Tx line
	minTxEnable();
	// Send reply
	MinSlave_sendReplyPRIV(txBuf, txLength);
	
	FlagEEPBusy = EEPFree;
	
//	temp = registerNumber & 0x00FF;

//	(void) HardwareUart_SendChar(temp);

//	temp = registerNumber & 0xFF00;
//	temp = (uint8_t)(registerNumber >>8);
//	(void) HardwareUart_SendChar(temp);

}

/*
 ========================================================================================
 Method name:  MinSlave_storeProductInfoPRIV()

 Originator:   Anish Venkataraman

 Description:

 	 	 Handles store of data critical to verifying controls are compatible with the
		 heater they are on
 Resources:

 ========================================================================================
 History:
 *-------*-----------*---------------------------------------------------*---------------
 1.00    04-15-2020  Original code									  Anish Venkataraman
 1.01    07-07-2020  Modified to write data instantly to NFC		  Anish Venkataraman
 1.02    08-28-2020  Modified to reply with ProductInfoNVM crc		  Anish Venkataraman
 1.03    08-28-2020  Restructured code, removed reference to		  Anish Venkataraman\
					 ProductInfoNVMSTYP.
 ----------------------------------------------------------------------------------------
 */
static void MinSlave_storeProductInfoPRIV(void) {
	uint8_t i;
	uint8_t * dataPtr;
	uint16_t crc;
	uint8_t index = 0;
	if(pMinSlaveSelf == 0) {
		return;
	}
	//Save productInfoLength
	oNFC.productInfoLength = pMinSlaveSelf->uart.rxBuffer[MIN_FC70_BYTES_TO_RX_INDEX] - MIN_FC70_PAYLOAD;
	//Save the Initialized Variable
	for(i = 0; i < oNFC.productInfoLength; i++) {
		oNFC.productInfoData[i] = pMinSlaveSelf->uart.rxBuffer[MIN_FC70_DATA_START_INDEX+i];
	}
	// Reply is required for specific slave writing of registers. Reply is same as request for first 6 bytes received
	dataPtr = &pMinSlaveSelf->uart.rxBuffer[MIN_SLAVE_ADDRESS_INDEX];
	for(i = 0; i < 2; i++){
		txBuf[index++] = *dataPtr++;
	}
	//update the config code sent from master MSB byte << 8 + LSB byte
	pMinSlaveSelf->slaveRegisters[MIN_SLAVE_MODEL_CONFIGURATION_CODE] = (((uint16_t)oNFC.productInfoData[oNFC.productInfoLength - 3] << 8) | (uint16_t)oNFC.productInfoData[oNFC.productInfoLength - 4]);
	txBuf[index++] = oNFC.productInfoData[oNFC.productInfoLength - 2];//CRC LSB
	txBuf[index++] = oNFC.productInfoData[oNFC.productInfoLength - 1];//CRC MSB
	crc = get_crc_16 (0xFFFF, txBuf, index);         // Calculate the CRC to send with reply slaveAddr
	txBuf[index++] = (uint8_t) (crc & 0xff);             // CRCL
	txBuf[index++] = (uint8_t) (crc >> 8);               // CRCH
	
	// Enable the Tx line
	minTxEnable();

	// Send reply
	MinSlave_sendReplyPRIV(txBuf, index);
	oNFC.productInfoFLG = TRUE;
}



/*
 ========================================================================================
 Method name:  MinSlave_getProductInfoPRIV()

 Originator:   Anish Venkataraman

 Description:

 	 	 Replies with the stored data critical to verifying controls are compatible 
		 with the heater they are on
 Resources:

 ========================================================================================
 History:
 *-------*-----------*---------------------------------------------------*---------------
 1.00    07-07-2020  Original code									  Anish Venkataraman
 1.01    05-10-2021  Modified code to support requested length		  Anish Venkataraman
					 instead of a fixed macro.
 ----------------------------------------------------------------------------------------
 */
static void MinSlave_getProductInfoPRIV(void) {
	uint8_t i;
	uint8_t * dataPtr;
	uint8_t readBuf[NFC_MAX_MEM];	//array to store read data
	uint8_t length;
	uint16_t crc;
	uint8_t txLength;
	txBuf[MIN_SLAVE_ADDRESS_INDEX] = pMinSlaveSelf->uart.moduleAddress;  	// Slave address
	txBuf[MIN_FUNCTION_CODE_INDEX] = MIN_FC69_GET_PRODUCT_INFO;            // Function code
	txBuf[MIN_FC69_BYTES_TO_RX_INDEX] =  pMinSlaveSelf->uart.rxBuffer[MIN_FC69_DATA_LENGTH_INDEX] * 2; // no. of registers * bytes per reg(2)
	length =  pMinSlaveSelf->uart.rxBuffer[MIN_FC69_DATA_LENGTH_INDEX] * 2;
	//set poitner to tx buffer
	dataPtr = &txBuf[MIN_FC69_DATA_START_INDEX];
	//read NFC memory
	NFC_SequentialRead(BLOCK_NVM_OFFSET,length,readBuf);
	for(i = 0; i < length; i++) {
		*dataPtr++ = readBuf[i];
	}
	//calculate crc 
	crc = get_crc_16 (0xFFFF, txBuf, MIN_FC69_DATA_START_INDEX + length);// Calculate the CRC to send with reply
	*dataPtr++ = (uint8_t) (crc & 0xff);   // CRCL
	*dataPtr = (uint8_t) (crc >> 8); // CRCH
	txLength = (MIN_FC03_DATA_START + length + MIN_CRC_LENGTH);
	// Enable the Tx line
	minTxEnable();
	// Send reply
	MinSlave_sendReplyPRIV(txBuf, txLength);
}



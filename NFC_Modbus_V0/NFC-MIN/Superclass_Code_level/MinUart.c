/*
=======================================================================================
File name:    MinUart.c
                    
Originator:   Tom Van Sistine

Description:
			  Hardware dependent class that handles characters as they are received
			  and determines what type of message it is.  Also sends the message and
			  resets the Rx/Tx enable line after the transmit is complete to make
			  sure all characters are transmitted before receiving more.


=======================================================================================
 History:	
*-------*-----------*---------------------------------------------------*--------------
1.00	07-10-2019  New file											Tom Van Sistine
1.01	12-09-2019	Removed change of baud rate module	 			   Anish Venkataraman
---------------------------------------------------------------------------------------
*/

#include "Build.h"
#include "MinUart.h"

//lint -e9029	suppress "Mismatched essential type" PC-Lint 9.00k  Bug in PC-Lint does not like subscripts!

/*
=======================================================================================
Method name:    MinUart_init()
                    
Originator:   	Tom Van Sistine

Description:
	Initializes the MIN driver.

  
=======================================================================================
 History: (Identify changes in this method)	
*-------*-----------*---------------------------------------------------*--------------
1.00	07-18-2012	Initial Write										Tom Van Sistine
1.01	12-09-2019	Removed change of baud rate module	 			   Anish Venkataraman
---------------------------------------------------------------------------------------
*/
void MinUart_init(MinUart_STYP *uart) {
	uart->processPacketFlag = FALSE;
	uart->currentState = (uint8_t)   WAIT_FOR_RESYNC;
	uart->rxBufferIndex = 0U;
	uart->badCrcFlag = 0U;
	uart->dataLength = 10;

	// Reinitialize the hardware UART
	HardwareUart_Init();  
	
}

/*
=======================================================================================
Method name:    MinUart_serviceRx()
                    
Originator:   	Tom Van Sistine

Description: 	Called by MinSlave_manageMessages() when byte received from MIN master.
				THEREFORE: This is called from an interrupt and must be kept short and
				efficient.

				Handles parsing the message.

				When the message is parsed and the CRC is correct it will set
				processPacketFlag to let MinSlave know to respond.

				MIN support Modbus RTU Slave communications.


				Three Basic Function Code commands are supported as well as a number of
				custom Function codes as described in Modbus Internal Network protocol.docx
				as well as protocol details

				The three basic commands are :
				FC03    Read holding registers
				FC06    Write Single Holding Register
				FC16    Write Multiple Holding Registers (used for broadcasts)

				User defined codes are FC65 to 71 (see MinSlave.c MinSlave_manageMessages()
				Description section for brief description or the above mentioned document
				for details).

				Synchronization is supposed to be minimum of 3.5
				characters worth of time without character sent between requests. This
				uses a count down timer reloaded when a character is received and
				decremented in Events.c 1msec timer interrupt.


=======================================================================================
 History: (Identify changes in this method)	
*-------*-----------*---------------------------------------------------*--------------
1.00	07-18-2012	Initial Write										Tom Van Sistine
1.01	12-09-2019	Removed change of baud rate module	 			   Anish Venkataraman
---------------------------------------------------------------------------------------
*/

void MinUart_serviceRx(MinUart_STYP *uart) {

static uint8_t rxByte = 0;			// Because a pointer is passed with HardwareUart_RecvChar() should not be auto-variable.
uint8_t error = 0;
uint16_t crc;
	// Get character from buffer
	error = HardwareUart_RecvChar(rxByte);
	
	if (error) {
		uart->currentState = WAIT_FOR_RESYNC;	// If in the middle of a message, this will wait until the end.
		HardwareUart_clearRxBuf();
		// Reset stuff needed in next character received is start of new message.
		uart->badCrcFlag = 0;
		uart->rxBufferIndex = 0;
		uart->dataLength = 10;
		return;
	}

	// NOTE: modbusReSync is decremented every 1msec in a timer interrupt.
	// Check if modbusReSync is counter is 0 which indicates start of new message.  If between
	// messages, it stays in WAIT_FOR_RESYNC state until modbusReSync counter is 0.
	// If modbusReSync is 0, then, regardless of state, force it to state HANDLE_DEVICE_ID to re-
	// synchronize communication.

    if (uart->modbusReSync == 0) {
        uart->currentState = PARSE_INCOMING;
		uart->badCrcFlag = 0;
		uart->rxBufferIndex = 0;
		uart->dataLength = 10;
    }
    uart->modbusReSync = MODBUS_RESYNC_LOAD;	// Set for three 1msec interrupts that are asynchronous to MIN_OnRxChar()
    											// to insure a minimum of 2 msec without receiving byte to indicate resync.

	// Run the UART character receive method state machine which parses the various types
	// of messages from the Master device
	switch (uart->currentState) {

	case WAIT_FOR_RESYNC:

		// Waiting for resync, i.e. no characters for 2 msec.
		uart->badCrcFlag = 0;
		uart->rxBufferIndex = 0;
		uart->dataLength = 10;
		USART_ClearRxBuffer();
		break;

	case PARSE_INCOMING:

		// Resync timeout elapsed

		// Save received byte
		uart->rxBuffer[uart->rxBufferIndex] = rxByte;
		
		// Check if slave address matches this slave device
		if (uart->rxBufferIndex == 0) {
			if((rxByte != uart->moduleAddress) && rxByte != (uint8_t) MIN_MASTER_BROADCAST) {
				uart->currentState = WAIT_FOR_RESYNC;  // Message not for this slave.
			}
		}
		
		// Check if FC code then set expected number of bytes
		else if (uart->rxBufferIndex == (uint8_t) MIN_FUNCTION_CODE_INDEX) {
			uart->functionCode = rxByte;
			switch (uart->functionCode) {

			case MIN_FC03:
				uart->dataLength = NUMBER_REQUEST_BYTES_FC03;
				break;

			case MIN_FC06:
				uart->dataLength = NUMBER_REQUEST_BYTES_FC06;
				break;

			case MIN_FC16:
				uart->dataLength = NUMBER_REQUEST_BYTES_FC16;
				break;

			case MIN_FC65_SLAVE_POLL:
				uart->dataLength = NUMBER_REQUEST_BYTES_FC65;
				break;

			case MIN_FC67_DISCOVER:
				uart->dataLength = NUMBER_REQUEST_BYTES_FC67;
				break;
			
			case MIN_FC69_GET_PRODUCT_INFO:
				uart->dataLength = NUMBER_REQUEST_BYTES_FC69;
				break;
				
			case MIN_FC70_STORE_PRODUCT_INFO:
				uart->dataLength = NUMBER_REQUEST_BYTES_FC70;
				break;

			default: // Unknown FC code
				uart->currentState = WAIT_FOR_RESYNC;
				HardwareUart_clearRxBuf();
				break;
			}
		}

		// FC16 received and number of data bytes is rxByte?
		if ((uart->functionCode == (uint8_t) MIN_FC16) && (uart->rxBufferIndex == (uint8_t) MIN_FC16_BYTES_TO_RX_INDEX)) {
			
			uart->dataLength = rxByte + 9;
		}
		// FC70 received and number of data bytes is rxByte?
		else if((uart->functionCode == (uint8_t) MIN_FC70_STORE_PRODUCT_INFO) && (uart->rxBufferIndex == (uint8_t) MIN_FC70_BYTES_TO_RX_INDEX)){
			uart->dataLength = rxByte;
		}	
		// Increment buffer pointer
		
		uart->rxBufferIndex += 1;
		
		// Have all bytes arrived?

		if (uart->rxBufferIndex >= uart->dataLength) {
            // All bytes received

			// Reset state for next incoming message regardless of CRC check.
			uart->currentState = WAIT_FOR_RESYNC;   // In case extra bytes sent
			uart->rxBufferIndex = 0;				// Make sure next received character is assumed to be first.

			// Check CRC, if good signal calling method to process it and reply.
			crc = get_crc_16(0xFFFF, uart->rxBuffer, (uart->dataLength - 2));
			
			if (crc == (((uint16_t) uart->rxBuffer[uart->dataLength - 1] << 8) +  (uint16_t) uart->rxBuffer[uart->dataLength - 2])) {
				
		        // CRC is good!
		    	// Set flag to process message upon return to MinSlave_manageMessage()
				uart->badCrcFlag = FALSE;
		    	uart->processPacketFlag = TRUE;
			}
			else {
				uart->badCrcFlag = TRUE;
				uart->processPacketFlag = FALSE;
			}
		}

	} // End switch current state


}

/*--------------------------------------------------------------------------------------------
Name:           get_crc_16()

Description:    Calculates a CRC-16 on n bytes starting at *p.  Seed value past is start.
                Resturns CRC value.

Revision History
Date        Programmer        Rev     Description
*-----------*-----------------*-------*--------------------------------------------------------
01-01-2018  Tom Van Sistine   1.00    Copy routine from http://akbar.marlboro.edu/~mahoney/support/alg/alg/node186.html
*/
/*
 ========================================================================================
 Method name:  	get_crc_16()

 Originator:    Tom Van Sistine

 Description:   Calculates a CRC-16 on n bytes starting at *p.  Seed value past is start.
				Returns CRC value.

 	 	 	 	Copy routine from web copy of algorithm; reference:
 	 	 	 	  Binstock, Andrew and Rex, John. 1995, Practical Algorithms for Programmers
 	 	 	 	  (Reading, MA: Addison-Wesley), pp. 541-543.

 	 	 	 	I copied it to a paper CRC - Byte and Nibble Based CRC rountines.docx
 	 	 	 	in DynamSoft $/Department Standards/Department Processes/Project Development Process/Misc docs.

 Resources:

 ========================================================================================
 History:
 *-------*-----------*---------------------------------------------------*---------------
 1.00    07-11-2019  Original code                                       Tom Van Sistine

 ----------------------------------------------------------------------------------------
 */
uint16_t crc_16_table[16] = {0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00, 0x2800, 0xE401,
  0xA001, 0x6C00, 0x7800, 0xB401, 0x5000, 0x9C01, 0x8801, 0x4400 };

uint16_t get_crc_16 (uint16_t start, uint8_t *p, uint16_t n) {
uint16_t crc = start;
uint16_t r;

  /* while there is more data to process */
  while (n-- > 0) {

    /* compute checksum of lower four bits of *p */
    r = crc_16_table[crc & 0xF];
    crc = (crc >> 4) & 0x0FFF;
    crc = crc ^ r ^ crc_16_table[*p & 0xF];

    /* now compute checksum of upper four bits of *p */
    r = crc_16_table[crc & 0xF];
    crc = (crc >> 4) & 0x0FFF;
    crc = crc ^ r ^ crc_16_table[(*p >> 4) & 0xF];

    /* next... */
    p++;
  }

  return(crc);
}


/*
================================================================================================
Method name:    MinUart_serviceTx()
                    
Originator:   	Tom Van Sistine

Description:  	Services the onTxComplete after MIN reply to command or poll is sent out.
				Turns off RS-485 driver, clears Rx buffer and turns on UART receive.

  
=======================================================================================
 History: (Identify changes in this method)	
*-------*-----------*---------------------------------------------------*--------------
1.00	07-10-2019	Initial Write										Tom Van Sistine
---------------------------------------------------------------------------------------
*/

void MinUart_serviceTx(MinUart_STYP *uart){
	// If the last byte has been transmitted then change the 485 to receive
	MinUart_init(uart);  // Reset everything.
	minRxEnable();
}



/*=====================================================================================
File name:    Scheduler.c                  
                    
Originator:   Tom Van Sistine

  
=======================================================================================
 History:	
*-------*-----------*---------------------------------------------------*--------------
1.00	01-13-2017	New File											Tom Van Sistine
1.01	11-04-2019	Adapted from CPAM and added 					 Anish Venkataraman
					Scheduler_serviceNFCWrite() function.
1.02	11-21-2019	Added a function Scheduler_checkNFCWrite		 Anish Venkataraman
					and modified Scheduler_serviceNFCWrite().
1.03	06-15-2020	Removed function Scheduler_checkNFCWrite		 Anish Venkataraman
					and modified Scheduler_serviceNFCWrite().
1.04	08-28-2020	Modified Scheduler_ManageMessage() to			 Anish Venkataraman
					to support block write. Added helper methods
					Scheduler_writeProductInfo(void)
					Scheduler_writeInstant(void)
					Scheduler_writeBroadcast(void)
1.04	08-28-2020	Scheduler_ManageTask() V1.05					 Anish Venkataraman
1.05	05-10-2021	Scheduler_writeProductInfo() 1.01				 Anish Venkataraman
---------------------------------------------------------------------------------------
*/
//Includes
#include "NFC.h"
#include "Scheduler.h"
#include "MinSlave.h"
#include "LedCtrl.h"


extern MinSlave_STYP oMinSlave;
extern LedCtrl_STYP oLed;
extern NFC_STYP oNFC;

//Prototypes
static void Scheduler_writeProductInfo(void);
static void Scheduler_writeInstant(void);
static void Scheduler_writeBroadcast(void);

/*
=======================================================================================
Method name:  Schedule_manageTasks()
					
Originator:   Sun Ran

Description:  Called from main() every 1 mSec. 
			  Calls tasks at:
			   - LEDs
			   - Scheduler_minTimeoutCheck
			   - Scheduler_serviceNFCWrite
			  CHecks to see if there is a flag set for writing data instantly,productInfo
			  or if it is a broadcast. Writes the data and then clears the flag;  
	
Resources:	  

=======================================================================================
History:
*-----*-------------*---------------------------------------------------*--------------
1.00	06-30-2015	Original Code										Sun Ran
1.01	01-18-2017	Adapted from PAM with headers and comments added.   Tom Van Sistine
1.02	12-13-2017	Changed call to flash red LED to use prioritized 	Tom Van Sistine
                    Look Up Table to flash fewer times for common
                    problems.
1.03	08-02-2019  Added call to Scheduler_minTimeoutCheck()			Tom Van Sistine
1.04	08-28-2020  Added call to schedule broadcast block write	 Anish Venkataraman
1.05	01-08-2021  Added code to save data only when permitted by	 Anish Venkataraman
					TRC
---------------------------------------------------------------------------------------
*/
void Scheduler_manageTasks(void)
{
	uint8_t flashCode = 0;
	LedCtrl_ledsManager(&oLed,flashCode);
	Scheduler_minTimeoutCheck();
	//only write if the permission to save has been initiated by TRC
	//if(oMinSlave.slaveRegisters[MIN_SLAVE_PERMIT_TO_SAVE_DATA_RNUM] == INITIALIZED)
	{
			if(oNFC.instantWriteFLG == TRUE){//schedule to write block instantly
				Scheduler_writeInstant();
				oNFC.instantWriteFLG = FALSE;
			}
			else if (oNFC.productInfoFLG == TRUE){//schedule to write productInfo block
				Scheduler_writeProductInfo();
				oNFC.productInfoFLG = FALSE;
			}
			else if(oNFC.storeBroadcastFLG == TRUE){//schedule to write block broadcast
				Scheduler_writeBroadcast();
				if(oNFC.broadcast.blockNumber == BLOCK2 && oNFC.block2WriteFLG == TRUE){
					oNFC.block2WriteFLG = FALSE;//clear block2 broadcast flag
				}
				else if(oNFC.broadcast.blockNumber == BLOCK11 && oNFC.block11WriteFLG == TRUE){
					oNFC.block11WriteFLG = FALSE;//clear block11 broadcast flag
				}
				else if(oNFC.broadcast.blockNumber == BLOCK226 && oNFC.block226WriteFLG == TRUE){
					oNFC.block226WriteFLG = FALSE;//clear block226 broadcast flag
				}
				else{//clear flags
					oNFC.block2WriteFLG = FALSE;
					oNFC.block11WriteFLG = FALSE;
					oNFC.block226WriteFLG = FALSE;
				}
				oNFC.storeBroadcastFLG = FALSE;
			}
	}
} 
/*
 ========================================================================================
 Method name:  Scheduler_serviceNFCWrite()

 Originator:   Anish Venkataraman

 Description:
 	 	 This routine is called every minute  and checks if there flag for  block is set or 
		 not. If it is set, it writes the data starting at an offset. Once all data is
		 written the flag is cleared.

 Resources:

 ========================================================================================
 History:
 *-------*-----------*---------------------------------------------------*---------------
 1.00    11-04-2019  Original code                                     Anish Venkataraman
 1.01	 11-21-2019  Modified the code to check for errors			   Anish Venkataraman
 1.02	 06-15-2020  Modified the code to support writing of		   Anish Venkataraman
					 block 11 & 226			  
 ----------------------------------------------------------------------------------------
*/

void Scheduler_serviceNFCWrite(void) {
	static uint8_t secondCounter = 1;
	 if (secondCounter == SCHEDULER_BLK2_WRITE) {
		 oNFC.block2WriteFLG = TRUE;
	 }
	 else if (secondCounter == SCHEDULER_BLK11_WRITE) {
		  oNFC.block11WriteFLG = TRUE;
	 }
	else if (secondCounter == SCHEDULER_BLK226_WRITE) {
		 oNFC.block226WriteFLG = TRUE;
		secondCounter = 1;
	 }
	 secondCounter++;
}


/*
 ========================================================================================
 Method name:  Scheduler_minTimeoutCheck()

 Originator:   Tom Van Sistine

 Description:
 	 	 If MIN timeout timer not zero, decrement.  If then 0 call reset uart to discover
 	 	 baud rate and reset.

 Resources:

 ========================================================================================
 History:
 *-------*-----------*---------------------------------------------------*---------------
 1.00    08-02-2019  Original code                                       Tom Van Sistine
 ----------------------------------------------------------------------------------------
 */

void Scheduler_minTimeoutCheck(void){
	if(oMinSlave.communicationTimeoutCNTR){
		oMinSlave.communicationTimeoutCNTR--;
		if(oMinSlave.communicationTimeoutCNTR == 0) {
			oMinSlave.uart.baudSelect = BR_SELECT_115200;  // Set back to Discovery baud
			MinSlave_init(&oMinSlave);
		}
	}
}

/*
 ========================================================================================
 Method name:  Scheduler_writeProductInfo()

 Originator:   Anish Venkataraman

 Description:
 	 	 This routine is called every 1ms  and checks if there flag for productinfo is 
		 set or not 
		

 Resources:

 ========================================================================================
 History:
 *-------*-----------*---------------------------------------------------*---------------
 1.00    08-28-2020  Original code                                     Anish Venkataraman	
 1.01    05-10-2021  Restructured code to use NFC_STYP class members   Anish Venkataraman	  
 ----------------------------------------------------------------------------------------
*/
void Scheduler_writeProductInfo(void){
	uint16_t i = 0;
	uint8_t count = 0;
	uint16_t address = 0;
	uint8_t length = 0;
	uint8_t data[BYTES_IN_PAGE];
	uint8_t j = 0;
	uint8_t index;
	address = BLOCK_NVM_OFFSET;
	length = oNFC.productInfoLength;
	index = 0;
	for(i = address;i < address+length;){
		//read data
		NFC_SequentialRead(i,BYTES_IN_PAGE,data);
		for(j = 0; j < BYTES_IN_PAGE; j++){
			//check data if it matches
			if(data[j] == oNFC.productInfoData[index]){
				count++;
			}
			data[j] = oNFC.productInfoData[index++];
		}
		if(count != BYTES_IN_PAGE){
			NFC_PageWrite(i,data);
			oNFC.schedulerNFCWriteWait = TRUE;
			//wait for 5ms before next read
			while(oNFC.schedulerNFCWriteWait == TRUE);
		}
		j = 0;
		count = 0;
		i = i + BYTES_IN_PAGE;
	}
}

/*
 ========================================================================================
 Method name:  Scheduler_writeInstant()

 Originator:   Anish Venkataraman

 Description:
 	 	 This routine is called every 1ms  and checks if there flag for instant is set or 
		 not. 
 Resources:

 ========================================================================================
 History:
 *-------*-----------*---------------------------------------------------*---------------
 1.00    08-28-2020  Original code                                     Anish Venkataraman	  
 ----------------------------------------------------------------------------------------
*/
void Scheduler_writeInstant(void){
	uint16_t i = 0;
	uint8_t count = 0;
	uint16_t address = 0;
	uint8_t length = 0;
	uint8_t data[BYTES_IN_PAGE];
	uint8_t j = 0;
	uint8_t index;
	//get address and length of bytes to write
	address = NFC_getUpdateBlkAddress(&oNFC);
	length = oNFC.update.length;
	index = 0;
	for(i = address;i < address+length;){
		NFC_SequentialRead(i,BYTES_IN_PAGE,data);
		for(j = 0; j < BYTES_IN_PAGE; j++){
			if(data[j] == oNFC.update.nfcBuffer[index]){
				count++;
			}
			data[j] = oNFC.update.nfcBuffer[index++];
		}
		if(count != BYTES_IN_PAGE){
			NFC_PageWrite(i,data);
			oNFC.schedulerNFCWriteWait = TRUE;
			//wait for 5ms before next read
			while(oNFC.schedulerNFCWriteWait == TRUE);
		}
		j = 0;
		count = 0;
		i = i + BYTES_IN_PAGE;
	}
}

/*
 ========================================================================================
 Method name:  Scheduler_writeBroadcast()

 Originator:   Anish Venkataraman

 Description:
 	 	 This routine is called every 1ms  and checks if there flag for broadcast is set or 
		 not.  Once all data is written the flag is cleared.

 Resources:

 ========================================================================================
 History:
 *-------*-----------*---------------------------------------------------*---------------
 1.00    08-28-2020  Original code                                     Anish Venkataraman	  
 ----------------------------------------------------------------------------------------
*/
void Scheduler_writeBroadcast(void){
	uint16_t i = 0;
	uint8_t count = 0;
	uint16_t address = 0;
	uint8_t length = 0;
	uint8_t data[BYTES_IN_PAGE];
	uint8_t j = 0;
	uint8_t index;
	//get address and length of bytes to write
	address = NFC_getBroadcastBlkAddress(&oNFC);
	length = oNFC.broadcast.length;
	index = 0;
	for(i = address;i < address+length;){
		//Read data before writing
		NFC_SequentialRead(i,BYTES_IN_PAGE,data);
		for(j = 0; j < BYTES_IN_PAGE; j++){
			if(data[j] == oNFC.broadcast.nfcBuffer[index]){
				count++;
			}
			data[j] = oNFC.broadcast.nfcBuffer[index++];
		}
		if(count != BYTES_IN_PAGE){//data is different so write the page
			NFC_PageWrite(i,data);
			oNFC.schedulerNFCWriteWait = TRUE;
			//wait for 5ms before next read
			while(oNFC.schedulerNFCWriteWait == TRUE);
		}
		j = 0;
		count = 0;
		i = i + BYTES_IN_PAGE;
	}
}
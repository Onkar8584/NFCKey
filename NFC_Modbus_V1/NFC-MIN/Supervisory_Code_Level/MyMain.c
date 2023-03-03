/*
=======================================================================================
File name:    MyMain.c                  
                    
Originator:   Tom Van Sistine



=======================================================================================
 History:	(Identify methods that changed)
*-------*-----------*---------------------------------------------------*--------------
1.00	11-04-2019	Initial code.									 Anish Venkataraman
1.01	11-04-2019	Adapted from CPAM code							 Anish Venkataraman
1.02	11-05-2019	Changed scheduling from 5msec to 1min			 Anish Venkataraman
1.03	08-12-2020	Added check in MyMain_run() to write to NFC		 Anish Venkataraman
---------------------------------------------------------------------------------------
*/

//Includes
#include "MinSlave.h"
#include "MyMain.h"
#include "Scheduler.h"
#include "LedCtrl.h"
#include "NFC.h"

//------------------ Instantiate all class objects here --------------------------------
MinSlave_STYP oMinSlave = MIN_DEFAULTS;
LedCtrl_STYP oLed;
NFC_STYP oNFC = NFC_DEFAULTS;

// Instantiate itself.
MyMain_STYP mainObject = MYMAIN_DEFAULTS;


/*
=======================================================================================
Method name:  MyMain_systemInit()
					
Originator:   Anish Venkataraman

Description: 
	Initialize all objects.
	
Resources:

=======================================================================================
History:
*-----*-------------*---------------------------------------------------*--------------
1.00	11-04-2019	Original Code										Anish Venkataraman
1.01	11-04-2019	Modified for NFC									Anish Venkataraman

---------------------------------------------------------------------------------------
*/

void MyMain_systemInit(void){
	LedCtrl_init(&oLed);
	MinSlave_init(&oMinSlave);
	NFC_init(&oNFC);
}
/*
=======================================================================================
Method name:  MyMain_main()
					
Originator:   Anish Venkataraman

Description: 
	Main loop code called from the function main() but does not return.
	Rather it does a normal main() function without the auto-generated code 
	
	MyMain() looks therefore like a standard Main.c file and is more portable show we
	go away from a PE project.
	
	It monitors the 1 msec timer flag set in driver_isr.c and calls Scheduler_run().  
	
Resources:

=======================================================================================
History:
*-----*-------------*---------------------------------------------------*--------------
1.00	01-13-2017	Original code									 Anish Venkataraman
1.01    11-04-2019	Added a 5ms check flag to service NFC write		 Anish Venkataraman
					operation
1.02	11-05-2019	Changed scheduling from 5msec to 1min			 Anish Venkataraman
1.03	11-21-2019	Changed scheduling from 1m to 10min and			 Anish Venkataraman
					added a check for every completion of NFC write.
1.04	08-12-2020	Check to see if the NFC is permitted to save the Anish Venkataraman
					data
---------------------------------------------------------------------------------------
*/
void MyMain_main(void){
	
	//Initialize Objects
	MyMain_systemInit();
	while(1)
	{
		//1ms interrupt check
		if(mainObject.realTimeInterruptFlag == TRUE){			// Set every 1 msec in driver_isr.c
			mainObject.realTimeInterruptFlag = FALSE;
			Scheduler_manageTasks();
		}
		//1min interrupt check
		if(mainObject.schedulerNFCRunFlag == TRUE){				// Set every 1min in driver_isr.c
			//only write if the permission to save has been initiated by TRC
			//if(oMinSlave.slaveRegisters[MIN_SLAVE_PERMIT_TO_SAVE_DATA_RNUM] == INITIALIZED)
			{
			//Writes to NFC's memory if the there is any data to be written
				Scheduler_serviceNFCWrite();
			}
			mainObject.schedulerNFCRunFlag = FALSE;
		}
	}
}


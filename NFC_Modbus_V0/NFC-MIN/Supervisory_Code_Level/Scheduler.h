/*=====================================================================================
File name:    Scheduler.h                  
                    
Originator:   Tom Van Sistine

Description:	Scheduler is used to call all tasks at the appropriate time.

Multi-Instance: No

Class Methods:
				void Scheduler_serviceNFCWrite(void);// Called from MyMain_main()
				void Scheduler_manageTasks(void);// Called from MyMain_main()
				void Scheduler_minTimeoutCheck(void);// Called from MyMain_main()
Peripheral Resources:
  
IoTranslate requirements:

Other requirements:
  
=======================================================================================
 History:	
*-------*-----------*---------------------------------------------------*--------------
1.00	01-13-2017	New File											Tom Van Sistine
1.01	11-04-2019	Adapted from CPAM and added 					 Anish Venkataraman
					Scheduler_serviceNFCWrite() function.
1.02	11-21-2019	Added a new function Scheduler_checkNFCWrite &	 Anish Venkataraman
					added a macro SCHEDULER_NFC_WAIT_TIME
1.03	06-15-2020	Added macros for blk2 and blk11 write			 Anish Venkataraman
					
---------------------------------------------------------------------------------------
*/

#ifndef SCHEDULER_H_
#define SCHEDULER_H_


#define SCHEDULER_NFC_WRITE_TIME  1//10min 
//wait time for all bytes from block2 multiplied by time per write cycle and a buffer of 50ms
#define SCHEDULER_NFC_WAIT_TIME	  (uint16_t)((B2_SIZE * NFC_WRITE_TIME) + 50) 
#define SCHEDULER_BLK2_WRITE		2//2 min
#define SCHEDULER_BLK11_WRITE		3//3 min
#define SCHEDULER_BLK226_WRITE		4//4 min
#define TIME_6_MSEC					6//6msec

//Public Methods
void Scheduler_serviceNFCWrite(void);
void Scheduler_manageTasks(void);
void Scheduler_minTimeoutCheck(void);
void Scheduler_checkNFCWrite(void);
#endif /* SCHEDULER_H_ */
/*=====================================================================================
File name:    MyMain.h                  
                    
Originator:   Tom Van Sistine

Description:	Substitute for Main.c as IDE generates its own.  That will just call
				this MyMain() were it will loop forever and do regular main() stuff.
				Calls scheduler.

Multi-Instance: No

Class Methods:
				void MyMain_main(void);  // Called from main()

Peripheral Resources:
  
IoTranslate requirements:

Other requirements:
  
=======================================================================================
 History:	
*-------*-----------*---------------------------------------------------*--------------
1.00	01-13-2017	Initial code.										Tom Van Sistine
1.01	11-04-2019	Adapted from CPAM code							 Anish Venkataraman
1.02	11-05-2019	Changed scheduling from 5msec to 1min			 Anish Venkataraman
1.03	11-21-2019	Changed scheduling from 1m to 10min and			 Anish Venkataraman
					updated class structure.
---------------------------------------------------------------------------------------
*/


#ifndef MYMAIN_H_
#define MYMAIN_H_

//Includes

//Class Structure
typedef struct {
	//Public variables
	bool realTimeInterruptFlag;		//Set every 1ms
	bool schedulerNFCRunFlag;		//Set every 10mins
}MyMain_STYP;

#define MYMAIN_DEFAULTS { FALSE, FALSE}
//Public Class Method prototypes
void MyMain_main(void);


#endif /* MYMAIN_H_ */
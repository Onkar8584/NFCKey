/*
=====================================================================================
File name:    LedCtrl.c                 
                    
Originator:   Sun Ran

=====================================================================================
 History:	(Indentify methods that changed)
-*-----*-----------*--------------------------------------------------*--------------
1.00	08-16-2015	Original Code										Sun Ran
1.01	11-04-2019	Modified as per project specification			Anish Venkataraman
1.02	11-21-2019	Modified parameter type to						Anish Venkataraman
					LedCtrl_HeartBeatSetInterval                   
-------------------------------------------------------------------------------------
*/
//------------------ Includes ---------------------------------------------------------
#include "Build.h"
#include "LedCtrl.h"


/*
=======================================================================================
Method name:  LedCtrl_init()
					
Originator:   Sun Ran

Description:  Initialize scheduler object
	
Resources:	  

=======================================================================================
History:
*-----*-------------*---------------------------------------------------*--------------
1.00	08-16-2015	Original Code										Sun Ran
1.01	01-18-2017	Adapted from PAM with headers and comments added.   Tom Van Sistine
1.02	11-04-2019	Removed components that were not needed for this	Anish Venkataraman	
					project
---------------------------------------------------------------------------------------
*/
void LedCtrl_init(LedCtrl_STYP *lc)
{
	lc->HeartBeatBlinkInterval = BLINK_NORMAL;
	
}
/*
=======================================================================================
Method name:  LedCtrl_HeartBeatBlink()
					
Originator:   Sun Ran

Description:  Blinks the Led at the interval specified.
	
Resources:	  

=======================================================================================
History:
*-----*-------------*---------------------------------------------------*--------------
1.00	08-16-2015	Original Code										Sun Ran
1.01	01-18-2017	Adapted from PAM with headers and comments added.   Tom Van Sistine
1.02	11-04-2019	Edited the description								Anish Venkataraman				
---------------------------------------------------------------------------------------
*/
void LedCtrl_HeartBeatBlink(LedCtrl_STYP *lc)
{
	if(lc->HeartBeatCounter > 0)
	{
		lc->HeartBeatCounter--;
	}
	else
	{
		lc->HeartBeatCounter = lc->HeartBeatBlinkInterval;
		HeartBeatLED_Blink();
	}
}

/*
=====================================================================================
Method name:    LedCtrl_HeartBeatSetInterval
Originator:   Sun Ran

Description: set the LED blink speed

=====================================================================================
History:
*-----*-------------*---------------------------------------------------*--------------
1.00	08-16-2015	Original Code										Sun Ran
1.01	11-04-2019	Modified parameter type to	uint16_t from		Anish Venkataraman
					uint8_t
---------------------------------------------------------------------------------------
*/
void LedCtrl_heartBeatSetInterval(LedCtrl_STYP *lc,uint16_t interval)
{
	lc->HeartBeatBlinkInterval = interval;
}

/*
=====================================================================================
Method name:    LedCtrl_LedsManager
Originator:   Sun Ran

Description: LED blinking

=====================================================================================
*/
void LedCtrl_ledsManager(LedCtrl_STYP *lc,uint8_t errorCode)
{
	LedCtrl_HeartBeatBlink(lc);
}
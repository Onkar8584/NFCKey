/*
 * LedCtrl.h
 *
 *  Created on: Aug 16, 2015
 *      Author: Sun Ran
 */
/*
=====================================================================================
File name:    LedCtrl.h

Originator:   Sun Ran

Description:
 	 Controls a heartbeat LED flashing at specified rate and two other LEDs red and
 	 green. The Red LED flashes fault codes and the Green is on (no fault) or off
 	 (fault condition).

Public functions
	LedCtrl_init(LedCtrl_STYP *lc);
	LedCtrl_heartBeatSetInterval(LedCtrl_STYP *lc,uint8_t interval);
	LedCtrl_ledsManager(LedCtrl_STYP *lc,uint8_t errorCode);

Resources:

IoTranslate requirements:

Private methods called

=====================================================================================
 History:
-*-----*-----------*--------------------------------------------------*--------------
 1.00  08-16-2015  New												  Sun Ran
 1.01  02-09-2017  Added proper header per Software standard.         Tom Van Sistine
     	 	 	   Moved #include Build.h to LedCtrl.c.
 -------------------------------------------------------------------------------------
*/

#ifndef LEDCTRL_H_
#define LEDCTRL_H_


#define BLINK_NORMAL	300
#define BLINK_ERROR		50

typedef struct {
	
	uint16_t HeartBeatCounter;
	uint16_t HeartBeatBlinkInterval;
}LedCtrl_STYP;


// Public functions
extern void LedCtrl_init(LedCtrl_STYP *lc);
extern void LedCtrl_heartBeatSetInterval(LedCtrl_STYP *lc,uint16_t interval);
extern void LedCtrl_ledsManager(LedCtrl_STYP *lc,uint8_t errorCode);
extern void LedCtrl_HeartBeatBlink(LedCtrl_STYP *lc);



#endif /* LEDCTRL_H_ */
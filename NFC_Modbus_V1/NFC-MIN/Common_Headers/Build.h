/*=====================================================================================
File name:    Build.h                  
                    
Originator:   Sun Ran

Description:  Hardware information

Multi-Instance: (not applicable)

Class Methods:

Peripheral Resources:
  
IoTranslate requirements:

Other requirements:
  
=======================================================================================
 History:	
*-------*-----------*---------------------------------------------------*--------------
1.00	06-25-2015  New File											Sun Ran
1.01	01-13-2017  Adapted from PAM for CPAM							Tom Van Sistine
1.02	02-07-2017	Added #define AIN_SLAVE_ADDRESS						Tom Van Sistine
1.03    02-21-2017  Moved to integrated CPAM, define ADC channels 0 & 1.Tom Van Sistine
1.04    04-08-2017  #define AIN_LAST_FLEX_ADDRESS 						Tom Van Sistine
1.05    04-17-2019  Voltage_VTYP, Current_VTYP and Adc10R_VTYP changed  Tom Van Sistine
                    from int to uint16_t.  int on CCB was 16 bits and
                    int on KE02 is 32 bits.
1.06	08-05-2019  Moved #include "PE_Types.h" to IoTranslate.h for    Tom Van Sistine
                    allowing to compile with TDD.
                    Added #include "stdint.h" and removed specially
                    defining int8_t and uint8_t. Added #define FALSE
                    and TRUE.
1.07	08-29-2019  Added NFC_BASE_ADDRESS as 2 and changed the         Anish Venkataraman
                    corresponding NFC_SLAVE_ADDRESS.
---------------------------------------------------------------------------------------
*/

#ifndef BUILD_H_
#define BUILD_H_
#include "stdint.h"

typedef uint16_t  Voltage_VTYP;         // Volts	MSByte integer, LSByte fraction
typedef uint16_t   Current_VTYP;        // MilliAmps	mA x 16 (not like volts as CCB, because CPAM current can go above 255.99ma.)
typedef uint16_t   Adc10R_VTYP;         // Counts	10 bit right justified
typedef unsigned char byte;

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#include "IoTranslate.h"
		
#define SCHEDULER_RUN_50MSEC_TASKS		50u 	
#define SCHEDULER_MANAGE_50MSEC_TICK	1
#define SCHEDULER_TICKS_PER_SECOND		20
#define ONE_SECOND_TIME					1000U//SCHEDULER_TICKS_PER_SECOND
#define FIVE_SECOND_TIME				(5 * ONE_SECOND_TIME)
#define THIRTY_SECONDS 					(30 * ONE_SECOND_TIME)
#define ONE_MINUTE_TIME					(60 * ONE_SECOND_TIME)
#define NFC_BASE_ADDRESS 2   // NFC Base Address

// MIN protocol enum as defined in Modbus Internal Network protocol.docx
enum {
	BR_SELECT_19200 = 1,
	BR_SELECT_38400,
	BR_SELECT_57600,
	BR_SELECT_115200,
};


//#define VREF  5U  // Used by IoTranslate.h and powered anode.

// ----- Support MIN Slave ---------------------------------------------------------
#define NFC_SLAVE_ADDRESS NFC_BASE_ADDRESS

#define CURRENT_HW_VERSION 1
#define CURRENT_HW_REVISION 0
#define CURRENT_HW_VERSION_REVISION ((CURRENT_HW_VERSION << 8) | CURRENT_HW_REVISION)

#define MAX_BAUD_RATE_SUPPORTED  BR_SELECT_115200

// ---------------------------------------------------------------------------------



// Global suppress of annoying enum message.
//lint -e641 -e655 allow bit-wise operators on compatible enums
//lint -e656 allow arithmetic operation on compatible enums
#endif /* BUILD_H_ */

/*
=========================================================================================
File name:		IoTranslate.c                  

Originator:		Anish Venkataraman

=========================================================================================
 History:	(Identify methods that changed)
 *-------*-----------*---------------------------------------------------*----------------
1.00	11-04-2019	New File										   Anish Venkataraman
1.01	11-04-2019	Added I2C and UART functions		 			   Anish Venkataraman
1.02	12-09-2019	Removed change of baud rate module	 			   Anish Venkataraman
-----------------------------------------------------------------------------------------
 */

//------------------ Includes -----------------------------------------------------------
#include "stdint.h"
#include "IoTranslate.h"
#include "atmel_start_pins.h"
#include "string.h"

//Global Variables
uint8_t rxChars = 0;
uint8_t rxBuffer[RX_BUFFER_LENGTH] = {0};
uint8_t rxIndex = 0;
uint8_t tempIndex = 0;

/*
 ========================================================================================
 Method name:  SDA_Write()

 Originator:   Anish Venkataraman

 Description:
 	 	 This routine sends bit value 1 or 0 depending on the value to be sent 

 Resources:

 ========================================================================================
 History:
 *-------*-----------*---------------------------------------------------*---------------
 1.00    11-04-2019  Original code                                     Anish Venkataraman

 ----------------------------------------------------------------------------------------
 */
void SDA_Write(unsigned char val){
	if(val)	{
		SDA_set_level(TRUE);
	}
	else {
		SDA_set_level(FALSE);
	}
}
/*
 ========================================================================================
 Method name:  SDA_Read()

 Originator:   Anish Venkataraman

 Description:
 	 	 This routine returns the value read on the SDA line. 

 Resources:

 ========================================================================================
 History:
 *-------*-----------*---------------------------------------------------*---------------
 1.00    11-04-2019  Original code                                     Anish Venkataraman

 ----------------------------------------------------------------------------------------
 */
uint8_t SDA_Read(void){
	return SDA_get_level();
}

/*
 ========================================================================================
 Method name:  Hardware_Delay()

 Originator:   Anish Venkataraman

 Description:
 	 	 This routine provides delay.

 Resources:

 ========================================================================================
 History:
 *-------*-----------*---------------------------------------------------*---------------
 1.00    11-04-2019  Original code                                     Anish Venkataraman
  ----------------------------------------------------------------------------------------
 */

void Hardware_Delay(int val) {
	for(int i = 0; i < val; i++){
		asm("NOP");		
	} 
}

/*
 ========================================================================================
 Method name:  USART_RxChar()

 Originator:   Anish Venkataraman

 Description:
 	 	 This routine stores the value from the receive buffer and returns ERR or NO_ERR
		 if there are no cahrachters inside receive buffer.  

 Resources:

 ========================================================================================
 History:
 *-------*-----------*---------------------------------------------------*---------------
 1.00    11-04-2019  Original code                                     Anish Venkataraman

 ----------------------------------------------------------------------------------------
 */
uint8_t USART_RxChar(byte *val) {

	if(rxChars > 0) {
		rxChars -= 1;
		*val = rxBuffer[tempIndex];
		tempIndex++;
	}
	else {
		*val = 0;
		tempIndex = 0;
		rxIndex = 0;
		memset(rxBuffer, 0, RX_BUFFER_LENGTH);
		return ERR;
	}

	return NO_ERR;
}


/*
 ========================================================================================
 Method name:  USART_GetCharsInRxBuf()

 Originator:   Anish Venkataraman

 Description:
 	 	 This routine returns the number of characters in receive buffer. 

 Resources:

 ========================================================================================
 History:
 *-------*-----------*---------------------------------------------------*---------------
 1.00    11-04-2019  Original code                                     Anish Venkataraman

 ----------------------------------------------------------------------------------------
 */

uint8_t USART_GetCharsInRxBuf(void){
	return rxChars;
}

/*
 ========================================================================================
 Method name:  USART_StoreData()

 Originator:   Anish Venkataraman

 Description:
 	 	 This routine stores the data received on the Rx line into a buffer.

 Resources:

 ========================================================================================
 History:
 *-------*-----------*---------------------------------------------------*---------------
 1.00    11-04-2019  Original code                                     Anish Venkataraman

 ----------------------------------------------------------------------------------------
 */

void USART_StoreData(void) {
	if(USART0.STATUS & USART_RXCIF_bm) { //checks if the interrupt flag is set
		byte data = USART0.RXDATAL;		//stores data 
		USART0.STATUS = USART_RXCIF_bm;	//clears the the receive interrupt status bit
		if(rxIndex <= RX_BUFFER_LENGTH){
			rxBuffer[rxIndex++] = data;
			rxChars += 1;					//increments the receive charachter
		}
		else{
			rxIndex = 0;
		}
	}
}

/*
 ========================================================================================
 Method name:  USART_ClearRxBuffer()

 Originator:   Anish Venkataraman

 Description:
 	 	 This routine clears the receive buffer and all other component associated with it.

 Resources:

 ========================================================================================
 History:
 *-------*-----------*---------------------------------------------------*---------------
 1.00    11-04-2019  Original code                                     Anish Venkataraman

 ----------------------------------------------------------------------------------------
 */
void USART_ClearRxBuffer(void) {
	memset(rxBuffer, 0, RX_BUFFER_LENGTH);
	rxChars = 0;
	rxIndex = 0;
	tempIndex = 0;
}

/*
 ========================================================================================
 Method name:  USART_SendChar()

 Originator:   Anish Venkataraman

 Description:
 	 	 This routine writes a character on the Tx Line. This is a non-blocking routine
		 until the Data Register Empty Flag is set.

 Resources:

 ========================================================================================
 History:
 *-------*-----------*---------------------------------------------------*---------------
 1.00    11-04-2019  Original code                                     Anish Venkataraman

 ----------------------------------------------------------------------------------------
 */
void USART_SendChar(byte *str) {
	while (!(USART0.STATUS & USART_DREIF_bm));
	USART0.TXDATAL = *str;
}

/*
 ========================================================================================
 Method name:  USART_SetTxInterrupt()

 Originator:   Anish Venkataraman

 Description:
 	 	 This routine writes a character on the Tx Line. This is a non-blocking routine
		 until the Data Register Empty Flag is set.

 Resources:

 ========================================================================================
 History:
 *-------*-----------*---------------------------------------------------*---------------
 1.00    11-04-2019  Original code                                     Anish Venkataraman

 ----------------------------------------------------------------------------------------
 */
void USART_SetTxInterrupt(void) {
	USART0.STATUS |= (1<<USART_TXCIE_bp); 
}



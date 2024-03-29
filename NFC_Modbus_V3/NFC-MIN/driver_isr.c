/**
 * \file
 *
 * \brief Driver ISR.
 *
 (c) 2018 Microchip Technology Inc. and its subsidiaries.

    Subject to your compliance with these terms,you may use this software and
    any derivatives exclusively with Microchip products.It is your responsibility
    to comply with third party license terms applicable to your use of third party
    software (including open source software) that may accompany Microchip software.

    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
    WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
    PARTICULAR PURPOSE.

    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
    BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
    FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
    ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
    THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 */

/*
 * Code generated by START.
 *
 * This file will be overwritten when reconfiguring your START project.
 * Please copy examples or other code you want to keep to a separate file
 * to avoid losing it when reconfiguring.
 */

#include <driver_init.h>
#include <compiler.h>
#include "Scheduler.h"
#include "MinSlave.h"
#include "MyMain.h"
#include "NFC.h"


//Global variables
extern MinSlave_STYP oMinSlave;
extern MyMain_STYP mainObject;
extern NFC_STYP oNFC;


ISR(USART0_RXC_vect)
{
	
	/**
	 * Insert your USART_0 reception complete interrupt handling code
	 *
	 * The interrupt flag will be cleared when the receive buffer is empty
	 * Otherwise interrupt flag can be cleared by writing 1 to its bit location
	 * in the STATUS register
	 */
	USART_StoreData();
	MinSlave_manageMessages();
	USART_ClearRxBuffer();
}


ISR(USART0_TXC_vect)
{

	/**
	 * Insert your USART_0 transmission complete interrupt handling code
	 *
	 * The interrupt flag will be automatically cleared
	 */
	
	if(USART0.STATUS & USART_TXCIF_bm)
	{
		MinUart_serviceTx(&oMinSlave.uart);
		USART_ClearRxBuffer();
	}
	USART0.STATUS |= (0<<USART_TXCIE_bp);  //Clear the transmission complete status bit
}

ISR(USART0_DRE_vect)
{

}

ISR(TCA0_OVF_vect)
{
	//1msec interrupt
	static unsigned int schedulerRunCounter = 1;
	static unsigned int schedulerTimeCounter = 0;
	static uint16_t schedulerWaitCounter = 0;
	mainObject.realTimeInterruptFlag = TRUE;
	
	if(SCHEDULER_NFC_WRITE_TIME <= schedulerTimeCounter)
	{
		schedulerTimeCounter = 0;
		mainObject.schedulerNFCRunFlag = TRUE;
	}
	else if(schedulerRunCounter >= ONE_MINUTE_TIME)
	{
		schedulerRunCounter = 0;
		schedulerTimeCounter++;
	}
	else
	{
		schedulerRunCounter++;
	}
	//Wait counter for NFC wait for write
	if(oNFC.schedulerNFCWriteWait == TRUE) {
		schedulerWaitCounter++;
		//if(schedulerWaitCounter >= TIME_6_MSEC) {
		if(schedulerWaitCounter >= TIME_10_MSEC) {
			oNFC.schedulerNFCWriteWait = FALSE;
			schedulerWaitCounter = 0;
		}
	}
	
	if (oMinSlave.uart.modbusReSync) 
	{	// Count for no MIN UART transmission lately
		oMinSlave.uart.modbusReSync--;
	}

	/* The interrupt flag has to be cleared manually */
	TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;
}
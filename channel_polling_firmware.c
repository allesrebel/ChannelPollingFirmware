/*
 * Senior Project - A simple communication card between electrodes and PC
 *
 * RTMS Polling firmware for MSP430G2553
 *
 *	Designed to continously poll 2 16:1 muxes
 *	Relays information from conditioned signal to UART connected
 *	device. PC is expected.
 *
 *
 * Initial Revision 4/23/2015 by Alles Rebel (Team RTMS)
 * 	4/21/15 - Added in Stablizing Delays (AR)
 * 	5/12/15 - Changed from 8:1 muxes to 16:1 muxes (AR)
 * 	5/18/15	- Fixed Issue with ISR (AR)
 */


#include <msp430.h> 
#include <stdio.h>

/*
 * Port Definitions
 */
//	UART Port Definitions
#define	uartPSel1	P1SEL
#define uartPSel2	P1SEL2
#define uartRxPort	BIT1
#define uartTxPort	BIT2

//	UART is on UCA0!
#define	uartCtrl1	UCA0CTL1
#define uartBRate0	UCA0BR0
#define	uartBRate1	UCA0BR1
#define clkDiv9600	1666
#define uartModCtrl	UCA0MCTL
#define uartTxBuf	UCA0TXBUF

//	ADC Port Definitions
#define	adcCtrl0	ADC10CTL0
#define	adcCtrl1	ADC10CTL1
#define	adcCh		INCH_0
#define adcChEn		BIT0
//Add additional Ports and channels as needed
#define	adcAEReg	ADC10AE0
#define	adcDTC0Reg	ADC10DTC0
#define	adcDTC1Reg	ADC10DTC1
#define	adcStrtAddr	ADC10SA

/*
 * Functions
 */
void setupClock();

/*
 * UART Functions
 */
void uartSetup();
void uartSend(char);
void uartSendStr(char*);

/*
 * UART Variables
 */
char test[30];
#define bufferSize 20
char buffer[bufferSize];	//Buffer for holding commands
int bufferIndex = 0;		//Current Index of Buffer

/*
 * ADC Functions
 */
void adcSetup();
void adcStart();

/*
 * Global Variables & functions
 */
#define TOTALCHANNELS 32
unsigned int chNumber = 0;
void initCh();
void nextCh();

/*
 * Initial Channel
 */
void initCh(){
	P2DIR |= BIT0 + BIT1 + BIT2 + BIT3;	//Set P2 to output
	P2OUT = chNumber;
}

/*
 * Next Select Source
 */
void nextCh(){
	chNumber++;
	if(chNumber >= TOTALCHANNELS)
		chNumber = 0;
	P2OUT = chNumber;

	if(P2OUT & BIT4)
		P2OUT &= ~BIT5;
	else
		P2OUT |= BIT5;
}

/*
 * main.c
 */
int main(void) {
	WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

	//set up phase!
	setupClock();
	adcSetup();
	uartSetup();

	_enable_interrupts();

	adcStart();

	P1DIR |= BIT6;
	P2DIR |= 0xFF;	// Set to all outputs

	while (1) {
		if(!(P1OUT & BIT6)){
			// Send the Information to Computer
			sprintf(test,"Ch %d ADC Val: %d\n", chNumber, ADC10MEM);
			uartSendStr(test);

			// Prep the Data for the Next Sample
			nextCh();
			__delay_cycles(6790000); // Wait for the cap on the RMS chip to charge
			adcStart();				// Take the Sample
			__delay_cycles(1000);	// Wait for ADC To Grab the data + send to PC
		}
		else{
			//	Error Occured: Shouldn't ever get here...
			uartSendStr("Error!\n");
		}
	}
}

void uartSendStr(char* data) {
	int i = 0;
	while (data[i] != '\0') {
		uartSend(data[i]);
		i++;
	}
}

void uartSend(char data) {
	while (!(IFG2 & UCA0TXIFG))
		; // Wait for TX buffer to be ready for new data
	uartTxBuf = data;
}

void adcStart() {
	P1OUT |= BIT6;			// Indiate when a Sample is taken
	adcCtrl0 |= ENC + ADC10SC;	// Sampling and conversion start
}

void adcSetup() {
	adcCtrl1 = CONSEQ_0 + adcCh;	// single channel. single conversion, A1
	adcCtrl0 = ADC10SHT_2 + ADC10ON + ADC10IE;// ADC10ON, interrupt enable
	adcAEReg |= adcChEn;	// Select the ports desired
}

void uartSetup() {
	/* Configure hardware UART */
	uartPSel1 |= uartRxPort + uartTxPort;	// P1.1 = RXD, P1.2=TXD
	uartPSel2 |= uartRxPort + uartTxPort;	// P1.1 = RXD, P1.2=TXD
	uartCtrl1 |= UCSSEL_2;					// Use SMCLK as src clk
	uartBRate0 = (clkDiv9600 & 0xFF);// Set baud rate to 9600 with 16MHz clock
	uartBRate1 = (clkDiv9600 >> 8);	// Set baud rate to 9600 with 16MHz clock
	uartModCtrl = UCBRS0;		// Modulation UCBRSx = 1
	uartCtrl1 &= ~UCSWRST;	// Initialize USCI state machine
	IE2 |= UCA0RXIE;		// Enable USCI_A0 RX interrupt
}

#pragma vector=USCIAB0RX_VECTOR
__interrupt void UARTRx_ISR(void) {
	//process current char
	buffer[bufferIndex] = UCA0RXBUF;	//this should clear the flag
	bufferIndex++;
}

//ADC10 interrupt Service Routine
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR(void) {
	ADC10CTL0 &= ~ADC10IFG;  // clear interrupt flag
	P1OUT &= ~BIT6;	// Indicate Completion of a Sample
}

void setupClock() {
	WDTCTL = WDTPW + WDTHOLD;	// Stop WDT
	if (CALBC1_16MHZ == 0xFF)	// If calibration constant erased
			{
		while (1)
			;	// do not load, trap CPU!!
	}
	DCOCTL = 0;	// Select lowest DCOx and MODx settings
	BCSCTL1 = CALBC1_16MHZ;	// Set DCO
	DCOCTL = CALDCO_16MHZ;
}

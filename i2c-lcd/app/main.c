/*
 * EELE 465, Project 5
 * Gabby and Iker
 *
 * Target device: MSP430FR2310 Slave
 */

//----------------------------------------------------------------------
// Headers
//----------------------------------------------------------------------
#include "intrinsics.h"
#include <msp430.h>
#include <stdbool.h>
//--End Headers---------------------------------------------------------

//----------------------------------------------------------------------
// Definitions
//----------------------------------------------------------------------
// Puerto 2
#define RS BIT0     // P2.0
#define EN BIT6     // P2.6

// Puerto 1
#define D4  BIT4     // P1.4
#define D5 BIT5     // P1.5
#define D6 BIT6     // P1.6
#define D7 BIT7     // P1.7
#define SLAVE_ADDR  0x48                    // Slave I2C Address
//--End Definitions-----------------------------------------------------

//----------------------------------------------------------------------
// Variables
//----------------------------------------------------------------------
volatile unsigned char receivedData = 0;    // Recieved data
char key_unlocked;
char mode = '\0';
char prev_mode = '\0';
char new_window_size = '\0';
char pattern_cur = '\0';
int length_time = 0;
int length_adc = 0;
bool in_time_mode = false;
bool in_temp_adc_mode = false;
//--End Variables-------------------------------------------------------

//----------------------------------------------------------------------
// Begin I2C Init
//----------------------------------------------------------------------
void I2C_Slave_Init(void)
{
    WDTCTL = WDTPW | WDTHOLD;  // Stop Watchdog Timer

    // Configure P1.2 as SDA and P1.3 as SCL
    P1SEL1 &= ~(BIT2 | BIT3);
    P1SEL0 |= BIT2 | BIT3;

    // Configure USCI_B0 as I2C Slave
    UCB0CTLW0 |= UCSWRST;               // Put eUSCI_B0 into software reset
    UCB0CTLW0 |= UCMODE_3;              // Select I2C slave mode
    UCB0I2COA0 = SLAVE_ADDR + UCOAEN;   // Set and enable first own address
    UCB0CTLW0 |= UCTXACK;               // Send ACKs

    PM5CTL0 &= ~LOCKLPM5;               // Disable low-power inhibit mode

    UCB0CTLW0 &= ~UCSWRST;              // Pull eUSCI_B0 out of software reset
    UCB0IE |= UCSTTIE + UCRXIE;         // Enable Start and RX interrupts

    __enable_interrupt();               // Enable Maskable IRQs
}
//--End I2C Init--------------------------------------------------------

//----------------------------------------------------------------------


//----------------------------------------------------------------------
// Begin Main
//----------------------------------------------------------------------
int main(void) {
    WDTCTL = WDTPW | WDTHOLD;  // Detener el watchdog
    PM5CTL0 &= ~LOCKLPM5;
    lcdInit();  // Inicializar el LCD
    I2C_Slave_Init();                   // Initialize the slave for I2C
    __bis_SR_register(LPM0_bits + GIE); // Enter LPM0, enable interrupts
    return 0;
}
//--End Main------------------------------------------------------------

//----------------------------------------------------------------------
// Begin Interrupt Service Routines
//----------------------------------------------------------------------
// I2C ISR
#pragma vector = USCI_B0_VECTOR
__interrupt void USCI_B0_ISR(void)
{
    switch (__even_in_range(UCB0IV, USCI_I2C_UCTXIFG0))
    {
        case USCI_I2C_UCRXIFG0:         // Receive Interrupt
            display_output(UCB0RXBUF);
            break;
        default: 
            break;
    }
}
//-- End Interrupt Service Routines --------------------------------------------

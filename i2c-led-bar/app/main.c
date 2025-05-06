#include <msp430.h>

#define SLAVE_ADDR 0x48

volatile unsigned char receivedData;

// Set P2.7 as output and drive low
void init_led(void)
{
    P2DIR |= BIT7;
    P2OUT &= ~BIT7;
}

// Initialize I2C in slave mode
void i2c_slave_init(void)
{
    // Configure P1.2 as SDA and P1.3 as SCL
    P1SEL1 &= ~(BIT2 | BIT3);
    P1SEL0 |= BIT2 | BIT3;

    UCB0CTLW0 |= UCSWRST;               // Put eUSCI_B0 into software reset
    UCB0CTLW0 |= UCMODE_3;              // I2C slave mode
    UCB0I2COA0 = SLAVE_ADDR + UCOAEN;   // Set own address, enable
    UCB0CTLW0 |= UCTXACK;               // Auto ACK (optional, but works)

    PM5CTL0 &= ~LOCKLPM5;               // Enable port settings

    UCB0CTLW0 &= ~UCSWRST;              // Exit reset
    UCB0IE |= UCSTTIE + UCRXIE;         // Enable Start + RX interrupts
}

volatile int blink;
void blink_led(char input)
{
    if (input == 'A') {
        P2OUT ^= BIT7;  // Toggle P2.7 instead of just setting
        blink++;
    }
}

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;          // Stop watchdog
    init_led();                        // Init LED
    i2c_slave_init();                  // Init I2C

    __bis_SR_register(LPM0_bits + GIE); // Sleep with interrupts enabled
    return 0;
}

// I2C ISR
#pragma vector = USCI_B0_VECTOR
__interrupt void USCI_B0_ISR(void)
{
    switch (__even_in_range(UCB0IV, USCI_I2C_UCTXIFG0))
    {
        case USCI_I2C_UCRXIFG0:
            receivedData = UCB0RXBUF;
            blink_led(receivedData);    // Call blink function
            break;
        default:
            break;
    }
}

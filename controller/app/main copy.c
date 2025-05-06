/*
 * EELE 465, Final Project
 * Iker
 *
 * Target device: MSP430FR2355 Master
 */

//----------------------------------------------------------------------
// Headers
//----------------------------------------------------------------------
#include <msp430.h>
#include <stdbool.h>
#include <stdio.h>
#include "intrinsics.h"
#include "..\src\master_i2c.h"
#include "..\src\heartbeat.h"
//--End Headers---------------------------------------------------------

//----------------------------------------------------------------------
// Definitions
//----------------------------------------------------------------------
#define PROWDIR     P6DIR  // FORMERLY P1
#define PROWREN     P6REN
#define PROWIN      P6IN
#define PROWOUT     P6OUT
#define PCOLDIR     P5DIR   // FORMERLY P5
#define PCOLOUT     P5OUT
#define RS BIT2  // P1.2 -> RS (Register Select)
#define EN BIT3  // P1.3 -> Enable
#define D4 BIT4  // P1.4 -> Data bit 4
#define D5 BIT5  // P1.5 -> Data bit 5
#define D6 BIT6  // P1.6 -> Data bit 6
#define D7 BIT7  // P1.7 -> Data bit 7
#define COL 4
#define ROW 4
#define TABLE_SIZE 4
//--End Definitions-----------------------------------------------------

//----------------------------------------------------------------------
// Variables
//----------------------------------------------------------------------
 char keypad[ROW][COL] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};
//--End Variables-------------------------------------------------------

//----------------------------------------------------------------------
// Begin Debounce
//----------------------------------------------------------------------
void debounce() {
    volatile unsigned int i;
    for (i = 20000; i > 0; i--) {}
}
//--End Debounce--------------------------------------------------------

//----------------------------------------------------------------------
// Begin Initializing Keypad Ports
//----------------------------------------------------------------------
void keypad_init(void) 
{
    // Set rows as inputs (with pull-up)
    PROWDIR &= ~0x0F;   // P6.0, P6.1, P6.2, P6.3 as inputs
    PROWREN |= 0x0F;    // Activate pull-up
    PROWOUT |= 0x0F;    // Activar pull-up in rows
    
    // Set columns as outputs
    PCOLDIR |= BIT0 | BIT1 | BIT2 | BIT3; // Set P5.0, P5.1, P5.2 y P5.3 as outputs:
    PCOLOUT &= ~(BIT0 | BIT1 | BIT2 | BIT3);  // Set down the pins P5.0, P5.1, P5.2 y P5.3:
}
//--End Initialize Keypad-----------------------------------------------

//----------------------------------------------------------------------
// Begin Unlocking Routine
//----------------------------------------------------------------------
char keypad_read(void)
{
    int row, col;

    // Go through 4 columns
    for (col = 0; col < 4; col++) {
        // Put column down (active)
        PCOLOUT &= ~(1 << col);   // P2.0, P2.1, P2.2, P2.4
         __delay_cycles(1000);  // Little stop to stabilize the signal

        // Go through 4 rows
        for (row = 0; row < 4; row++) {
            if ((PROWIN & (1 << row)) == 0) {  // We detect that the row is low
                debounce();  // Wait to filter the bouncing
                if ((PROWIN & (1 << row)) == 0) {  // Confirm that the key is pressed
                    rgb_led_continue(0);                // Set LED to yellow
                    char key = keypad[row][col];
                    // Wait until the key is released to avoid multiple readings 
                    while ((PROWIN & (1 << row)) == 0);
                    // Deactivate the column before returning
                    PCOLOUT |= (1 << col);                    
                    return key;
                }
            }
        }
        // Put the column high (deactivated)
        PCOLOUT |= (1 << col);
    }

    return 0; // No key pressed
}
//--End Unlocking-------------------------------------------------------

//----------------------------------------------------------------------
// Begin Send Commands
//----------------------------------------------------------------------
void pulseEnable() {
    P2OUT |= EN;             // Establecer Enable en 1
    __delay_cycles(1000);    // Retardo
    P2OUT &= ~EN;            // Establecer Enable en 0
    __delay_cycles(1000);    // Retardo
}

void sendNibble(unsigned char nibble) {
    P1OUT &= ~(D4 | D5 | D6 | D7);  // Limpiar los bits de datos
    P1OUT |= ((nibble & 0x0F) << 4);  // Cargar el nibble en los bits correspondientes (P1.4 a P1.7)
    pulseEnable();  // Pulsar Enable para enviar datos
}

void send_data(unsigned char data) {
    P2OUT |= RS;    // Modo datos
    sendNibble(data >> 4);  // Enviar los 4 bits más significativos
    sendNibble(data & 0x0F);  // Enviar los 4 bits menos significativos (corregido)
    __delay_cycles(4000); // Retardo para procesar los datos
}

void send_command(unsigned char cmd) {
    P2OUT &= ~RS;   // Modo comando
    sendNibble(cmd >> 4);  // Enviar los 4 bits más significativos
    sendNibble(cmd);  // Enviar los 4 bits menos significativos
    __delay_cycles(4000); // Retardo para asegurarse de que el comando se procese
}

void lcdSetCursor(unsigned char position) {
    send_command(0x80 | position);  // Establecer la dirección del cursor en la DDRAM
}
//--End Send Commands---------------------------------------------------

//----------------------------------------------------------------------
// Begine LCD Init
//----------------------------------------------------------------------
void lcdInit() {
    // Configurar pines como salida
    P1DIR |= D4 | D5 | D6 | D7;
    P2DIR |= RS | EN;

    // Limpiar salidas
    P1OUT &= ~(D4 | D5 | D6 | D7);
    P2OUT &= ~(RS | EN);
    __delay_cycles(50000);  // Retardo de inicio
    sendNibble(0x03);  // Inicialización del LCD
    __delay_cycles(5000);  // Retardo
    sendNibble(0x03);  // Repetir la inicialización
    __delay_cycles(200);  // Retardo
    sendNibble(0x03);  // Repetir la inicialización
    sendNibble(0x02);  // Establecer modo de 4 bits

    send_command(0x28);  // Configurar LCD: 4 bits, 2 líneas, 5x8
    send_command(0x0C);  // Encender display, apagar cursor
    send_command(0x06);  // Modo de escritura automática
    send_command(0x01);  // Limpiar la pantalla
    __delay_cycles(2000); // Esperar para limpiar la pantalla
}
//--End LCD Init--------------------------------------------------------

//----------------------------------------------------------------------
// Begin Print Commands
//----------------------------------------------------------------------
void lcd_print(const char* str, unsigned char startPos) {
    lcdSetCursor(startPos);
    while (*str) {
        send_data(*str++);
        startPos++;
        if (startPos == 0x10) startPos = 0x40;  // Salto automático a segunda línea
    }
}
//--End Print Commands--------------------------------------------------

//----------------------------------------------------------------------
// Begin Main
//----------------------------------------------------------------------
void main(void)
{   

    keypad_init();
    heartbeat_init();
    master_i2c_init();
     int key;

    lcd_print("HELLO SPAIN", 0x00);
    lcd_print("1 TO START", 0x40);

    if (keypad_read() == 1)
    {
        send_command(0x01);
        lcd_print("CHOOSE TOPIC", 0x00);
        lcd_print("1 TO CONTINUE", 0x40);

        if (keypad_read() == 1)
        {
            do {
                send_command(0x01); // Clear screen
                lcd_print("1.HIST 2.SPORT", 0x00);
                lcd_print("3.CULTURE 4.FOOD", 0x40);
                __delay_cycles(4000000); // 4 segundos (ajusta según tu reloj)

                key = keypad_read();
                if (key == '1' || key == '2' || key == '3' || key == '4' || key == '5') break;

                send_command(0x01); // Clear screen
                lcd_print("5.RANDOM", 0x00);
                __delay_cycles(2000000); // 2 segundos

                key = keypad_read();
            } while (key != '1' && key != '2' && key != '3' && key != '4' && key != '5');     
        }
    }

    //while (true) 
    //{   
    //}
      
    /*while(true)
    {
        counter = 0;
        key = 0;

        while (counter < TABLE_SIZE)
        {
            key = keypad_unlocking();
            if(key!=0)
            {
                introduced_password [counter] = key;
                counter++;
            }        
        }
        //Compare the introduced code with the real code   
        equal = 1;   
        for (i = 0; i < TABLE_SIZE; i++) {
            if (introduced_password[i] != real_code[i]) 
            {
                equal = 0;
                break;
            }
        }
        // Verify the code
        if (equal==1) 
        {
            printf("Correct Code!\n");
            adc_init();
            counter = 0;
            rgb_led_continue(1);            // Set LED to blue
            for (i = 0; i < TABLE_SIZE; i++) 
            {
                introduced_password[i] = 0;        
            }
            bool_unlocked = true;
            master_i2c_send('Z', 0x048);            // lcd slave
            keypad_unlocked();  // This now handles polling until 'D' is pressed
        } 
        else 
        {
            printf("Incorrect code. Try again.\n");
            counter = 0;  // Reinitiate counter to try again
            rgb_led_continue(3);            // Set LED to red
            master_i2c_send('\0', 0x058);
            master_i2c_send('\0', 0x048);
            //led_patterns('\0');
            for (i = 0; i < TABLE_SIZE; i++) 
            {
                introduced_password[i] = 0;        
            }
        }    
    }*/
}
//--End Main------------------------------------------------------------

//----------------------------------------------------------------------
// Begin Interrupt Service Routines
//----------------------------------------------------------------------
/*#pragma vector = TIMER1_B0_VECTOR
__interrupt void ISR_TB1_CCR0(void)
{
    ADCCTL0 |= ADCENC | ADCSC;             // Start ADC conversion
    while (ADCCTL1 & ADCBUSY);             // Wait for result
    //adc_result = ADCMEM0;

    //samples[sample_index++] = adc_result;
    //if (sample_index >= window_size)
    {
        sample_index = 0;
        adc_ready = true;   // Tell main loop to process this
    }

    TB1CCTL0 &= ~CCIFG;                    // Clear interrupt flag
}*/
//-- End Interrupt Service Routines ------------------------------------

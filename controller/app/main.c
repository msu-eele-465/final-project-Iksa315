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

#define RS BIT0     // P2.0 -> RS
#define EN BIT1     // P2.1 -> EN
#define D4 BIT4     // P1.4 -> Data bit 4
#define D5 BIT5     // P1.5 -> Data bit 5
#define D6 BIT6     // P1.6 -> Data bit 6
#define D7 BIT7     // P1.7 -> Data bit 7

#define COL 4
#define ROW 4
#define TABLE_SIZE 4

volatile bool reset_requested = false;
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
    lcdInit();

    // Botón en P2.3
    P2DIR &= ~BIT3;
    P2REN |= BIT3;
    P2OUT |= BIT3;
    P2IES |= BIT3;
    P2IFG &= ~BIT3;
    P2IE |= BIT3;

    __enable_interrupt();

    char key;

    while (true) 
    {
    restart_game:
        reset_requested = false;

        send_command(0x01);
        lcd_print("HELLO SPAIN", 0x00);
        lcd_print("1 TO START", 0x40);

        do {
            key = keypad_read();
            if (reset_requested) goto restart_game;
        } while (key != '1');

        send_command(0x01);
        lcd_print("CHOOSE TOPIC", 0x00);
        lcd_print("1 TO CONTINUE", 0x40);

        do {
            key = keypad_read();
            if (reset_requested) goto restart_game;
        } while (key != '1');

        unsigned int i;

        do {
            send_command(0x01);
            lcd_print("1.HIST 2.SPORT", 0x00);
            lcd_print("3.CULTURE 4.FOOD", 0x40);

            for (i = 0; i < 1000; i++) {
                key = keypad_read();
                if (reset_requested) goto restart_game;
                if (key == '1' || key == '2' || key == '3' || key == '4' || key == '5') break;
                __delay_cycles(1000);
            }
            if (reset_requested) goto restart_game;
            if (key == '1' || key == '2' || key == '3' || key == '4' || key == '5') break;

            send_command(0x01);
            lcd_print("5.RANDOM", 0x00);

            for (i = 0; i < 500; i++) {
                key = keypad_read();
                if (reset_requested) goto restart_game;
                if (key == '1' || key == '2' || key == '3' || key == '4' || key == '5') break;
                __delay_cycles(1000);
            }
        } while (key != '1' && key != '2' && key != '3' && key != '4' && key != '5');


        switch (key) 
        {
            case '1':  // Historia de España
            {
                unsigned int correct = 0;
                unsigned int incorrect = 0;
                unsigned int no_answer = 0;
                char answer;

                // ------------ PREGUNTA 1 ------------
                send_command(0x01);
                lcd_print("Granada fell", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)1492 B)1500", 0x00);
                lcd_print("C)1400 D)1512", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'A') 
                {
                    correct++;  
                    master_i2c_send('A', 0x48);        
                }
                else 
                {
                    incorrect++;
                    master_i2c_send('A', 0x48);
                }

                // ------------ PREGUNTA 2 ------------
                send_command(0x01);
                lcd_print("End dictatory", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)1975 B)1980", 0x00);
                lcd_print("C)1970 D)1965", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'A') correct++;
                else incorrect++;

                // ------------ PREGUNTA 3 ------------
                send_command(0x01);
                lcd_print("Current leader", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)Pedro B)Rajoy", 0x00);
                lcd_print("C)Laia D)Jaime", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'A') correct++;
                else incorrect++;

                // ------------ PREGUNTA 4 ------------
                send_command(0x01);
                lcd_print("El Cid war", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)Succ B)Civil", 0x00);
                lcd_print("C)Reconq D)Indep", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'C') correct++;
                else incorrect++;

                // ------------ PREGUNTA 5 ------------
                send_command(0x01);
                lcd_print("Civil war start", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)1936 B)1940", 0x00);
                lcd_print("C)1918 D)1923", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'A') correct++;
                else incorrect++;

                // ------------ PREGUNTA 6 ------------
                send_command(0x01);
                lcd_print("Dictator 1939", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)Franco B)Primo", 0x00);
                lcd_print("C)Genis D)Felipe", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'A') correct++;
                else incorrect++;

                // ------------ PREGUNTA 7 ------------
                send_command(0x01);
                lcd_print("Painter Guernica", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)Picasso B)Dali", 0x00);
                lcd_print("C)Goya D)Miro", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'A') correct++;
                else incorrect++;

                // ------------ PREGUNTA 8 ------------
                send_command(0x01);
                lcd_print("First Republic", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)1873 B)1931", 0x00);
                lcd_print("C)1868 D)1848", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'A') correct++;
                else incorrect++;

                // ------------ PREGUNTA 9 ------------
                send_command(0x01);
                lcd_print("Disc. America", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)1492 B)1500", 0x00);
                lcd_print("C)1480 D)1512", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'A') correct++;
                else incorrect++;

                // ------------ PREGUNTA 10 ------------
                send_command(0x01);
                lcd_print("Cadiz Constitut.", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)1812 B)1808", 0x00);
                lcd_print("C)1820 D)1833", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'A') correct++;
                else incorrect++;

                // ------------ Mostrar resultados ------------
                send_command(0x01);
                lcd_print("C:", 0x06);
                send_data('0' + correct);
                lcd_print("W:", 0x00);
                send_data('0' + incorrect);

                __delay_cycles(10000000); // 10 segundos
            }
            break;

            case '2':  // Spanish Sports
            {
                unsigned int correct = 0;
                unsigned int incorrect = 0;
                unsigned int no_answer = 0;
                char answer;

                // ------------ PREGUNTA 1 ------------
                send_command(0x01);
                lcd_print("Topuria sport", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)MMA B)Boxing", 0x00);
                lcd_print("C)Judo D)Karate", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'A') correct++;
                else incorrect++;

                // ------------ PREGUNTA 2 ------------
                send_command(0x01);
                lcd_print("Topuria born in", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)N/A B)Spain", 0x00);
                lcd_print("C)Russia D)Cuba", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'A') correct++;
                else incorrect++;

                // ------------ PREGUNTA 3 ------------
                send_command(0x01);
                lcd_print("Nadal sport", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)Tennis B)Golf", 0x00);
                lcd_print("C)Soccer D)Surf", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'A') correct++;
                else incorrect++;

                // ------------ PREGUNTA 4 ------------
                send_command(0x01);
                lcd_print("Gasol sport", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)Basket B)Golf", 0x00);
                lcd_print("C)Volley D)MMA", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'A') correct++;
                else incorrect++;

                // ------------ PREGUNTA 5 ------------
                send_command(0x01);
                lcd_print("Iniesta sport", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)Golf B)Soccer", 0x00);
                lcd_print("C)Ski D)Surf", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'B') correct++;
                else incorrect++;

                // ------------ PREGUNTA 6 ------------
                send_command(0x01);
                lcd_print("2010 WC sport?", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)Soccer B)Surf", 0x00);
                lcd_print("C)Rugby D)Tennis", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'A') correct++;
                else incorrect++;

                // ------------ PREGUNTA 7 ------------
                send_command(0x01);
                lcd_print("Contador sport?", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)Cycling B)Golf", 0x00);
                lcd_print("C)Tennis D)Run", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'A') correct++;
                else incorrect++;

                // ------------ PREGUNTA 8 ------------
                send_command(0x01);
                lcd_print("Craviotto sport", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)Canoeing B)Box", 0x00);
                lcd_print("C)Judo D)Rugby", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'A') correct++;
                else incorrect++;

                // ------------ PREGUNTA 9 ------------
                send_command(0x01);
                lcd_print("Maravilla Martinez", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)Boxing B)MMA", 0x00);
                lcd_print("C)Soccer D)Judo", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'A') correct++;
                else incorrect++;

                // ------------ PREGUNTA 10 ------------
                send_command(0x01);
                lcd_print("Masvidal sport", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)MMA B)Boxing", 0x00);
                lcd_print("C)Judo D)Surf", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'A') correct++;
                else incorrect++;

                // ------------ Mostrar resultados ------------
                send_command(0x01);
                lcd_print("C:", 0x06);
                send_data('0' + correct);
                lcd_print("W:", 0x00);
                send_data('0' + incorrect);

                __delay_cycles(10000000); // 10 segundos
            }
            break;

            case '3':  // Culture
            {
                unsigned int correct = 0;
                unsigned int incorrect = 0;
                unsigned int no_answer = 0;
                char answer;

                // ------------ PREGUNTA 1 ------------
                send_command(0x01);
                lcd_print("Catalonia Indep.", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)No B)Yes", 0x00);
                lcd_print("C)Maybe D)1960", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'A') correct++;
                else incorrect++;

                // ------------ PREGUNTA 2 ------------
                send_command(0x01);
                lcd_print("Hispanic day", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)12 B)N/A", 0x00);
                lcd_print("C)15 D)21", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'A') correct++;
                else incorrect++;

                // ------------ PREGUNTA 3 ------------
                send_command(0x01);
                lcd_print("Sagrada Familia", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)Gaudi B)Dali", 0x00);
                lcd_print("C)Miro D)Sorolla", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'A') correct++;
                else incorrect++;

                // ------------ PREGUNTA 4 ------------
                send_command(0x01);
                lcd_print("Bull fest name", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)Fermin B)Fallas", 0x00);
                lcd_print("C)Merce D)Abril", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'A') correct++;
                else incorrect++;

                // ------------ PREGUNTA 5 ------------
                send_command(0x01);
                lcd_print("Prado", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)Museum B)N/A", 0x00);
                lcd_print("C)Person D)Food", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'A') correct++;
                else incorrect++;

                // ------------ PREGUNTA 6 ------------
                send_command(0x01);
                lcd_print("Valencia fest", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)Fallas B)Merce", 0x00);
                lcd_print("C)N/A D)Feria", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'A') correct++;
                else incorrect++;

                // ------------ PREGUNTA 7 ------------
                send_command(0x01);
                lcd_print("Meninas Painter", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)Manuel B)Goya", 0x00);
                lcd_print("C)Dali D)Picasso", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'A') correct++;
                else incorrect++;

                // ------------ PREGUNTA 8 ------------
                send_command(0x01);
                lcd_print("Galician instrument", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)Gaita B)Laud", 0x00);
                lcd_print("C)Tambor D)Casta", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'A') correct++;
                else incorrect++;

                // ------------ PREGUNTA 9 ------------
                send_command(0x01);
                lcd_print("Benicassim fest", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)FIB B)Mad Cool", 0x00);
                lcd_print("C)BBK D)Sonorama", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'A') correct++;
                else incorrect++;

                // ------------ PREGUNTA 10 ------------
                send_command(0x01);
                lcd_print("Don Quijote author", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)CervantesB)N/A", 0x00);
                lcd_print("C)Luis D)Lopez", 0x40);

                answer = 0;
               while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'A') correct++;
                else incorrect++;

                // ------------ Mostrar resultados ------------
                send_command(0x01);
                lcd_print("C:", 0x06);
                send_data('0' + correct);
                lcd_print("W:", 0x00);
                send_data('0' + incorrect);

                __delay_cycles(10000000); // 10 segundos
            }
            break;

            case '4':  // Food
            {
                unsigned int correct = 0;
                unsigned int incorrect = 0;
                unsigned int no_answer = 0;
                char answer;

                // ------------ PREGUNTA 1 ------------
                send_command(0x01);
                lcd_print("Base of Paella", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)Rice B)Potato", 0x00);
                lcd_print("C)Pasta D)Flour", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'A') correct++;
                else incorrect++;

                // ------------ PREGUNTA 2 ------------
                send_command(0x01);
                lcd_print("Gazpacho is", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)Cold B)Fried", 0x00);
                lcd_print("C)Grilled D)Hot", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'A') correct++;
                else incorrect++;

                // ------------ PREGUNTA 3 ------------
                send_command(0x01);
                lcd_print("Famous ham?", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)Iberico B)York", 0x00);
                lcd_print("C)Serrano D)N/A", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'A') correct++;
                else incorrect++;

                // ------------ PREGUNTA 4 ------------
                send_command(0x01);
                lcd_print("Christmas dessert", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)Roscon B)Flan", 0x00);
                lcd_print("C)Churros D)Cake", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'A') correct++;
                else incorrect++;

                // ------------ PREGUNTA 5 ------------
                send_command(0x01);
                lcd_print("Basque dish", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)MarmitakoB)N/A", 0x00);
                lcd_print("C)Pasta D)Paella", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'A') correct++;
                else incorrect++;

                // ------------ PREGUNTA 6 ------------
                send_command(0x01);
                lcd_print("Nougat base", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)Almond B)Hazelnut", 0x00);
                lcd_print("C)Cocoa D)Butter", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'A') correct++;
                else incorrect++;

                // ------------ PREGUNTA 7 ------------
                send_command(0x01);
                lcd_print("Typical drink", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)Sangria B)Cava", 0x00);
                lcd_print("C)Milk D)Cafe", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'A') correct++;
                else incorrect++;

                // ------------ PREGUNTA 8 ------------
                send_command(0x01);
                lcd_print("New year fruit", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)Grape B)Peach", 0x00);
                lcd_print("C)Apple D)Kiwi", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'A') correct++;
                else incorrect++;

                // ------------ PREGUNTA 9 ------------
                send_command(0x01);
                lcd_print("Catalan dessert", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)Crema B)Flan", 0x00);
                lcd_print("C)Helado D)Nuts", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'A') correct++;
                else incorrect++;

                // ------------ PREGUNTA 10 ------------
                send_command(0x01);
                lcd_print("Omelette base", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)Egg B)Pasta", 0x00);
                lcd_print("C)Rice D)Bread", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'A') correct++;
                else incorrect++;

                // ------------ Mostrar resultados ------------
                send_command(0x01);
                lcd_print("C:", 0x06);
                send_data('0' + correct);
                lcd_print("W:", 0x00);
                send_data('0' + incorrect);

                __delay_cycles(10000000); // 10 segundos
            }
            break;

            case '5':  // Spain Random
            {
                unsigned int correct = 0;
                unsigned int incorrect = 0;
                unsigned int no_answer = 0;
                char answer;

                // ------------ PREGUNTA 1 ------------
                send_command(0x01);
                lcd_print("Galicia capital", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)Lugo B)Vigo", 0x00);
                lcd_print("C)Santiago D)N/A", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'C') correct++;
                else incorrect++;

                // ------------ PREGUNTA 2 ------------
                send_command(0x01);
                lcd_print("Highest mountain", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)N/A B)Teide", 0x00);
                lcd_print("C)Aneto D)K1", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'B') correct++;
                else incorrect++;

                // ------------ PREGUNTA 3 ------------
                send_command(0x01);
                lcd_print("City Alhambra", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)N/A B)Granada", 0x00);
                lcd_print("C)Cordoba D)Lugo", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'B') correct++;
                else incorrect++;

                // ------------ PREGUNTA 4 ------------
                send_command(0x01);
                lcd_print("River Toledo", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)Tajo B)Duero", 0x00);
                lcd_print("C)Ebro D)Mino", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'A') correct++;
                else incorrect++;

                // ------------ PREGUNTA 5 ------------
                send_command(0x01);
                lcd_print("N of states", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)30 B)17", 0x00);
                lcd_print("C)2 D)21", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'B') correct++;
                else incorrect++;

                // ------------ PREGUNTA 6 ------------
                send_command(0x01);
                lcd_print("Coin", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)Dollar B)Euro", 0x00);
                lcd_print("C)Yen D)Yuan", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'B') correct++;
                else incorrect++;

                // ------------ PREGUNTA 7 ------------
                send_command(0x01);
                lcd_print("Popular sport", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)Soccer B)Golf", 0x00);
                lcd_print("C)Basket D)Tenis", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'A') correct++;
                else incorrect++;

                // ------------ PREGUNTA 8 ------------
                send_command(0x01);
                lcd_print("Catalan architect.", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)Gaudi B)N/A", 0x00);
                lcd_print("C)Goya D)Dali", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'A') correct++;
                else incorrect++;

                // ------------ PREGUNTA 9 ------------
                send_command(0x01);
                lcd_print("Tomato festival", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)Fallas B)Fermin", 0x00);
                lcd_print("C)Tomatina D)N/A", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'C') correct++;
                else incorrect++;

                // ------------ PREGUNTA 10 ------------
                send_command(0x01);
                lcd_print("Flamenco region", 0x00);
                __delay_cycles(5000000);

                send_command(0x01);
                lcd_print("A)N/A B)Andalucia", 0x00);
                lcd_print("C)Lugo D)Madrid", 0x40);

                answer = 0;
                while (1) {
                    if (reset_requested) goto restart_game;
                    answer = keypad_read();
                    if (answer == 'A' || answer == 'B' || answer == 'C' || answer == 'D') break;
                }

                if (answer == 'B') correct++;
                else incorrect++;

                // ------------ Mostrar resultados ------------
                send_command(0x01);
                lcd_print("C:", 0x06);
                send_data('0' + correct);
                lcd_print("W:", 0x00);
                send_data('0' + incorrect);

                __delay_cycles(10000000); // 10 segundos
            }
            break;
        }
    }
}
//--End Main------------------------------------------------------------

#pragma vector=PORT2_VECTOR
__interrupt void PORT2_ISR(void)
{
    if (P2IFG & BIT3) {
        reset_requested = true;
        P2IFG &= ~BIT3;
    }
}


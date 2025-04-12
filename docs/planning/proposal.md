# Final project proposal

- [x] I have reviewed the project guidelines.
- [x] I will be working alone on this project.
- [x] No significant portion of this project will be (or has been) used in other course work.

## Embedded System Description

This project is a trivia game that displays cultural questions about Spain on an LCD and takes user input via a keypad. Based on the answer, a slave microcontroller will activate a green or red LED to show whether the answer was correct or incorrect. A separate button input will allow the user to exit the game at any time.

Inputs:
The keypad will be used by the user to navigate categories and select answers to trivia questions.
A push-button will serve as an input to exit the game early when pressed.

Process:
The master microcontroller will handle game logic: category selection, question/answer display, and determining correctness. It will communicate with the slave via I2C to control the LED. It will also monitor the button input to allow the user to quit the game.

Outputs:
The LCD will show categories, questions, and answer options.
The LED connected to the slave will blink green for correct and red for incorrect answers.

## Hardware Setup

Required hardware:
MSP430FR2355 (master)
MSP430FR2310 (slave)
LCD in 4-bit mode
4x4 matrix keypad
Push-button (game exit)
Red/Green LED
Pull-up resistors, connecting wires, breadboard

The master reads input from the keypad and button, updates the LCD, and tells the slave when to blink the LED.

![CircuitDiagram-iker](../assets/IKERSAL_CD.svg)

## Software overview

The master initializes the keypad, button, LCD, and I2C. A welcome screen prompts the user to select a category. It then cycles through ten questions in that category, displaying each question and four options. When the user presses a key, the master checks the answer and sends a signal to the slave via I2C to blink the LED accordingly. At any time, if the user presses the exit button, the game stops and returns to the welcome screen or ends.

![FlowChart-iker](../assets/FlowChartiker.svg)

## Testing Procedure

I will test each component individually: LCD display output, keypad input recognition, button response, and I2C LED control. Then, I will test full game flow with all components connected. To demo the project, I will show a working trivia round with category selection, question answering, LED response, and game exit using the button. All parts can be shown on a single breadboard, with no external equipment needed.

## Prescaler

Desired Prescaler level: 

- [ ] 100%
- [ ] 95% 
- [ ] 90% 
- [ ] 85% 
- [ ] 80% 
- [x] 75% 

### Prescalar requirements 

**Outline how you meet the requirements for your desired prescalar level**

**The inputs to the system will be:**

Keypad: used to select categories and answer questions.
Button: used to exit the game early.

**The outputs of the system will be:**

LCD: displays instructions, questions, and options.
LED: green or red blink depending on correct/incorrect answer.

To create an educational trivia game that quizzes users on Spanish culture using embedded hardware for interactive input and visual feedback.

**The new hardware or software modules are:**
Implementation of keypad scanning logic
LCD control in 4-bit mode
I2C communication between master and slave
Game state logic and trivia database
Interrupt or polling logic for button-based game exit

The Master will be responsible for:

Handling game logic, displaying information on the LCD, scanning keypad and button inputs, and sending commands to the slave based on user input.

The Slave(s) will be responsible for:

Receiving I2C commands from the master and controlling the LED to blink green or red depending on the correctness of the answer.

### Argument for Desired Prescaler

This project uses I2C communication between two microcontrollers, integrates a matrix keypad for user input, an LCD for real-time display, and a button for interrupt-style exit control. The game structure includes conditional logic, input/output synchronization, and user feedback through LED indication. While the system uses common peripherals, the combination of components and the implementation of full game logic make it a complete and interactive embedded system application, which justifies a 75% prescaler level.
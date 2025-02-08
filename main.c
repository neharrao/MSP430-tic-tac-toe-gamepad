/*
 * Tic-Tac-Toe Gamepad
 * Neha R Rao
 */

#include <msp430.h>
#include "ssd1306.h"  // Include your SSD1306 OLED library

// Function prototypes
void initI2C();
void i2c_write(unsigned int slave_address, unsigned char *data, unsigned int length);
void initButtons();
void initBuzzer();
void playBuzzer(unsigned int frequency, unsigned int duration);
void playEventSound(char event);
void displayStartMessage();
void displayPlayerSelection();
void drawGrid();
void drawMarker(unsigned int x, unsigned int y, char marker);
void moveMarker();
void placeMarker();
void checkWinCondition();
void resetGame();
void initDebounceTimer();
void initUART();
void transmitData(const char *data);
void handleReceivedData();
void startDebounceTimer();

// Global variables
unsigned int markerX = 0;  // Marker column position (0 to 2)
unsigned int markerY = 0;  // Marker row position (0 to 2)
char grid[3][3] = {{' ', ' ', ' '}, {' ', ' ', ' '}, {' ', ' ', ' '}};  // Logical grid for X and O
char currentPlayer = 'X';  // Current player ('X' or 'O')
unsigned int gameOver = 0;  // Game state flag
volatile unsigned int gamePhase = 0;  // 0: Player Selection, 1: Gameplay
volatile unsigned char resetPending = 0;  // 0: No reset, 1: Reset is pending
volatile unsigned char waitingForReset = 0; // 0: Normal state, 1: Waiting for Reset

// UART-specific variables
volatile char rxBuffer[10] = {0};
volatile unsigned int rxIndex = 0;
volatile char txBuffer[10] = {0};
volatile unsigned int txIndex = 0;
volatile unsigned char dataReceived = 0;
volatile unsigned char resetHandled = 0;  // 0: Reset not handled, 1: Reset handled

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;  // Stop watchdog timer
    BCSCTL1 = CALBC1_1MHZ;
    DCOCTL = CALDCO_1MHZ;

    initI2C();                 // Initialize I2C communication
    initButtons();             // Initialize buttons
    initBuzzer();              // Initialize buzzer
    initLED();                 // Initialize LED
    initDebounceTimer();       // Initialize debounce timer
    initUART();                // Initialize UART communication
    ssd1306_init();            // Initialize OLED display

    __delay_cycles(500000);    // Short delay to stabilize OLED

    displayPlayerSelection();  // Allow players to choose 'X' or 'O'

    __bis_SR_register(GIE);    // Enable global interrupts

    while (1) {
        if (gameOver && !resetHandled) {  // Only reset the game once
            resetGame();
            resetHandled = 1;  // Mark reset as handled
        }

        if (gamePhase == 1) {  // Gameplay Setup Phase
            drawGrid();  // Transition to gameplay by drawing the grid
            gamePhase = 2;  // Indicate that the game is now in progress
        }

        if (dataReceived) {
            handleReceivedData();  // Handle received UART data
            dataReceived = 0;
        }
    }
}

// Initialize I2C Communication
void initI2C() {
    P1SEL |= BIT6 + BIT7;   // Assign I2C pins to USCI_B0 (SCL, SDA)
    P1SEL2 |= BIT6 + BIT7;
    UCB0CTL1 |= UCSWRST;    // Enable software reset
    UCB0CTL0 = UCMST + UCMODE_3 + UCSYNC;  // I2C Master mode
    UCB0CTL1 |= UCSSEL_2;   // Use SMCLK
    UCB0BR0 = 10;           // fSCL = SMCLK/10 = ~100kHz
    UCB0BR1 = 0;
    UCB0CTL1 &= ~UCSWRST;   // Clear reset
}

// I2C Write Function
void i2c_write(unsigned int slave_address, unsigned char *data, unsigned int length) {
    UCB0I2CSA = slave_address;  // Set slave address
    UCB0CTL1 |= UCTR + UCTXSTT;  // Transmit mode and START condition

    while (length--) {
        while (!(IFG2 & UCB0TXIFG));  // Wait for TX buffer to be ready
        UCB0TXBUF = *data++;          // Load data into TX buffer
    }

    while (!(IFG2 & UCB0TXIFG));  // Wait for the last byte to be transmitted
    UCB0CTL1 |= UCTXSTP;          // Send STOP condition
    while (UCB0CTL1 & UCTXSTP);   // Wait for STOP condition to complete
}

// Initialize Buttons
void initButtons() {
    P1DIR &= ~(BIT0 + BIT3);  // Set P1.2 and P1.3 as inputs
    P1REN |= BIT0 + BIT3;     // Enable pull-up/down resistors
    P1OUT |= BIT0 + BIT3;     // Configure as pull-ups
    P1IES |= BIT0 + BIT3;     // Trigger on high-to-low transition
    P1IFG &= ~(BIT0 + BIT3);  // Clear interrupt flags
    P1IE |= BIT0 + BIT3;      // Enable interrupts for P1.2 and P1.3
}

// Initialize Buzzer
void initBuzzer() {
    P1DIR |= BIT4;  // Set P1.4 as output for buzzer
    P1OUT &= ~BIT4; // Turn off buzzer initially
}


// Play Buzzer
void playBuzzer(unsigned int frequency, unsigned int duration) {
    unsigned int delay = 1000000 / frequency / 2;  // Calculate delay for given frequency
    unsigned int cycles = (frequency * duration) / 1000;  // Calculate total cycles for duration in ms

    while (cycles--) {
        P1OUT |= BIT4;          // Turn on buzzer
        unsigned int i = 0;
        for (i = 0; i < delay; i++) {
            __no_operation();   // Software delay loop
        }
        P1OUT &= ~BIT4;         // Turn off buzzer
        for ( i = 0; i < delay; i++) {
            __no_operation();   // Software delay loop
        }
    }
}

void playEventSound(char event) {
    switch (event) {
        case 'W':  // Win sound
            playBuzzer(800, 200);  // Higher tone
            __delay_cycles(200000);  // Short pause
            playBuzzer(1000, 200);  // Even higher tone
            __delay_cycles(200000);
            playBuzzer(1200, 300);  // Final tone
            break;
        case 'L':  // Lose sound
            playBuzzer(800, 200);  // Higher tone
            __delay_cycles(200000);
            playBuzzer(600, 200);  // Lower tone
            __delay_cycles(200000);
            playBuzzer(400, 300);  // Final low tone
            break;
        case 'D':  // Draw sound
            playBuzzer(700, 200);  // Flat tone
            __delay_cycles(200000);
            playBuzzer(700, 200);  // Repeat the same tone
            break;
        default:  // No action for unknown event
            break;
    }
}

// Player Selection Phase
void displayPlayerSelection() {
    ssd1306_clearDisplay();

    // Display player selection screen
    ssd1306_printText(0, 0, "Do you want to play?");
    ssd1306_printText(0, 2, "Choose X or O");
    ssd1306_printText(0, 4, "Press Btn 1 for X");
    ssd1306_printText(0, 5, "Press Btn 2 for O");
}

// Draw the Tic Tac Toe Grid
void drawGrid() {
    ssd1306_clearDisplay();  // Clear the OLED display

    // Declare variables outside the loop for compatibility
    int x, y, i, j;

    // Draw horizontal lines
    for (y = 21; y <= 43; y += 22) {
        for (x = 0; x < 128; x++) {
            ssd1306_setPosition(x, y / 8);  // Set pixel position
            buffer[0] = 0x40;
            buffer[1] = 0xFF;  // Full line
            i2c_write(SSD1306_I2C_ADDRESS, buffer, 2);
        }
    }

    // Draw vertical lines
    for (x = 42; x <= 85; x += 43) {
        for (y = 0; y < 64; y++) {
            ssd1306_setPosition(x, y / 8);
            buffer[0] = 0x40;
            buffer[1] = 0xFF;  // Full line
            i2c_write(SSD1306_I2C_ADDRESS, buffer, 2);
        }
    }

    // Redraw any existing markers
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            if (grid[i][j] != ' ') {
                drawMarker(j, i, grid[i][j]);
            }
        }
    }

    drawMarker(markerX, markerY, currentPlayer);  // Highlight the current marker
}


// Draw Marker (X or O) on the Grid
void drawMarker(unsigned int x, unsigned int y, char marker) {
    // Map grid coordinates to pixel positions
    unsigned int pixelX = x * 42 + 14;  // Adjust for grid alignment
    unsigned int pixelY = y * 21 + 7;

    ssd1306_setPosition(pixelX, pixelY / 8);

    // Draw the marker ('X' or 'O') on the grid
    char markerStr[2] = {marker, '\0'};
    ssd1306_printText(pixelX, pixelY / 8, markerStr);
}

// Initialize Debounce Timer
void initDebounceTimer() {
    TA0CCTL0 = CCIE;              // Enable Timer A interrupt
    TA0CCR0 = 50000;              // Set debounce interval (~10 ms)
    TA0CTL = TASSEL_2 + MC_1;     // SMCLK, up mode
}

void startDebounceTimer() {
    TA0R = 0;                     // Reset timer
    TA0CTL |= MC_1;               // Start timer in up mode
}

#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A(void) {
    P1IE |= (BIT0 + BIT3);        // Re-enable button interrupts
    TA0CTL &= ~MC_1;              // Stop timer
    TA0CCTL0 &= ~CCIFG;           // Clear timer interrupt flag
}

// UART Initialization
void initUART() {
    P1SEL |= BIT1 + BIT2;  // Set P1.1, P1.2 to UART mode
    P1SEL2 |= BIT1 + BIT2;

    //UCA0CTL1 &= ~UCSWRST;  // Hold USCI in reset
    UCA0CTL1 |= UCSSEL_2; // Use SMCLK
    UCA0BR0 = 104;        // Set baud rate to 9600
    UCA0BR1 = 0;
    UCA0MCTL = UCBRS0;    // Modulation
    UCA0CTL1 &= ~UCSWRST; // Release USCI for operation

    IE2 |= UCA0RXIE;  // Enable RX and TX interrupts
}

// UART Transmit Data
void transmitData(const char *data) {
    unsigned int i = 0;

    // Sequentially transmit each character in the string
    while (data[i] != '\0') {        // Loop until the null terminator
        while (!(IFG2 & UCA0TXIFG)); // Wait for TX buffer to be ready
        UCA0TXBUF = data[i];         // Transmit the current character
        i++;                         // Move to the next character
    }

    // Optionally send a null terminator explicitly
    while (!(IFG2 & UCA0TXIFG));
    UCA0TXBUF = '\0';

    __delay_cycles(50000);  // Add a small delay to ensure message is fully sent
}

// UART Receive Handler
#pragma vector = USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void) {
    char receivedChar = UCA0RXBUF;  // Read the received character

    if (rxIndex < sizeof(rxBuffer) - 1) {
        rxBuffer[rxIndex++] = receivedChar;  // Add character to RX buffer
    }
    if (receivedChar == '\0') {  // Check for null terminator
        dataReceived = 1;          // Set the data received flag
        rxBuffer[rxIndex] = '\0';  // Null-terminate the string
        //dataReceived = 1;          // Set the data received flag
        rxIndex = 0;               // Reset the buffer index
    }
}

// Handle Received Data
void handleReceivedData() {
    if (dataReceived) {
        if (rxBuffer[0] == 'A') {  // Marker 'X' selected by the other board
            currentPlayer = 'O';   // Assign this board as 'O'
            gamePhase = 1;         // Transition to Gameplay Phase
            updateLED(0);          // Turn off this board's LED as it's not this board's turn
        }
        else if (rxBuffer[0] == 'B') {  // Marker 'O' selected by the other board
            currentPlayer = 'X';        // Assign this board as 'X'
            gamePhase = 1;              // Transition to Gameplay Phase
            updateLED(0);               // Turn off this board's LED as it's not this board's turn
        }
        else if (rxBuffer[0] == 'P') {  // Marker placement received
            unsigned int x = rxBuffer[1] - '0';  // Extract X coordinate
            unsigned int y = rxBuffer[2] - '0';  // Extract Y coordinate
            char marker = rxBuffer[3];          // Extract marker ('X' or 'O')

            grid[y][x] = marker;                // Update the grid
            drawMarker(x, y, marker);           // Draw the marker on OLED
            playBuzzer(1000, 200);              // Play placement sound
            updateLED(1);                       // Turn on LED to indicate it's this board's turn
        }
        else if (rxBuffer[0] == 'R') {  // Reset game message received
            resetGame();  // Trigger reset game
        }
        else if (rxBuffer[0] == 'G') {  // Winning message received
            char winner = rxBuffer[1];  // Extract winner ('X' or 'O')
            ssd1306_clearDisplay();
            ssd1306_printText(0, 0, "Game Over!");

            if (winner == 'X') {  // Check if X wins
                ssd1306_printText(0, 2, "X Wins!");
            } else if (winner == 'O') {  // Check if O wins
                ssd1306_printText(0, 2, "O Wins!");
            }

            playEventSound('W');  // Play the winning sound
            __delay_cycles(5000000);  // Display message for 5 seconds

            resetGame();  // Reset the game after showing the result
        }
        else if (rxBuffer[0] == 'D') {  // Draw message received
            ssd1306_clearDisplay();
            ssd1306_printText(0, 0, "Game Over!");
            ssd1306_printText(0, 2, "It's a Draw!");

            playEventSound('D');  // Play the draw sound
            __delay_cycles(5000000);  // Display message for 5 seconds

            resetGame();  // Reset the game after showing the result
        }

        // Reset RX buffer for the next message
        rxBuffer[0] = '\0';       // Clear the first character of the RX buffer
        rxIndex = 0;              // Reset RX buffer index
        dataReceived = 0;         // Reset the flag after processing
    }
}

void moveMarker() {
    // Clear the current marker highlight without altering existing markers
    if (grid[markerY][markerX] == ' ') {
        drawMarker(markerX, markerY, ' ');  // Clear the highlight
    } else {
        drawMarker(markerX, markerY, grid[markerY][markerX]);  // Redraw existing marker
    }

    // Find the next empty cell
    unsigned int startX = markerX, startY = markerY;
    do {
        // Move to the next position
        markerX++;
        if (markerX >= 3) {  // Move to the next row after the last column
            markerX = 0;
            markerY++;
            if (markerY >= 3) {  // Wrap back to the first row after the last row
                markerY = 0;
            }
        }

        // If we've looped back to the starting position, break to avoid infinite loop
        if (markerX == startX && markerY == startY) {
            break;
        }
    } while (grid[markerY][markerX] != ' ');  // Continue until an empty cell is found

    // Highlight the new position for navigation
    if (grid[markerY][markerX] == ' ') {
        drawMarker(markerX, markerY, currentPlayer);  // Highlight for navigation
    } else {
        drawMarker(markerX, markerY, grid[markerY][markerX]);  // Leave the placed marker as is
    }

    // Play navigation buzzer
    playBuzzer(400, 200);
}

// Place Marker (Updated with UART)
void placeMarker() {
    if (grid[markerY][markerX] == ' ') {
        grid[markerY][markerX] = currentPlayer;
        drawMarker(markerX, markerY, currentPlayer);

        char message[5] = {'P', markerX + '0', markerY + '0', currentPlayer, '\0'};
        transmitData(message);
        checkWinCondition();
 playBuzzer(1000, 300);
        updateLED(0);
    }
}

// Button Interrupt Service Routine
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void) {
    __delay_cycles(3000);

    if (P1IFG & BIT0) {  // Button for Player 1
        P1IE &= ~BIT0;
        startDebounceTimer();

        if (gamePhase == 0) {  // Marker Selection Phase
            currentPlayer = 'X';  // Assign Player 1 as 'X'
            transmitData("A");    // Send 'A' to the other board
            gamePhase = 1;        // Transition to Gameplay Phase
            drawGrid();           // Draw the grid for the new game
            updateLED(1);
        } else if (gamePhase == 2) {  // Gameplay Phase
            moveMarker();

        }

        P1IFG &= ~BIT0;
    } else if (P1IFG & BIT3) {  // Button for Player 2
        P1IE &= ~BIT3;
        startDebounceTimer();

        if (gamePhase == 0) {  // Marker Selection Phase
            currentPlayer = 'O';  // Assign Player 2 as 'O'
            transmitData("B");    // Send 'B' to the other board
            gamePhase = 1;        // Transition to Gameplay Phase
            drawGrid();           // Draw the grid for the new game
            updateLED(1);
        } else if (gamePhase == 2) {  // Gameplay Phase
            placeMarker();

        }

        P1IFG &= ~BIT3;
    }
}

// Check for Win Condition
void checkWinCondition() {
    int i, j;
    int isDraw = 1; // Assume it's a draw unless proven otherwise

    // Check rows and columns for a win
    for (i = 0; i < 3; i++) {
        if ((grid[i][0] == currentPlayer && grid[i][1] == currentPlayer && grid[i][2] == currentPlayer) ||
            (grid[0][i] == currentPlayer && grid[1][i] == currentPlayer && grid[2][i] == currentPlayer)) {
            gameOver = 1;

            // Play winning sound
            playEventSound('W');

            // Send "Game Over" and winner message to the other board
            char message[5] = {'G', currentPlayer, '\0'};
            transmitData(message);

            // Display "Game Over" and winner on this board
            ssd1306_clearDisplay();
            ssd1306_printText(0, 0, "Game Over!");
            ssd1306_printText(0, 2, currentPlayer == 'X' ? "X Wins!" : "O Wins!");

            // Delay to display result for 5 seconds
            __delay_cycles(5000000);

            resetPending = 1;  // Indicate reset is required
            return;
        }
    }

    // Check diagonals for a win
    if ((grid[0][0] == currentPlayer && grid[1][1] == currentPlayer && grid[2][2] == currentPlayer) ||
        (grid[0][2] == currentPlayer && grid[1][1] == currentPlayer && grid[2][0] == currentPlayer)) {
        gameOver = 1;

        // Send "Game Over" and winner message to the other board
        char message[5] = {'G', currentPlayer, '\0'};
        transmitData(message);

        // Display "Game Over" and winner on this board
        ssd1306_clearDisplay();
        ssd1306_printText(0, 0, "Game Over!");
        ssd1306_printText(0, 2, currentPlayer == 'X' ? "X Wins!" : "O Wins!");

        // Delay to display result for 5 seconds
        __delay_cycles(5000000);

        resetPending = 1;  // Indicate reset is required
        return;
    }

    // Check if any cell is still empty
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            if (grid[i][j] == ' ') {
                isDraw = 0; // Not a draw if an empty cell is found
                break;
            }
        }
        if (!isDraw) break; // Exit the loop if we find an empty cell
    }

    // If no win and no empty cells, it's a draw
    if (isDraw) {
        gameOver = 1;

        // Play draw sound
        playEventSound('D');

        // Notify the other board about the draw
        transmitData("D"); // 'D' indicates a draw

        // Display "Game Over" and draw message on this board
        ssd1306_clearDisplay();
        ssd1306_printText(0, 0, "Game Over!");
        ssd1306_printText(0, 2, "It's a Draw!");

        // Delay to display result for 5 seconds
        __delay_cycles(5000000);

        resetPending = 1;  // Indicate reset is required
    }
}

// Reset the Game
void resetGame() {
    // Wait a bit before clearing the display and grid
    __delay_cycles(1000000);  // Add a delay before resetting (optional)

    // Reset the game state
    gameOver = 0;
    resetPending = 0;
    waitingForReset = 0;

    // Clear the logical grid
    int i, j;
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            grid[i][j] = ' ';  // Clear the grid
        }
    }

    // Clear the OLED display and reset marker positions
    ssd1306_clearDisplay();
    markerX = 0;
    markerY = 0;

    // Transition back to the player selection phase
    gamePhase = 0;  // Reset game phase to player selection
    displayPlayerSelection();

    // Notify the other board to reset
    transmitData("R");

    // Reset the handled flag after player selection
    resetHandled = 0;  // Allow future resets for new games
}

// Add this function to control the LED
void updateLED(unsigned char isTurn) {
    if (isTurn) {
        P2OUT |= BIT3;  // Turn on LED when it's this board's turn
    } else {
        P2OUT &= ~BIT3;  // Turn off LED otherwise
    }
}

// Modify initLED() function to initialize the LED
void initLED() {
    P2DIR |= BIT3;  // Set P2.3 as output for LED
    P2OUT &= ~BIT3; // Turn off LED initially
}

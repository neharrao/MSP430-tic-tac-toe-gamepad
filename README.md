# MSP430 Tic Tac Toe Gamepad

## 📌 Project Overview
This project implements a **two-player Tic Tac Toe game** using the **MSP430G2553 microcontroller**. Each player has a **custom game controller**, featuring:
- **An OLED display** to show the game board.
- **Buttons** for navigation and placing moves.
- **A buzzer** for audio feedback.
- **An LED** to indicate the active player’s turn.
- **UART communication** between two controllers via a **3-foot communication cable**.

The project reinforces concepts in **embedded system design, real-time communication, and user interaction**.

---

## 🎯 Objectives
✔ Implement **Tic Tac Toe** on the MSP430G2553.  
✔ Establish **UART-based communication** between two controllers.  
✔ Use **buttons** for player navigation and move selection.  
✔ Provide **audio and LED feedback** during gameplay.  
✔ Ensure **low-power operation** using battery power.  

---

# 🛠️ Hardware Design
### Components
| Component | Description | Quantity |
|-----------|------------|---------|
| **MSP430G2553** | Microcontroller for game logic & control | 2 |
| **OLED Display (I2C, SSD1306)** | Displays game board | 2 |
| **Push Buttons** | For navigation & move selection | 6 |
| **LEDs** | Indicates active player's turn | 2 |
| **Buzzer** | Provides sound feedback | 2 |
| **Resistors (1kΩ, 10kΩ)** | Pull-up/down for buttons & LEDs | 10 |
| **Capacitors (0.1µF, 10µF)** | Power stability | 6 |
| **Voltage Regulator (LM7805)** | Converts 9V to 3.3V | 2 |
| **9V Battery** | Power source | 2 |
| **Communication Cable (3ft, UART)** | Connects two controllers | 1 |

### Hardware Setup
- **Buttons**: Connected to P1.0 and P1.3 for navigation and move selection.
- **Buzzer**: Connected to P1.4 for sound feedback.
- **OLED Display**: Uses **I2C protocol** on P1.6 (SDA) and P1.7 (SCL).
- **LED Indicator**: Connected to P2.3 to indicate turn.
- **UART Communication**: P1.1 (RX) and P1.2 (TX) for real-time synchronization.

---

# 🎮 Gameplay Implementation
### Game Features
✔ **Two-player Tic Tac Toe game** with turn-based mechanics.  
✔ **X and O selection** at game start.  
✔ **Game board displayed on OLED screen**.  
✔ **Player navigation using buttons**.  
✔ **Winning/draw conditions detection**.  
✔ **LED indicator** for active player.  
✔ **Buzzer feedback** for move selection.  

### Game Logic
1️⃣ **Players navigate** the 3x3 grid using buttons.  
2️⃣ **Press selection button** to place an X or O.  
3️⃣ **System checks for win or draw conditions** after each move.  
4️⃣ **Game resets automatically** after a win/draw.  

---

# 💾 Software Design
### Software Architecture
- **Main Function**: Manages game state and turns.
- **Initialize Processor**: Sets up WDT, timers, and clock.
- **Initialize Ports**: Configures GPIOs for **buttons, LEDs, buzzer, and I2C**.
- **Timer ISR**: Handles **button debouncing** and **buzzer timing**.
- **Button ISR**: Processes **navigation and selection inputs**.
- **LCD Function**: Updates the game board display.
- **Communication Function**: Synchronizes game state via **UART**.
- **Speaker Function**: Plays **sounds for navigation, move selection, and game results**.

### Pseudo Code
```c
// Initialization
Initialize processor, timers, and I2C
Initialize buttons, LEDs, and buzzer
Display empty grid on OLED

// Main Game Loop
WHILE true DO
    IF navigation button pressed THEN
        Move cursor & update OLED
        Play navigation sound
    ENDIF

    IF selection button pressed THEN
        Validate move
        IF valid THEN
            Place X or O
            Update OLED & check win/draw
            Play selection sound
            Switch turn & update LED
            Send game state via UART
        ELSE
            Display "Invalid Move"
        ENDIF
    ENDIF

    IF reset button pressed THEN
        Reset game state
        Clear OLED and LEDs
        Send reset signal via UART
    ENDIF

    IF game state received via UART THEN
        Update local board
        Display updated game state
    ENDIF
ENDWHILE
```

---

# 🔬 Testing & Results
| Test Scenario | Expected Outcome | Result |
|--------------|----------------|--------|
| **Move navigation** | Cursor moves between grid cells | ✅ Passed |
| **Marker placement** | Updates grid with X/O | ✅ Passed |
| **Win detection** | Displays "Player X/O Wins" | ✅ Passed |
| **Draw detection** | Displays "Draw" message | ✅ Passed |
| **UART synchronization** | Two controllers stay in sync | ✅ Passed |
| **Reset function** | Game resets properly | ✅ Passed |
| **LED turn indication** | LED lights up for active player | ✅ Passed |
| **Power stability** | 3.3V regulated logic | ✅ Passed |

---

# 🛠️ Setup & Execution
### Prerequisites
✔ **Code Composer Studio (CCS)**  
✔ **MSP430G2553 Development Board**  
✔ **UART-to-USB Adapter (for debugging)**  

### Running the Game
1️⃣ **Flash the Code**  
   - Open **Code Composer Studio**.  
   - Load the **Tic Tac Toe code**.  
   - Compile and flash onto the **MSP430G2553**.  

2️⃣ **Connect the Controllers**  
   - Use a **3ft UART cable** to link both boards.  
   - Power each board with a **9V battery**.  

3️⃣ **Play Tic Tac Toe!**  
   - Press **buttons to navigate & select moves**.  
   - **Listen to buzzer feedback** for move confirmation.  
   - **Game resets automatically** after a win/draw.  

---
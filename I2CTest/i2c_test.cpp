// STM32F103 I2C Setup & Test
////////////////////////////////////////////////////////////////////////////////
#include "clock.h"       // Setup system and peripheral clocks
#include "buffer.h"      // Circular Buffers
#include "serial.h"      // USART1 Setup & Functions
#include "delay.h"       // Simple Delay Function
#include <stdint.h>      // uint8_t
#include "stm32f103xb.h" // HW Specific Header
////////////////////////////////////////////////////////////////////////////////
// Function Declarations
void I2CSetup();
void I2CRead(uint8_t NumberBytesToRead, uint8_t slaveAddress);
extern "C" void USART1_IRQHandler(void);
////////////////////////////////////////////////////////////////////////////////
// Buffers
// -------
// Create buffers - Size defined by buffer.h or variable for compiler.
volatile struct Buffer serial_tx_buffer {{},0,0};
volatile struct Buffer serial_rx_buffer {{},0,0};

////////////////////////////////////////////////////////////////////////////////
// Main - Called by the startup code.
int main(void) {
    ClockSetup();       // Setup System & Peripheral Clocks
    SerialSetup();      // Enable Serial Support - Currently USART1 Specific

    // USART1 Message to confirm program running - Using for Debugging
    uint8_t test_message[] = "Waitong!\n\r"; //Size 10, escape chars 1 byte each
    // Send a message to the terminal to indicate program is running.
    LoadBuffer(&serial_tx_buffer, test_message, 10);
    SerialBufferSend(&serial_tx_buffer);

    while(1){
    delay(1000);
    };
}


void I2CSetup() {
    // Ref: Datasheet DS5319 Section 5.3.16 I2C Interface Characteristics

    // ABP1 Bus Speed should be 36MHz
    // I2C1 SDA - PB9
    // I2C1 SCL - PB8

    // Enable I2C1 Peripheral Clock
    RCC->APB1ENR |= 0x00200000;

    // Configure Ports
    // Enable Port B Clock & Alternate Function Clock
    RCC->APB2ENR |= 0x00000009;

    // Remap I2C1 to use PB8,PB9
    AFIO->MAPR |= 0x00000002;


    // Setup Ports
    // Max Speed set to 2MHz as I2C Fast Mode is 400kHz Max.

    // PORT B9 - SDA
    GPIOB->CRH &= ~0x000000F0; // Reset PB 9 bits before setting on next line
    GPIOB->CRH |= 0x000000E0;  // AF Open Drain, Max 2MHz
    // PORT B8 - SCL
    GPIOB->CRH &= ~0x0000000F; // Reset PB 8 bits before setting on next line
    GPIOB->CRH |= 0x0000000E;  // AF Open Drain, Max 2MHz






    // I2C Master Mode
    // Setup Timings I2C_CR2 based on 36MHz Peripheral Clock
    I2C1->CR2 = 0x0024;

    // Config Clock Control Reg
    // Ref: Datasheet Table 41 SCL Frequency
    //  400kHz = 0x801E - Fast Mode
    //  100kHz = 0x00B4 - Standard Mode
    I2C1->CCR = 0x00B4;

    // Setup Rise Time
    // Fast Mode     - TRISE = 11 = 0x000B
    // Standard Mode - TRISE = 37 = 0x0025
    I2C1->TRISE = 0x0025;

    // Program I2C_CR1 to enable peripheral
    // Only enable after all setup operations complete.
    I2C1->CR1 |= 0x0001;


    // Move outside of Setup
    // Set START bit in I2C_CR1 to generate start condition
}

void I2CRead(uint8_t NumberBytesToRead, uint8_t slaveAddress)
{
    uint16_t flag_reset = 0;
    uint8_t Buffer[255]; // Define Buffer
    uint8_t *pBuffer = Buffer; // Pointer to Buffer start location

    // TODO: Add a check to see if NumberBytesToRead > Buffer to prevent overflow.

    if (NumberBytesToRead == 1){

        // Start
        // Send Slave Addr

        I2C1->CR1 &= ~(0x0400); // Clear ACK Flag
        __disable_irq();

        // Clear Addr Flag
        while(!(I2C1->SR1 & 0X0002)); // Read SR1 & Check for Addr Flag
        flag_reset = I2C1->SR2; // Read SR2 to complete Addr Flag reset.

        I2C1->CR1 |= 0x0200; // Set Stop Flag
        __enable_irq();

        while(!(I2C1->SR1 & 0x0040)); // Wait for RxNE
        *pBuffer = I2C1->DR; // Read Byte into Buffer
        pBuffer++; // Increment Buffer Pointer

        while (I2C1->CR1 & 0x0200); // Wait until STOP Flag cleared by HW
        I2C1->CR1 |= (0x0400); // Set ACK



    } else if (NumberBytesToRead == 2) {
        // Start
        // Send Slave Addr

        I2C1->CR1 |= 0x0800; // Set POS Flag
        __disable_irq();

        // Clear Addr Flag
        while(!(I2C1->SR1 & 0X0002)); // Read SR1 & Check for Addr Flag
        flag_reset = I2C1->SR2; // Read SR2 to complete Addr Flag reset.


        I2C1->CR1 &= ~(0x0400); // Clear ACK Flag
        __enable_irq();

        while(!(I2C1->SR1 & 0x0004)); // Wait BTF Flag (Byte Transfer Finished)
        __disable_irq();

        I2C1->CR1 |= 0x0200; // Set Stop Flag

        // Read 1st Byte
        *pBuffer = I2C1->DR; // Read 1st Byte into Buffer
        pBuffer++; // Increment Buffer Pointer

        __enable_irq();

        *pBuffer = I2C1->DR; // Read 2nd Byte into Buffer
        pBuffer++; // Increment Buffer Pointer

        while (I2C1->CR1 & 0x0200); // Wait until STOP Flag cleared by HW

        // Clear POS Flag, Set ACK Flag (to be ready to receive)
        I2C1->CR1 &= ~(0x0800); // Clear POS
        I2C1->CR1 |= (0x0400); // Set ACK


    } else {
    // Read 3+ Bytes
    }
}



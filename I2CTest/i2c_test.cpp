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
void I2CStart();
void I2CWriteMode(uint8_t SlaveAddr);
void I2CWriteData(uint8_t Data);
void I2CStop();
void I2CReadMode(uint8_t SlaveAddr);
void I2CReadData(uint8_t NumberBytesToRead, uint8_t slaveAddress);
void I2CRead(uint8_t NumberBytesToRead, uint8_t slaveAddress);
extern "C" void USART1_IRQHandler(void);
////////////////////////////////////////////////////////////////////////////////
// Buffers
// -------
// Create buffers - Size defined by buffer.h or variable for compiler.
volatile struct Buffer serial_tx_buffer {{},0,0};
volatile struct Buffer serial_rx_buffer {{},0,0};
volatile struct Buffer i2c_rx_buffer{{},0,0};

////////////////////////////////////////////////////////////////////////////////
// Main - Called by the startup code.
int main(void) {
    ClockSetup();       // Setup System & Peripheral Clocks
    SerialSetup();      // Enable Serial Support - Currently USART1 Specific
    I2CSetup();

    // USART1 Message to confirm program running - Using for Debugging
    uint8_t test_message[] = "Waiting!\n\r"; //Size 10, escape chars 1 byte each
    // Send a message to the terminal to indicate program is running.
    LoadBuffer(&serial_tx_buffer, test_message, 10);
    SerialBufferSend(&serial_tx_buffer);

    while(1){
    delay(1000);
    // Initially need a simple device to allow development of comms functions.
    // Using BH1750FVI Breakout board - Ambient Light Sensor
    I2CStart();
    I2CWriteMode(0x78); // Slave Address
    I2CWriteData(0x01); //BH1750FVI - Power On
    I2CStop();

    I2CStart();
    I2CWriteMode(0x78);
    I2CWriteData(0x20); // BH1750FVI One Time H-Res Mode.
    I2CStop();

    delay(1000); // Allow time for reading to be taken, auto power down.

    I2CReadData(2,0x78);    // Reads 2 Byte Measurement into i2c_rx_buffer

    SerialBufferSend(&i2c_rx_buffer); // Send measurement via serial.


    };
}


void I2CSetup() {
    // Ref: Datasheet DS5319 Section 5.3.16 I2C Interface Characteristics

    // ABP1 Bus Speed should be 36MHz
    // I2C1 SDA - PB9
    // I2C1 SCL - PB8

    RCC->APB1ENR |= 0x00200000;    // Enable I2C1 Peripheral Clock

    // Configure Ports
    RCC->APB2ENR |= 0x00000009; // Enable Port B Clock & Alternate Function Clk
    AFIO->MAPR |= 0x00000002;   // Remap I2C1 to use PB8,PB9

    // Setup Ports - Max Speed set to 2MHz as I2C Fast Mode is 400kHz Max.
    // PORT B9 - SDA
    GPIOB->CRH &= ~0x000000F0; // Reset PB 9 bits before setting on next line
    GPIOB->CRH |= 0x000000E0;  // AF Open Drain, Max 2MHz
    // PORT B8 - SCL
    GPIOB->CRH &= ~0x0000000F; // Reset PB 8 bits before setting on next line
    GPIOB->CRH |= 0x0000000E;  // AF Open Drain, Max 2MHz

    // I2C Master Mode -
    I2C1->CR2 = 0x0024;        // Set Timings I2C_CR2 for 36MHz Peripheral Clk

    // Config Clock Control Reg
    //  Ref: Datasheet Table 41 SCL Frequency
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
}

void I2CStart()
{
    // Set Start Condition by setting START bit
    // Master/Slave bit set once Busy bit clear.

    I2C1->CR1 |= 0x0100;

    // Wait for MSL = 1 (& Busy = 0)
    while(!(I2C1->SR2 & 0x0001));
}

void I2CWriteMode(uint8_t SlaveAddr) // TODO: Combine with I2CReadMode as almost identicle functions
{   // 7bit Addressing Mode

    while(!(I2C1->SR1 & 0x0001));   // Wait for start bit to be set
    SlaveAddr &= 0xFE;              // Clear Slave Addr LSB for write mode
    I2C1->DR = SlaveAddr;           // Write SlaveAddr to Data Register

    // Read of SR1 and write to DR should have reset the
    // start bit in SR1

    // Wait for confirmation that addr has been sent.
    // Check ADDR bit in I2C1->SR
    while(!(I2C1->SR1 & 0x0002));   // Read SR1

    // Addr bit now needs to be reset. Read SR1 (above) then SR2
    uint16_t flagreset = I2C1->SR2; // Read SR2
    // Write Mode Enabled. Send Data using I2CWriteData()
}

void I2CWriteData(uint8_t Data)
{
    // Write Data Byte to established I2C Connection
    // Load Data Register with byte to send.
    I2C1->DR = Data;
    // Wait for Byte Transfer Finished (BTF) flag in I2C1->SR1
    while(!(I2C1->SR1 & 0x0004));
}

void I2CStop()
{
    // End I2C Communication Session

    // To close the connection a STOP condition is generated by setting the STOP bit.
    I2C1->CR1 |= 0x0200; // Set STOP bit

    // Check STOP bit has been cleared indicating STOP condition detected.
    while(!(I2C-> & 0x0200));

    // Communication Ended, I2C interface in slave mode.
}

void I2CReadMode(uint8_t SlaveAddr)  // TODO: Combine with I2CWriteMode as almost identicle functions
{   // 7bit Addressing Mode

    while(!(I2C1->SR1 & 0x0001));   // Wait for start bit to be set
    SlaveAddr |= 0x0001;            // Set Slave Addr LSB to indicate read mode
    I2C1->DR = SlaveAddr;           // Write SlaveAddr to Data Register

    // Read of SR1 and write to DR should have reset the
    // start bit in SR1

    // Wait for confirmation that addr has been sent.
    // Check ADDR bit in I2C1->SR
    while(!(I2C1->SR1 & 0x0002)); // Read SR1

    // Addr bit now needs to be reset. Read SR1 (above) then SR2
    uint16_t flagreset = I2C1->SR2; // Read SR2
    // Read Mode Enabled, Receive Data using I2CReadData()
}

void I2CReadData(uint8_t NumberBytesToRead, uint8_t slaveAddress)
{
    // Dev Note:
    // There are a number of calls to enable/disable interrupts in this section.
    // This is related to limitations in the silicon errata sheet.
    // ES096 - STM32F10xx8 (Rev12)  Which covers F10xx8/B devices.
    // Details: Section 2.13.2, Workaround 2.

    uint16_t flag_reset = 0;

    // TODO: Add a check to see if NumberBytesToRead > Buffer to prevent overflow.
    // NOTE: The buffer size is set globally for all buffers.

    // Implementation Based on Application Note AN2824

    I2CStart();                         // Start
    I2CReadMode(slaveAddress);          // Send Slave Addr

    if (NumberBytesToRead == 1){

        I2C1->CR1 &= ~(0x0400);         // Clear ACK Flag
        __disable_irq();                // Disable Interrupts

        // Clear Addr Flag
        while(!(I2C1->SR1 & 0X0002));   // Read SR1 & Check for Addr Flag
        flag_reset = I2C1->SR2;         // Read SR2 to complete Addr Flag reset.

        I2C1->CR1 |= 0x0200;            // Set Stop Flag
        __enable_irq();                 // Enable Interrupts

        while(!(I2C1->SR1 & 0x0040));   // Wait for RxNE Set

        bufferWrite(&i2c_rx_buffer, I2C1->DR); // Read Byte into Buffer

        while (I2C1->CR1 & 0x0200);     // Wait until STOP Flag cleared by HW
        I2C1->CR1 |= (0x0400);          // Set ACK

    } else if (NumberBytesToRead == 2) {

        I2C1->CR1 |= 0x0800;            // Set POS Flag
        __disable_irq();                // Disable Interrupts

                                        // Clear Addr Flag
        while(!(I2C1->SR1 & 0X0002));   // Read SR1 & Check for Addr Flag
        flag_reset = I2C1->SR2;         // Read SR2 to complete Addr Flag reset.

        I2C1->CR1 &= ~(0x0400);         // Clear ACK Flag
        __enable_irq();                 // Enable Interrupts

        while(!(I2C1->SR1 & 0x0004));   // Wait BTF Flag (Byte Transfer Finish)
        __disable_irq();                // Disable Interrupts
        I2C1->CR1 |= 0x0200;            // Set Stop Flag
        bufferWrite(&i2c_rx_buffer, I2C1->DR); // Read 1st Byte into Buffer
        __enable_irq();                 // Enable Interrupts
        bufferWrite(&i2c_rx_buffer, I2C1->DR); // Read 2nd Byte into Buffer

        while (I2C1->CR1 & 0x0200);     // Wait until STOP Flag cleared by HW

        // Clear POS Flag, Set ACK Flag (to be ready to receive)
        I2C1->CR1 &= ~(0x0800);         // Clear POS
        I2C1->CR1 |= (0x0400);          // Set ACK


    } else {    // Read 3+ Bytes

        while (NumberBytesToRead > 3){
            // Wait until BTF = 1
            while(!(I2C1->SR1 & 0x0004));   // Wait BTF Flag
            // Read Data in Data Register
                bufferWrite(&i2c_rx_buffer, I2C1->DR); // Read Byte into Buffer
            // Decrement NumberBytesToRead
            NumberBytesToRead = NumberBytesToRead - 1;
        }

        // At this point there are 3 bytes left to read.

        // Dev Note:
        // ---------
        // The sequence is detailed in RM0008 (Rev16)- Fig 274 and in
        // AN2824 (Rev4) - Fig 1. There is an error in the flowchart in Fig 1;
        // it doesnt read N-2 as the sequence EV7_2 in Fig 274 outlines.
        // The correct sequence is below.

        while(!(I2C1->SR1 & 0x0004));           // Wait until BTF Flag = 1
        I2C1->CR1 &= ~(0x0400);                 // Clear ACK Flag
        __disable_irq();                        // Disable Interrupts

        // Missing read from AN2824 (Rev4) Fig 1
        bufferWrite(&i2c_rx_buffer, I2C1->DR);  // Read Byte N-2 into Buffer

        I2C1->CR1 |= 0x0200;                    // Set Stop Flag
        bufferWrite(&i2c_rx_buffer, I2C1->DR);  // Read Byte N-1 into Buffer
        __enable_irq();                         // Enable Interrupts
        while(!(I2C1->SR1 & 0x0040));           // Wait for RxNE Set
        bufferWrite(&i2c_rx_buffer, I2C1->DR);  // Read Byte N into Buffer
        while (I2C1->CR1 & 0x0200);             // Wait for STOP Flag HW clear
        I2C1->CR1 |= (0x0400);                  // Set ACK
    }

    I2CStop();
}



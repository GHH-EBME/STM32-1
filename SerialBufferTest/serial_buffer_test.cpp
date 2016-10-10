// STM32F103 Serial Setup & Test with Buffer
////////////////////////////////////////////////////////////////////////////////
#include "clock.h"      // Setup system and peripheral clocks
#include "buffer.h"
#include "serial.h"
#include <stdint.h>                     // uint8_t
#include "stm32f103xb.h"
////////////////////////////////////////////////////////////////////////////////
// Function Declarations
// ---------------------
// http://stackoverflow.com/questions/12543076/usart-receive-interrupt-stm32
extern "C" void USART1_IRQHandler(void); // Solves Interrupt Problem
//void ClockSetup();  // Setup the system clock sources
//void SerialSetup(); // Setup USART1
void delay(int);    // Simple Delay
void SerialReceiveEcho(); // Serial Test Function - Echos Received Data
volatile uint8_t flag_overrun = 0; // Monitors SerialRx Overrun

////////////////////////////////////////////////////////////////////////////////
// Buffers
// -------
// Create buffers - Size defined by buffer.h or variable for compiler.
volatile struct Buffer serial_tx_buffer {{},0,0};
volatile struct Buffer serial_rx_buffer {{},0,0};

////////////////////////////////////////////////////////////////////////////////
// Main - This function is called by the startup code.
// ---------------------------------------------------
int main() {
    ClockSetup();
    SerialSetup();

    // Strings
    uint8_t test_message[] = "Waiting!\n\r"; //Size 10, escape chars 1 byte each
    uint8_t bufferStatus = 1; // Start as 1 indicating empty buffer.
    uint8_t tmp_byte = 0x00;

    LoadBuffer(&serial_tx_buffer, test_message, 10);
    SerialBufferSend(&serial_tx_buffer);

    __enable_irq();  // Is this needed??

    while(1){ // Required to prevent SIGTRAP - Infinite loop.
        // Obtain status of rx buffer, Status 0 = holds data, 1 = empty
        bufferStatus = bufferPeek(&serial_rx_buffer,&tmp_byte);
        if (bufferStatus == 0){                     // If data in buffer
            SerialBufferSend(&serial_rx_buffer);    // Transmit buffer content
        }
    }
}




void delay(int count){
    // volatile so that the compiler doesn't optimise it out
    volatile int i;

    for (i = 0; i < count; i++)
    {
    }
}

void USART1_IRQHandler(void){

    // USART IRQ Triggered
    //  Read USART_SR & determine RXNE or ORE
    //  If overrun, increment flag_overrun, store the data to buffer and return.
    //  If no overrun read the received data to the buffer and return

    //if (USART1->SR & 0x00000008){  // Read Status register
    //    // If Overrun detected increment flag (ORE Set)
    //    flag_overrun = flag_overrun + 1;
    //}

    // Store byte in buffer
    if (USART1->SR & 0x00000020){ // Read Status register
        // If Read Data Register not empty (RXNE Set)
        uint8_t data = (uint8_t)(USART1->DR & 0xFF); // Read Data resgister, Reads lowest 8bits of 32.
        bufferWrite(&serial_rx_buffer, data);
    }
}





# Build note

The refactored source uses the standard TI `msp430.h` register names for the MSP430FR2355.
The timer and UART constants assume the original laboratory's 24 MHz SMCLK configuration.
Copy the exact clock-system initialization from the original course starter code into `main.c`
if the project does not already configure SMCLK to 24 MHz before these modules initialize.

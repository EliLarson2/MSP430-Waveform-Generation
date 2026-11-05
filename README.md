# MSP430 Mixed-Signal Embedded Controller

A mixed-signal embedded system built around the **Texas Instruments MSP430FR2355**, combining UART communication, ADC acquisition, timer-driven PWM, interrupt-based firmware, an internal DAC and op-amp, and numerically controlled waveform generation.

This project started as embedded-systems coursework, but I later revisited the firmware and reorganized it into separate modules for communication, sensing, timing, actuation, and analog output.

What I like most about this project is that it connects software all the way to the physical hardware. A command sent over UART can trigger an ADC measurement, change a PWM output, or generate an analog waveform through the MSP430's internal DAC and op-amp. I also used an oscilloscope to measure execution time and analog behavior rather than relying only on what the firmware was supposed to be doing.

That combination of **sensor input, actuator output, deterministic timing, communication, and hardware validation** is the part of embedded development I want to continue applying to robotics, UAVs, and control systems.

> **Note:** The timer and UART calculations in this project assume the original laboratory configuration of a 24 MHz SMCLK. The clock-configuration code itself is not included in the current `src/` files.

---

## Firmware Architecture

The current firmware is organized by peripheral and responsibility rather than keeping the entire application in one source file.

| Module | Responsibility |
| --- | --- |
| [`main.c`](src/main.c) | System initialization and application loop |
| [`uart.c`](src/uart.c) | UART reception, command processing, and responses |
| [`adc.c`](src/adc.c) | 12-bit ADC configuration and acquisition |
| [`pwm.c`](src/pwm.c) | Timer-generated PWM output |
| [`nco.c`](src/nco.c) | Phase accumulator and sine-wave generation |
| [`sac.c`](src/sac.c) | Internal DAC and op-amp control |
| [`timer.c`](src/timer.c) | Periodic timing and waveform updates |

`main.c` initializes the hardware modules, enables interrupts, and then spends most of its time waiting for completed UART commands:

```c
while (1)
{
    if (uart_command_ready())
    {
        uart_process_command();
    }
}

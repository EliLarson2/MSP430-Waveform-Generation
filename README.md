# MSP430 Mixed-Signal Waveform Generator

A mixed-signal embedded system built around the **Texas Instruments MSP430FR2355**, integrating UART communication, ADC acquisition, PWM motor control, hardware timers, interrupt-driven firmware, an internal 12-bit DAC and operational amplifier, and numerically controlled sine-wave generation.

The project began as a sequence of embedded-systems laboratory exercises and culminated in a UART-controlled application capable of interacting with multiple digital and analog peripherals.

The final system connects several layers of embedded engineering:

```text
Serial Command
      |
      v
Embedded Firmware
      |
      +----------+----------+----------+
      |          |          |          |
      v          v          v          v
     ADC       GPIO        PWM        NCO
                           Motor       |
                                      v
                              Phase Accumulator
                                      |
                                      v
                               Sine Lookup Table
                                      |
                                      v
                                  12-bit DAC
                                      |
                                      v
                               Internal Op-Amp
                                      |
                                      v
                                Analog Output
                                      |
                                      v
                                 Oscilloscope
```

---

## Project Goals

The primary goal of this project was to understand how peripherals inside a microcontroller can be combined into a complete embedded mixed-signal system.

The project focused on:

- Register-level MSP430 peripheral configuration
- Interrupt-driven UART communication
- ADC configuration and acquisition
- Hardware timers
- PWM generation and motor control
- Internal DAC operation
- Internal operational-amplifier configuration
- Oscilloscope-based hardware verification
- Numerically Controlled Oscillators
- Lookup-table-based waveform generation
- Integrating several peripherals through a common command interface

A major objective was to verify firmware behavior at the **physical electrical interface**, rather than assuming correct code automatically meant correct hardware behavior.

---

## Hardware Platform

| Category | Implementation |
|---|---|
| Microcontroller | Texas Instruments MSP430FR2355 |
| Programming Language | Embedded C |
| System Clock | 24 MHz |
| Communication | UART at 57,600 baud |
| ADC | Internal 12-bit ADC |
| DAC | Internal 12-bit DAC |
| Analog | Smart Analog Combo / internal op-amp |
| Waveform Generation | 32-bit NCO |

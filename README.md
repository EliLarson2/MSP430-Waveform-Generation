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
| Sine Lookup Table | 256 signed samples |
| NCO Update Rate | 1 kHz |
| Motor Control | Timer-generated PWM |
| Measurement | Digital oscilloscope |

---

## Repository Architecture

The repository contains both the original academic implementation and a later architectural refactor.

```text
MSP430-Waveform-Generation/
│
├── README.md
│
├── src/
│   ├── main.c
│   ├── uart.c
│   ├── uart.h
│   ├── adc.c
│   ├── adc.h
│   ├── pwm.c
│   ├── pwm.h
│   ├── nco.c
│   ├── nco.h
│   ├── sac.c
│   ├── sac.h
│   ├── timer.c
│   └── timer.h
│
├── legacy/
│   └── original_lab_main.c
│
├── Images/
│   ├── sac_high_speed_slew_rate.png
│   ├── sac_low_speed_slew_rate.png
│   └── sine_computation_timing.png
│
└── docs/
    └── Lab_5_SAC.pdf
```

### Original vs. Refactored Implementation

`legacy/original_lab_main.c` preserves the original academic implementation.

The code under `src/` is a later refactor that separates the project into functional modules.

| Module | Responsibility |
|---|---|
| `main.c` | System initialization and application loop |
| `uart.c/.h` | UART reception and command processing |
| `adc.c/.h` | ADC configuration and analog acquisition |
| `pwm.c/.h` | PWM generation and motor control |
| `nco.c/.h` | Phase accumulator and sine-wave generation |
| `sac.c/.h` | Internal DAC and op-amp control |
| `timer.c/.h` | Deterministic timing and waveform updates |

The objective of the refactor was to turn:

```text
One Large Application
        |
        v
Direct Peripheral Access Everywhere
```

into:

```text
Application Logic
        |
        v
Peripheral Interfaces
        |
        v
MCU Hardware
```

---

# Smart Analog Combo

The MSP430FR2355 includes configurable analog peripherals known as **Smart Analog Combos (SACs)**.

For this project, SAC3 combines:

- Internal 12-bit DAC
- Internal operational amplifier
- Voltage-follower operation
- Externally accessible analog output

Conceptually:

```text
Digital DAC Value
       |
       v
+--------------+
| 12-Bit DAC   |
+--------------+
       |
       v
+--------------+
| Internal     |
| Op-Amp       |
| Buffer       |
+--------------+
       |
       v
     OA3O
       |
       v
Oscilloscope
```

The DAC creates the requested voltage while the internal op-amp buffers the signal before it reaches the external output.

The refactored firmware abstracts direct DAC-register access:

```c
void sac_set_dac(uint16_t value)
{
    if (value > 4095U)
    {
        value = 4095U;
    }

    SAC3DAT = value;
}
```

Other modules can therefore use:

```c
sac_set_dac(dac_sample);
```

without needing to know how the SAC peripheral is implemented internally.

---

# SAC Slew-Rate Characterization

Before implementing waveform generation, I characterized the analog response of the internal DAC and op-amp.

The DAC was repeatedly switched between minimum and maximum codes:

```c
SAC3DAT = 0;

/* delay */

SAC3DAT = 4095;
```

The transition was measured using an oscilloscope under different SAC operating modes.

## Measured Results

| SAC Mode | Measured Slew Rate |
|---|---:|
| Low-Speed Mode | Approximately 0.33 V/µs |
| High-Speed Mode | Approximately 1.65 V/µs |

### High-Speed Mode

Images/sac_high_speed_slew_rate.png

### Low-Speed Mode

Images/sac_low_speed_slew_rate.png

The high-speed configuration produced a substantially faster transition than the low-speed configuration.

The experiment also demonstrated that configured peripheral behavior should be **physically measured**, not inferred solely from firmware.

Factors such as:

- Probe capacitance
- Output loading
- Peripheral configuration
- Measurement technique
- Analog signal-path behavior

can influence what appears at the real hardware interface.

---

# Measuring Floating-Point Sine Execution Time

Before implementing the NCO, I investigated the computational cost of directly calculating sine values.

A GPIO pin was asserted immediately before calling `sin()` and cleared immediately afterward:

```c
P1OUT |= BIT1;

sin(2);

P1OUT &= ~BIT1;
```

The GPIO pulse width was then measured with an oscilloscope.

This converts software execution time into an electrical signal that can be directly measured.

## Results

| Measurement | Result |
|---|---:|
| Observed computation range | Approximately 0.6 ms to 2.1 ms |
| Representative measurement | Approximately 1.89 ms |
| System clock | 24 MHz |
| Approximate cycles at 1.89 ms | 45,360 cycles |

Images/sine_computation_timing.png

Repeatedly evaluating a floating-point trigonometric function would therefore consume a significant amount of processor time in a time-sensitive waveform generator.

That observation motivated the NCO architecture.

---

# Numerically Controlled Oscillator

The waveform generator uses a **Numerically Controlled Oscillator**, or NCO.

The implementation contains:

- 32-bit phase accumulator
- Frequency-dependent phase increment
- 256-entry sine lookup table
- Fixed 1 kHz update rate
- 12-bit DAC output

```text
Requested Frequency
        |
        v
Phase Increment
        |
        v
+-------------------+
| 32-Bit Phase      |
| Accumulator       |
+-------------------+
        |
        | Upper 8 bits
        v
+-------------------+
| 256-Entry         |
| Sine Lookup Table |
+-------------------+
        |
        v
Signed 16-Bit Sample
        |
        v
Scale to 12 Bits
        |
        v
Internal DAC
        |
        v
Internal Op-Amp
        |
        v
Analog Sine Wave
```

---

## Frequency Control

The requested output frequency determines the amount added to the phase accumulator during every update:

```c
static uint32_t phase = 0;
static uint32_t delta = 0;

void nco_set_frequency(float frequency_hz)
{
    const float scaling_factor = 4294967.296f;

    delta =
        (uint32_t)((scaling_factor * frequency_hz)
        + 0.5f);
}
```

The scaling factor corresponds to:

```text
2^32 / 1000
```

because the NCO is updated at:

```text
1000 samples/second
```

The phase increment therefore determines how quickly the accumulator progresses through one conceptual waveform cycle.

---

# Sine Lookup Table

Instead of calculating `sin()` during each timer interrupt, one sinusoidal cycle is stored in a **256-entry lookup table**.

A simplified portion looks like:

```c
static const int16_t sinLUT[256] =
{
       0,
     804,
    1608,
    2410,

    /* ... */

   32767,

    /* ... */

       0,

    /* ... */

  -32767

    /* ... */
};
```

The NCO function is intentionally simple:

```c
int16_t nco_run(void)
{
    phase += delta;

    return sinLUT[phase >> 24];
}
```

The table contains:

```text
256 = 2^8
```

samples.

The upper eight bits of the 32-bit accumulator are therefore used as the lookup-table index.

---

## Why Use an NCO?

Direct sine calculation requires considerably more computation.

The NCO replaces repeated trigonometric evaluation with:

```text
Integer Addition
      +
Bit Shift
      +
Lookup Table Access
```

This makes waveform generation significantly more deterministic.

The same basic architecture appears in more advanced systems involving:

- Digital signal processing
- Frequency synthesis
- Digital communications
- FPGA DSP
- Digital mixers
- Software-defined radio

---

# Timer-Driven Waveform Generation

The NCO is updated using a **1 kHz Timer B0 interrupt**.

The waveform-generation path is:

```text
Timer Interrupt
      |
      v
nco_run()
      |
      v
Signed 16-Bit Sample
      |
      v
Shift to Unsigned Range
      |
      v
Reduce to 12 Bits
      |
      v
DAC
```

The relevant implementation is:

```c
#pragma vector=TIMER0_B0_VECTOR
__interrupt void TimerB0_ISR(void)
{
    int32_t signed_sample;
    uint16_t dac_sample;

    if (g_nco_enabled)
    {
        signed_sample = nco_run();

        dac_sample =
            (uint16_t)(signed_sample + 32768L);

        dac_sample >>= 4;

        sac_set_dac(dac_sample);
    }
}
```

The lookup table produces approximately:

```text
-32767 ... +32767
```

Adding:

```c
32768
```

moves the waveform into an unsigned 16-bit range.

Then:

```c
dac_sample >>= 4;
```

maps the value into the 12-bit range required by the DAC.

---

# Generated Waveforms

The original system supports three sinusoidal frequencies.

| UART Command | Analog Output |
|---|---|
| `D1` | 40 Hz sine wave |
| `D2` | 80 Hz sine wave |
| `D3` | 100 Hz sine wave |

Additional commands directly control the DAC:

| Command | Analog Output |
|---|---|
| `D0` | NCO disabled, DAC = 0 V |
| `D4` | NCO disabled, DAC ≈ 3.3 V |

---

# UART Interface

The final embedded application uses **eUSCI_A1 UART** at **57,600 baud**.

UART provides one interface through which multiple peripherals can be controlled:

```text
Computer / Terminal
        |
       UART
        |
        v
+-------------------+
| Command Interface |
+-------------------+
    |    |    |    |
    v    v    v    v
  GPIO  ADC  PWM   NCO
```

The original implementation performed command decoding directly inside the UART ISR.

The later refactor separates character reception from command execution.

### Simplified UART Receive ISR

```c
#pragma vector=EUSCI_A1_VECTOR
__interrupt void UART_ISR(void)
{
    uint8_t received_char = UCA1RXBUF;

    if (command_index < UART_COMMAND_LENGTH)
    {
        command_buffer[command_index++] =
            (char)received_char;
    }

    if (command_index >= UART_COMMAND_LENGTH)
    {
        command_buffer[command_index] = '\0';

        command_ready = 1U;
        command_index = 0U;
    }
}
```

The ISR handles communication timing.

The main application performs the slower command interpretation.

This separates:

```text
Interrupt Context
      |
Receive / Buffer
      |
      v
Normal Application Context
      |
Decode / Execute
```

---

# UART Command Set

The final application supports 17 commands.

| Command | Description |
|---|---|
| `R0` | Turn red LED off |
| `R1` | Turn red LED on |
| `G0` | Turn green LED off |
| `G1` | Turn green LED on |
| `A1` | Measure ADC channel A1 |
| `A2` | Measure ADC channel A2 |
| `A3` | Measure ADC channel A3 |
| `A4` | Measure ADC channel A4 |
| `A5` | Measure ADC channel A5 |
| `M0` | Stop motor |
| `M1` | Rotate motor clockwise |
| `M2` | Rotate motor counter-clockwise |
| `D0` | Set analog output to 0 V |
| `D1` | Generate 40 Hz sine wave |
| `D2` | Generate 80 Hz sine wave |
| `D3` | Generate 100 Hz sine wave |
| `D4` | Set analog output to approximately 3.3 V |

---

# Example: From UART Command to Analog Waveform

Sending:

```text
D2
```

results in:

```text
Serial Terminal
      |
      | "D2"
      v
UART Peripheral
      |
      v
UART RX Interrupt
      |
      v
Command Buffer
      |
      v
Command Parser
      |
      v
nco_set_frequency(80)
      |
      v
New Phase Increment
      |
      v
1 kHz Timer Interrupt
      |
      v
nco_run()
      |
      v
Sine Lookup Table
      |
      v
12-Bit DAC Value
      |
      v
Internal DAC
      |
      v
Internal Op-Amp
      |
      v
80 Hz Analog Output
      |
      v
Oscilloscope
```

This is one of the parts of the project I found most valuable because a two-character command ultimately generates a **measurable analog signal** through several layers of hardware and firmware.

---

# ADC Integration

The same serial interface controls the MSP430's **12-bit ADC**.

The refactored implementation provides one function for channels A1 through A5:

```c
uint16_t adc_read_channel(uint8_t channel)
{
    if (channel < 1U || channel > 5U)
    {
        return 0U;
    }

    ADCCTL0 &= ~ADCENC;

    ADCMCTL0 &= ~ADCINCH_15;
    ADCMCTL0 |= channel;

    ADCCTL0 |= ADCENC;

    return adc_get_reading();
}
```

The original implementation contained similar configuration blocks for every ADC command.

The refactor reduces:

```text
A1 implementation
A2 implementation
A3 implementation
A4 implementation
A5 implementation
```

to:

```c
adc_read_channel(channel);
```

The measured conversion result is then transmitted back through UART.

---

# PWM Motor Control

The application also integrates **Timer B1 PWM generation** for motor control.

The PWM period is approximately:

```text
20 ms
```

and motor state is controlled using the capture/compare register.

```c
void motor_stop(void)
{
    TB1CCR1 = 0U;
}

void motor_clockwise(void)
{
    TB1CCR1 = 525U;
}

void motor_counterclockwise(void)
{
    TB1CCR1 = 750U;
}
```

UART commands therefore connect directly to actuator behavior:

```text
M0 -> Stop
M1 -> Clockwise
M2 -> Counter-Clockwise
```

This portion of the project later helped reinforce my understanding of PWM-based control in other embedded and avionics applications.

---

# Software Architecture

The final refactor separates responsibilities across modules.

```text
                    +------------------+
                    |      main.c      |
                    |   Application    |
                    +--------+---------+
                             |
         +-------------------+-------------------+
         |          |           |       |       |
         v          v           v       v       v
       UART        ADC         PWM     NCO     SAC
         |                               |
         v                               v
      RX ISR                         Timer ISR
```

## Timer ISR

Responsible for:

- Fixed update timing
- NCO waveform generation
- DAC updates
- Periodic LED timing

## UART ISR

Responsible for:

- Receiving serial data
- Buffering commands
- Signaling when a command is ready

## Main Application

Responsible for:

- Command interpretation
- ADC acquisition requests
- Motor commands
- NCO frequency changes
- GPIO output control

---

# Why I Refactored the Project

The original academic implementation successfully demonstrated the required functionality but placed much of the application behavior inside a single firmware file and UART ISR.

That was appropriate for quickly demonstrating laboratory requirements.

After gaining additional embedded and hardware-development experience, I revisited the software and reorganized it by responsibility.

The original implementation remains available in:

```text
legacy/original_lab_main.c
```

The refactored version is located in:

```text
src/
```

This preserves the actual coursework while also demonstrating how I would structure the system today.

### Original

```text
UART ISR
   |
   +--> LED Control
   +--> ADC Control
   +--> PWM Control
   +--> NCO Control
   +--> DAC Control
```

### Refactored

```text
UART ISR
   |
   v
Receive Buffer
   |
   v
Command Handler
   |
   +--> adc.c
   +--> pwm.c
   +--> nco.c
   +--> sac.c
```

Benefits include:

- Smaller interrupt handlers
- Better separation of responsibilities
- Reusable peripheral interfaces
- Easier debugging
- Better readability
- Easier maintenance
- More scalable firmware architecture

---

# What I Learned

This project connected several subjects that can otherwise feel like isolated microcontroller peripherals.

### Embedded Firmware

- Embedded C
- Register-level programming
- Interrupt Service Routines
- UART
- Serial command parsing
- Peripheral abstraction

### Digital Systems

- Hardware timers
- PWM
- Phase accumulation
- Lookup tables
- Deterministic update timing

### Analog Electronics

- ADC conversion
- DAC output
- Internal operational amplifiers
- Voltage buffering
- Slew rate
- Mixed-signal interfaces

### Signal Processing

- Numerically Controlled Oscillators
- Phase increments
- Lookup-table synthesis
- Sampling/update rates
- Waveform generation

### Hardware Validation

- Oscilloscope operation
- Execution-time measurement
- Signal measurement
- Comparing expected and measured behavior

The biggest takeaway was understanding the full path:

```text
Software Command
       |
       v
Digital Logic
       |
       v
MCU Peripheral
       |
       v
Analog Electronics
       |
       v
Physical Signal
       |
       v
Measurement
```

---

# Future Improvements

## Arbitrary Output Frequencies

Instead of supporting only 40, 80, and 100 Hz, the command interface could accept a requested frequency directly.

Example:

```text
F125
```

could request:

```text
125 Hz
```

and automatically calculate the required phase increment.

---

## Spectral Characterization

A future revision could use FFT analysis or a spectrum analyzer to measure:

- Fundamental frequency
- Harmonics
- Quantization artifacts
- LUT artifacts
- Noise floor
- Spurious output

---

## Lookup-Table Analysis

Different lookup table sizes could be compared.

| LUT Size | Tradeoff |
|---:|---|
| 64 | Low memory use, coarse waveform |
| 128 | Moderate memory use |
| 256 | Current implementation |
| 512 | Greater resolution, higher memory use |

---

## FPGA Implementation

The NCO is particularly well suited for implementation in programmable logic.

A future VHDL version could use:

```text
Phase Accumulator
      |
      v
ROM-Based Sine LUT
      |
      v
Sample Stream
```

This would directly connect the MSP430 project with my FPGA/VHDL experience.

---

## RF / SDR Extension

The NCO and DSP concepts also provide a foundation for software-defined radio.

A future signal chain could become:

```text
NCO
 |
 v
Digital Mixer
 |
 v
Filter
 |
 v
Sample Stream
 |
 v
Spectrum / Demodulation
```

This project therefore serves as an early foundation for continued work in:

- Embedded systems
- Digital signal processing
- FPGA development
- RF systems
- Hardware/software integration

---

# Project Background

This project originated from embedded-systems laboratory work performed on the MSP430FR2355.

The laboratory sequence introduced:

- ADC operation
- UART communication
- Hardware timers
- PWM
- Motor control
- Smart Analog Combo configuration
- DAC generation
- Operational-amplifier characterization
- NCO waveform synthesis

Rather than presenting each exercise as an unrelated lab, this repository documents how those concepts combined into a **single embedded mixed-signal system** and how I later reorganized the original firmware into a cleaner architecture.

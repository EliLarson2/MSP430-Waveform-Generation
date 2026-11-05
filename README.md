# MSP430 Mixed-Signal Embedded Controller

A mixed-signal embedded system built around the **Texas Instruments MSP430FR2355**, combining UART communication, ADC acquisition, timer-driven PWM, interrupt-driven firmware, an internal DAC and op-amp, and numerically controlled waveform generation.

This project started as embedded-systems coursework, but I later revisited the firmware and reorganized it into separate modules for communication, sensing, timing, actuation, and analog output.

What I found most valuable was seeing how the different parts of an embedded system interact. A UART command can request an ADC measurement, change a PWM output, or start an analog waveform. Hardware timers handle timing-sensitive work, while the main application handles commands and higher-level behavior.

I also used an oscilloscope to verify what the hardware was actually doing. That combination of **firmware, sensors, actuators, timing, analog hardware, and physical measurement** is the kind of embedded work I want to continue applying to robotics, UAVs, and control systems.

> **Note:** The timer and UART calculations in this project assume the original laboratory configuration of a 24 MHz SMCLK. The clock-configuration code itself is not included in the current `src/` files.

---

## Firmware Architecture

The current firmware is organized by responsibility rather than keeping the entire application in one source file.

| Module | Responsibility |
| --- | --- |
| [`main.c`](src/main.c) | System initialization and application loop |
| [`uart.c`](src/uart.c) | UART reception, command processing, and responses |
| [`adc.c`](src/adc.c) | 12-bit ADC configuration and acquisition |
| [`pwm.c`](src/pwm.c) | Timer-generated PWM output |
| [`nco.c`](src/nco.c) | Phase accumulator and sine-wave generation |
| [`sac.c`](src/sac.c) | Internal DAC and op-amp control |
| [`timer.c`](src/timer.c) | Periodic timing and waveform updates |

`main.c` initializes the peripherals, enables interrupts, and then waits for completed UART commands:

```c
while (1)
{
    if (uart_command_ready())
    {
        uart_process_command();
    }
}
```

One of the main changes I made when reorganizing the project was keeping the UART receive interrupt small.

The ISR receives characters and builds a two-character command, but the actual command is processed later in the main application. ADC conversions, PWM changes, and waveform commands therefore do not execute directly inside the UART ISR.

Timer B0 has a different responsibility. It provides the fixed 1 ms timing used by the waveform generator, allowing DAC updates to occur independently of command processing in the main loop.

This separation between **time-sensitive interrupt work and application-level processing** was one of the more useful embedded-firmware lessons from the project.

---

## UART Command Interface

The application uses **eUSCI_A1 UART at 57,600 baud** and accepts simple two-character commands.

| Command | Action |
| --- | --- |
| `R0` / `R1` | Red LED off / on |
| `G0` / `G1` | Green LED off / on |
| `A1`–`A5` | Read ADC channel A1–A5 |
| `M0` | Stop PWM-controlled output |
| `M1` | Clockwise command |
| `M2` | Counterclockwise command |
| `D0` | Disable waveform generation and set DAC to zero |
| `D1` | Generate a 40 Hz sine wave |
| `D2` | Generate an 80 Hz sine wave |
| `D3` | Generate a 100 Hz sine wave |
| `D4` | Disable waveform generation and set DAC to full scale |

The interface is simple, but it gave me one way to interact with several different peripherals.

For example, `A3` selects ADC channel A3, performs a conversion, and returns the result over UART. `D2` changes the NCO phase increment so the timer-driven DAC output generates an 80 Hz waveform.

---

## ADC Acquisition

The ADC module configures the MSP430 for **12-bit, single-channel conversions** and supports analog inputs A1 through A5.

Instead of maintaining separate conversion code for every channel, the input is selected at runtime:

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

The conversion itself is currently blocking:

```c
ADCCTL0 |= ADCSC;

while (ADCCTL1 & ADCBUSY)
{
}

return ADCMEM0;
```

For a command-driven application this kept the implementation straightforward. It also gave me experience with a common embedded-system path: **acquire a physical measurement, handle it in firmware, and return the result through a communication interface.**

---

## PWM and Actuator Control

Timer B1 generates a PWM output on **P2.0 / TB1.1**.

Using the project's 24 MHz SMCLK assumption and the configured timer dividers, `TB1CCR0 = 7500` produces an approximately **20 ms period**.

The capture/compare value controls the output state:

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

The repository does not document the external motor hardware, so I treat this portion of the project primarily as a **timer-based actuator-control interface**.

The important part for me was learning to use the hardware timer to generate the output rather than trying to maintain timing manually in software. That same approach is useful in robotics and UAV systems where communication, sensing, and actuator updates all need to coexist.

---

## Numerically Controlled Waveform Generation

The most interesting part of the project for me was the waveform generator.

The MSP430 generates a sine wave using a **32-bit phase accumulator** and a **256-entry lookup table** rather than calculating `sin()` for every output sample.

The NCO maintains a phase and phase increment:

```c
static uint32_t phase = 0;
static uint32_t delta = 0;
```

The requested frequency determines how far the phase accumulator advances during each update:

```c
void nco_set_frequency(float frequency_hz)
{
    const float scaling_factor = 4294967.296f; /* 2^32 / 1000 Hz */

    delta =
        (uint32_t)(*scaling_factor * frequency_hz) + 0*5f);
}
```

Timer B0 updates the w*veform every **1 ms**, giving the *CO a 1 kHz update rate.

Generatin* the next sample is then relativel* simple:

```c
int16_t nco_run(voi*)
{
    phase += delta;
    return*sinLUT[phase >> 24];
}
```

Becaus* the lookup table contains 256 ent*ies, the upper eight bits of the 3*-bit phase accumulator can be used*directly as the table index.

The *imer ISR then converts the signed *ookup-table value into the range r*quired by the 12-bit DAC:

```c
si*ned_sample = nco_run();

dac_sampl* =
    (uint16_t)(signed_sample + *2768L);

dac_sample >>= 4;

sac_se*_dac(dac_sample);
```

This part o* the project connected timer inter*upts, fixed update rates, phase ac*umulation, lookup tables, integer *caling, and a physical analog outp*t.

---

## Measuring the Cost of *sin()`

Before using the lookup-ta*le approach, I measured how long a*direct floating-point `sin()` calc*lation took on the microcontroller*

I toggled a GPIO around the func*ion call:

```c
P1OUT |= BIT1;

si*(2);

P1OUT &= ~BIT1;
```

The res*lting GPIO pulse could then be mea*ured with an oscilloscope.

![Osci*loscope measurement of floating-po*nt sine execution time](./Images/sine_computation_timing.png)

*GPIO *iming measurement around a floatin*-point sine calculation.*

I obser*ed execution times ranging from ap*roximately **0.6 ms to 2.1 ms**, w*th a representative measurement ar*und **1.89 ms**.

At the 24 MHz la*oratory clock, 1.89 ms corresponds*to roughly **45,360 clock cycles***

That measurement helped motivate*the NCO implementation. Instead of*repeatedly evaluating a floating-p*int trigonometric function inside * time-sensitive update path, the f*nal implementation uses a phase ad*ition, bit shift, lookup-table acc*ss, and DAC update.

More importan*ly, this experiment reinforced a d*bugging technique I still use: **t*rn software behavior into somethin* measurable at a hardware interfac* whenever possible.**

---

## Int*rnal DAC and Op-Amp

The analog ou*put uses **SAC3**, one of the MSP4*0FR2355's Smart Analog Combo perip*erals.

In the current firmware, t*e internal 12-bit DAC drives the i*ternal op-amp configured as a volt*ge follower. The buffered output i* routed to **P3.5 / OA3O**.

The r*st of the application can write a *AC value without directly manipula*ing the SAC registers:

```c
void *ac_set_dac(uint16_t value)
{
    i* (value > 4095U)
    {
        val*e = 4095U;
    }

    SAC3DAT = va*ue;
}
```

The range check prevent* callers from writing beyond the 1*-bit DAC range.

This was one of m* first projects where firmware con*iguration directly affected an ana*og signal path inside the microcon*roller, which made oscilloscope ve*ification especially useful.

---
*## SAC Slew-Rate Measurement

I me*sured the response of the internal*DAC/op-amp path by switching the D*C between minimum and maximum code* and observing the transition on a* oscilloscope.

### High-Speed Mod*

![High-speed SAC slew-rate measu*ement](./Images/sac_high_speed_slew_rate.png)

*Measured high-speed S*C transition: approximately 1.65 V*µs.*

### Low-Speed Mode

![Low-sp*ed SAC slew-rate measurement](./Images/sac_low_speed_slew_rate.png)

*Measured low-speed SAC transition: approximately 0.33 V/µs.*

The high-speed configuration measured approximately **1.65 V/µs**, compared with approximately **0.33 V/µs** in low-speed mode.

The important lesson was that configuring a peripheral correctly in firmware does not automatically prove that the physical output behaves the way I expect. The oscilloscope gave me a way to compare the configured behavior with what was actually happening electrically.

---

## What I Would Improve Today

The current firmware is intentionally simple, but there are several things I would change if I expanded it into a larger control application.

The first would be the UART protocol. Two-character commands work well for a small system, but they do not scale very far. I would move toward commands with parameters and explicit responses so values such as waveform frequency, PWM output, or sampling configuration could be changed without adding another hard-coded command.

For example, instead of:

```text
D1 -> 40 Hz
D2 -> 80 Hz
D3 -> 100 Hz
```

I would rather support something closer to:

```text
FREQ 80
```

and calculate the NCO phase increment from the requested value.

I would also make ADC acquisition event-driven rather than polling `ADCBUSY`. The current blocking implementation is fine for an occasional UART-requested measurement, but a robotics or control application may need to sample sensors continuously while communication and actuator updates continue in parallel.

As the application grew, I would also separate the application state more clearly from the MSP430-specific register layer. The current modules already improve organization, but a larger system would benefit from interfaces that make the higher-level control logic less dependent on the specific microcontroller underneath it.

Finally, I would add better command validation, status responses, and test coverage around the command parser and NCO calculations.

---

## Why This Project Is in My Portfolio

This project gave me experience with more than configuring individual MSP430 peripherals. I had to think about **how the peripherals interact**.

UART receives commands. The ADC provides measurements. Hardware timers create deterministic events. PWM controls an external output. The NCO generates samples. The DAC converts those samples into an analog voltage, and the internal op-amp buffers that voltage before it reaches the physical pin.

Just as importantly, I measured the system with an oscilloscope instead of stopping once the firmware compiled and ran.

That is the part of embedded engineering I find most interesting: connecting software to real electrical behavior.

The specific MSP430 peripherals are only one implementation of that idea. The same architecture shows up in larger systems such as robotics and UAV avionics, where embedded firmware has to coordinate **sensors, communication, actuators, timing, and physical hardware**.

---

## Repository Structure

```text
.
├── README.md
├── Images/
│   ├── sac_high_speed_slew_rate.png
│   ├── sac_low_speed_slew_rate.png
│   └── sine_computation_timing.png
└── src/
    ├── README_BUILD_NOTE.md
    ├── adc.c
    ├── adc.h
    ├── main.c
    ├── nco.c
    ├── nco.h
    ├── pwm.c
    ├── pwm.h
    ├── sac.c
    ├── sac.h
    ├── timer.c
    ├── timer.h
    ├── uart.c
    └── uart.h
```

---

## Background

This project originated from embedded-systems laboratory work using the MSP430FR2355. I later revisited the firmware and reorganized the different exercises into the modular application shown in this repository.

That second pass was valuable because I could look at the project less as a set of lab requirements and more as an embedded system: **what belongs in an interrupt, what should happen in the main application, which tasks need hardware timing, how peripherals should be separated in software, and how the resulting behavior can be verified on real hardware.**

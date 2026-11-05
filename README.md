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
```

One design decision I made during the refactor was to keep the UART receive interrupt small. The ISR receives characters and builds a two-character command, but it does not perform ADC conversions, change motor outputs, or execute the rest of the application logic.

Once a complete command has been received, the main loop processes it outside interrupt context.

That separation became one of the more useful lessons from the project. Interrupts are valuable for events that need immediate attention, but that does not mean all of the work associated with an event belongs inside the ISR.

The timer interrupt has a different job. Timer B0 provides the fixed timing used by the waveform generator, so the DAC continues to update at a predictable rate regardless of what the main loop is doing.

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

The command interface is simple, but it gave me a useful way to exercise several peripherals through one application.

For example, sending `A3` selects ADC channel A3, performs a conversion, and returns the result over UART. Sending `D2` changes the NCO phase increment so the timer-driven DAC output generates an 80 Hz waveform.

That made the project feel much more like an embedded system than a collection of unrelated peripheral demonstrations.

---

## ADC Acquisition

The ADC module configures the MSP430 for **12-bit, single-channel conversions** and supports analog inputs A1 through A5.

Instead of maintaining separate conversion code for every channel, the channel is selected at runtime:

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

The conversion itself is currently blocking. `adc_get_reading()` starts the conversion, waits for the ADC to finish, and returns the value from `ADCMEM0`.

```c
ADCCTL0 |= ADCSC;

while (ADCCTL1 & ADCBUSY)
{
}

return ADCMEM0;
```

For this command-driven application, that kept the implementation straightforward. It also gave me experience with a pattern that shows up constantly in embedded control systems: **acquire a physical measurement, process or package it in firmware, and return it through a communication interface.**

---

## PWM and Actuator Control

Timer B1 generates a PWM output on **P2.0 / TB1.1**.

Using the project's 24 MHz SMCLK assumption and the configured timer dividers, `TB1CCR0 = 7500` produces an approximately **20 ms period**. The capture/compare value then controls the output state.

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

What mattered to me was learning to let the hardware timer generate the output rather than trying to create timing manually in software. That same idea carries directly into embedded control applications where communication, sensing, and actuator timing all need to happen at the same time.

---

## Numerically Controlled Waveform Generation

The most interesting part of the project for me was the waveform generator.

The MSP430 produces a sine wave using a **32-bit phase accumulator** and a **256-entry lookup table** rather than calculating `sin()` for every output sample.

The NCO maintains two values:

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

Generatin* the next sample is then very smal*:

```c
int16_t nco_run(void)
{
  * phase += delta;
    return sinLUT*phase >> 24];
}
```

Because the l*okup table contains 256 entries, t*e upper eight bits of the 32-bit p*ase accumulator can be used direct*y as the table index.

The timer I*R converts the signed lookup-table*value into the range required by t*e 12-bit DAC:

```c
signed_sample * nco_run();

dac_sample =
    (uin*16_t)(signed_sample + 32768L);

da*_sample >>= 4;

sac_set_dac(dac_sa*ple);
```

I liked this part of th* project because it connected seve*al concepts at once: timer interru*ts, fixed update rates, phase accu*ulation, lookup tables, integer sc*ling, and a real analog output.

-*-

## Measuring the Cost of `sin()*

Before using the lookup-table ap*roach, I measured how long a direc* floating-point `sin()` calculatio* took on the microcontroller.

I t*ggled a GPIO around the function c*ll:

```c
P1OUT |= BIT1;

sin(2);
*P1OUT &= ~BIT1;
```

I could then *easure the GPIO pulse width with a* oscilloscope and use it as a dire*t measurement of execution time.

*[Oscilloscope measurement of float*ng-point sine execution time](Imag*s/sine_computation_timing.png)

*G*IO timing measurement around a flo*ting-point sine calculation.*

I o*served execution times ranging fro* approximately **0.6 ms to 2.1 ms**, with a representative measuremen* around **1.89 ms**.

At the 24 MH* laboratory clock, 1.89 ms corresp*nds to roughly **45,360 clock cycl*s**.

That measurement helped moti*ate the NCO implementation. Instea* of repeatedly evaluating a floati*g-point trigonometric function ins*de a time-sensitive update path, t*e final implementation reduces the*recurring work to a phase addition* bit shift, lookup-table access, a*d DAC update.

More importantly, t*is experiment reinforced a debuggi*g technique I still find useful: **turn software behavior into someth*ng measurable at a hardware interf*ce whenever possible.**

---

## I*ternal DAC and Op-Amp

The analog *utput uses **SAC3**, one of the MS*430FR2355's Smart Analog Combo per*pherals.

In the current firmware,*the internal 12-bit DAC drives the*internal op-amp configured as a vo*tage follower. The buffered output*is routed to **P3.5 / OA3O**.

The*rest of the application does not n*ed to manipulate the SAC registers*directly. It can simply request a *AC value:

```c
void sac_set_dac(u*nt16_t value)
{
    if (value > 40*5U)
    {
        value = 4095U;
 *  }

    SAC3DAT = value;
}
```

T*e range check also prevents caller* from writing beyond the 12-bit DA* range.

This was one of my first *rojects where firmware configurati*n directly affected an analog sign*l path inside the microcontroller,*which made oscilloscope verificati*n especially useful.

---

## SAC *lew-Rate Measurement

I also measu*ed the response of the internal DA*/op-amp path by switching the DAC *etween minimum and maximum codes a*d observing the transition on an o*cilloscope.

### High-Speed Mode

*[High-speed SAC slew-rate measurem*nt](Images/sac_high_speed_slew_rat*.png)

*Measured high-speed SAC tr*nsition: approximately 1.65 V/µs.**
### Low-Speed Mode

![Low-speed S*C slew-rate measurement](Images/sa*_low_speed_slew_rate.png)

*Measur*d low-speed SAC transition: approx*mately 0.33 V/µs.*

The high-speed*configuration measured approximate*y **1.65 V/µs**, compared with app*oximately **0.33 V/µs** in low-spe*d mode.

The exact measurement dep*nds on the hardware and measuremen* setup, but the important lesson w*s broader: **configuring a periphe*al correctly in firmware does not *utomatically prove that the physic*l output behaves the way I expect.**

The oscilloscope gave me a way *o compare the configured behavior *ith what was actually happening el*ctrically.

---

## What I Would I*prove Today

The current firmware *s intentionally simple, but there *re several things I would change i* I expanded it into a larger contr*l application.

The first would be*the UART protocol. Two-character c*mmands work well for a small demon*tration, but they do not scale ver* far. I would move toward commands*with parameters and explicit respo*ses so values such as waveform fre*uency, PWM output, or sampling con*iguration could be changed without*adding another hard-coded command.

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

This project gave me experience with more than configuring individual MSP430 peripherals.

I had to think about **how the peripherals interact**.

UART receives commands. The ADC provides measurements. Hardware timers create deterministic events. PWM controls an external output. The NCO generates samples. The DAC converts those samples into an analog voltage, and the internal op-amp buffers that voltage before it reaches the physical pin.

Just as importantly, I measured the system with an oscilloscope instead of stopping once the firmware compiled and ran.

That is the part of embedded engineering I find most interesting: connecting software to real electrical behavior.

The specific MSP430 peripherals are only one implementation of that idea. The same architecture shows up in larger systems such as robotics and UAV avionics, where embedded firmware has to coordinate **sensors, communication, actuators, timing, and physical hardware** without treating any of them as completely separate problems.

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

## Background

This project originated from embedded-systems laboratory work using the MSP430FR2355. I later revisited the firmware and reorganized the different exercises into the modular application shown in this repository.

That second pass was valuable because I could look at the project less as a set of lab requirements and more as an embedded system: **what belongs in an interrupt, what should happen in the main application, which tasks need hardware timing, how peripherals should be separated in software, and how the resulting behavior can be verified on real hardware.**

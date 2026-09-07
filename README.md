# PicoProjects

A collection of embedded-systems projects developed using the **Raspberry Pi Pico (RP2040)**, covering bare-metal programming, GPIO, UART, timers, PIO, ultrasonic sensing, SPI displays, and LoRa communication.

This repository contains my hands-on work in **Embedded C, low-level RP2040 programming, peripheral interfacing, sensor integration, and wireless communication**.

---

## About

This repository brings together my Raspberry Pi Pico projects developed while learning and working with embedded systems.

The projects progress from fundamental microcontroller concepts such as GPIO and UART to more advanced topics including:

* Bare-metal RP2040 register programming
* UART communication
* Programmable I/O (PIO)
* Ultrasonic distance measurement
* SPI TFT display interfacing
* LoRa wireless communication
* Timer and timing-based applications
* Sensor integration
* Embedded system debugging and hardware interfacing

The projects are primarily written in **C / Embedded C** and built using the **Raspberry Pi Pico SDK and CMake** where applicable.

---

## Hardware

### Main Development Board

* Raspberry Pi Pico
* RP2040 microcontroller
* Dual-core ARM Cortex-M0+
* 133 MHz default clock
* GPIO
* UART
* SPI
* I2C
* PWM
* ADC
* Programmable I/O (PIO)
* Hardware timers

### Peripherals & Modules

The repository contains projects involving several embedded peripherals and modules, including:

* Ultrasonic distance sensors
* JSN-SR04T / AJSR04T
* ILI9341 TFT display
* LoRa modules
* UART-to-USB adapters
* LEDs
* Push buttons
* Buzzers
* Transistors
* Other GPIO-based peripherals

---

## Software & Tools

* **C**
* **Embedded C**
* **Raspberry Pi Pico SDK**
* **CMake**
* **Ninja**
* **ARM GCC Toolchain**
* **VS Code**
* **PIO Assembly**

---

## Projects

| Project                 | Description                                                    |
| ----------------------- | -------------------------------------------------------------- |
| `BareMetalBlink`        | Basic RP2040 GPIO programming using direct register access     |
| `PICO_UART`             | UART communication using the Raspberry Pi Pico                 |
| `PIO_UART_TX`           | UART transmission implemented using RP2040 Programmable I/O    |
| `ajsr04t`               | Ultrasonic sensor interfacing using PIO                        |
| `Display_Sensor`        | Sensor data acquisition and display interfacing                |
| `ILI9341_Pico`          | ILI9341 TFT display driver and Pico interfacing                |
| `LoRa_TX`               | LoRa transmitter implementation                                |
| `LoRa_RX`               | LoRa receiver implementation                                   |
| `Indicator_TX_SIde`     | Transmitter-side embedded application                          |
| `Indicator_TX_PIO_Side` | Transmitter application using PIO-based ultrasonic measurement |
| `Indicator_RX_Side`     | Receiver-side indicator, display and alert application         |

---

## Key Concepts Covered

### 1. Bare-Metal RP2040 Programming

Some projects interact directly with RP2040 hardware registers instead of relying entirely on high-level SDK functions.

Topics include:

* GPIO configuration
* GPIO function selection
* SIO registers
* GPIO input/output registers
* Peripheral control registers
* Interrupt configuration
* NVIC
* Hardware timers
* Register-level bit manipulation
* `volatile`
* Memory-mapped I/O

Example:

```c
#define SIO_BASE 0xD0000000u

#define SIO_GPIO_OUT (*(volatile uint32_t *)(SIO_BASE + 0x010))

#define SIO_GPIO_OE  (*(volatile uint32_t *)(SIO_BASE + 0x020))
```

This approach provides a deeper understanding of how the RP2040 hardware actually operates underneath the SDK abstractions.

---

### 2. UART Communication

UART projects explore serial communication between the Raspberry Pi Pico and external devices.

Topics include:

* UART initialization
* Baud-rate configuration
* TX/RX
* FIFO operation
* UART status flags
* Polling
* Serial debugging
* UART peripheral registers

---

### 3. Programmable I/O (PIO)

The RP2040's PIO subsystem is used for implementing timing-sensitive interfaces and custom communication protocols.

Projects explore:

* PIO state machines
* PIO assembly
* FIFO communication
* `PULL`
* `MOV`
* `SET`
* `JMP`
* Scratch registers
* PIO timing
* CPU ↔ PIO communication

Example PIO source files are included directly in the relevant projects.

---

### 4. Ultrasonic Distance Measurement

The ultrasonic projects interface the Pico with ultrasonic distance sensors such as the JSN-SR04T / AJSR04T.

The measurement process involves:

```text
Pico
 │
 │ Trigger
 ▼
Ultrasonic Sensor
 │
 │ Echo
 ▼
Pico
 │
 ▼
Measure Echo Pulse Width
 │
 ▼
Calculate Distance
```

The measured echo duration is converted into distance using the propagation speed of sound.

Some implementations use the RP2040 PIO subsystem to perform accurate timing measurements.

---

### 5. ILI9341 TFT Display

The repository also contains projects for interfacing an **ILI9341 TFT display** with the Raspberry Pi Pico.

Topics include:

* SPI communication
* Display initialization
* Commands and data
* Pixel drawing
* Text rendering
* Bitmap rendering
* Basic graphics
* Low-level display control

---

### 6. LoRa Communication

The LoRa projects implement wireless communication between transmitter and receiver nodes.

The general architecture is:

```text
┌─────────────────────┐
│     Pico TX         │
│                     │
│ Sensor              │
│    │                │
│    ▼                │
│ LoRa Module         │
└─────────┬───────────┘
          │
          │  LoRa
          │ Wireless
          ▼
┌─────────────────────┐
│     Pico RX         │
│                     │
│ LoRa Module         │
│    │                │
│    ▼                │
│ Display / Buzzer    │
└─────────────────────┘
```

The LoRa module is controlled by the Pico through UART.

---

## Project Architecture

Several projects follow a modular embedded-software structure:

```text
Application
     │
     ├── Sensor Driver
     │
     ├── Display Driver
     │
     ├── UART Driver
     │
     ├── LoRa Driver
     │
     └── Timer / Timing Module
             │
             ▼
        RP2040 Hardware
```

Source files are separated into `.c` and `.h` modules where appropriate to keep drivers and application logic organized.

---

## Build System

The projects that use the Raspberry Pi Pico SDK are configured using:

* CMake
* Pico SDK
* ARM GCC
* Ninja

A typical project contains:

```text
Project/
├── CMakeLists.txt
├── pico_sdk_import.cmake
├── main.c
├── driver.c
├── driver.h
└── driver.pio
```

Build directories and generated firmware files are intentionally excluded from version control.

---

## Repository Structure

```text
PicoProjects/
│
├── BareMetalBlink/
│   └── Bare-metal GPIO experiments
│
├── PICO_UART/
│   └── UART experiments
│
├── PIO_UART_TX/
│   └── PIO-based UART transmission
│
├── ajsr04t/
│   └── Ultrasonic sensor + PIO
│
├── Display_Sensor/
│   └── Sensor + ILI9341 display
│
├── ILI9341_Pico/
│   └── ILI9341 display driver
│
├── LoRa_TX/
│   └── LoRa transmitter
│
├── LoRa_RX/
│   └── LoRa receiver
│
├── Indicator_TX_SIde/
│   └── Transmitter-side application
│
├── Indicator_TX_PIO_Side/
│   └── PIO-based transmitter application
│
├── Indicator_RX_Side/
│   └── Receiver + display + alert system
│
├── .gitignore
└── README.md
```

---

## Development Philosophy

The main goal of these projects is not only to make the hardware work, but also to understand what is happening at the hardware level.

I focus on:

* Understanding microcontroller architecture
* Reading datasheets and reference manuals
* Working with memory-mapped registers
* Writing reusable peripheral drivers
* Understanding timing requirements
* Using hardware peripherals efficiently
* Debugging hardware and firmware together
* Gradually moving from SDK abstractions toward low-level programming

Where appropriate, projects intentionally use direct register manipulation to understand the underlying RP2040 hardware.

---

## Future Work

Planned areas of development include:

* Low-power operation
* Battery-powered Pico systems
* Deep-sleep and power-management techniques
* More PIO-based peripherals
* Additional sensor interfaces
* SD card interfacing
* RTC integration
* More advanced display graphics
* Embedded networking
* Improved driver architecture
* RTOS-based embedded applications
* More bare-metal RP2040 development

---

## Author

**Prince Jha**

B.Tech — Electronics and Communication Engineering

Interested in:

* Embedded Systems
* Firmware Development
* IoT
* Microcontrollers
* Low-level Programming
* Wireless Communication
* Hardware–Software Integration

---

## License

This repository is primarily a personal collection of embedded-systems projects and experiments.

Unless otherwise specified, the code is intended for learning, experimentation, and educational purposes.

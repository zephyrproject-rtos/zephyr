.. zephyr:board:: max78002evkit

Overview
********
The MAX78002 evaluation kit (EV kit) provides a platform and tools for leveraging device capabilities to build new
generations of artificial intelligence (AI) products.

The kit provides optimal versatility with a modular peripheral architecture, allowing a variety of input and output
devices to be remotely located. DVP and CSI cameras, I2S audio peripherals, digital microphones, and analog sensors
are supported, while a pair of industry-standard QWIIC connectors supports a large and growing array of aftermarket
development boards. An onboard stereo audio codec offers line-level audio input and output, and tactile input is
provided by a touch-enabled 2.4in TFT display. The MAX78002 energy consumption is tracked by a power accumulator,
with four channels of formatted results presented on a secondary TFT display. All device GPIOs are accessible on
0.1in pin headers. A standard coaxial power jack serves as power input, using the included 5V, 3A wall-mount
adapter. Two USB connectors provide serial access to the MAX78002, one directly and the other through a USB to UART
bridge. A third USB connector allows access to the MAX78002 energy consumption data. Rounding out the features, a
microSD connector provides the capability for inexpensive highdensity portable data storage.

The Zephyr port is running on the MAX78002 MCU.

.. image:: img/max78002evkit.webp
   :align: center
   :alt: MAX78002 EVKIT Front

.. image:: img/max78002evkit_back.webp
   :align: center
   :alt: MAX78002 EVKIT Back

Hardware
********

- MAX78002 MCU:

  - Dual-Core, Low-Power Microcontroller

    - Arm Cortex-M4 Processor with FPU up to 120MHz
    - 2.5MB Flash, 64KB ROM, and 384KB SRAM
    - Optimized Performance with 16KB Instruction Cache
    - Optional Error Correction Code (ECC SEC-DED) for SRAM
    - 32-Bit RISC-V Coprocessor up to 60MHz
    - Up to 60 General-Purpose I/O Pins
    - MIPI Camera Serial Interface 2 (MIPI CSI-2) Controller V2.1
    - Support for Two Data Lanes
    - 12-Bit Parallel Camera Interface
    - I 2S Controller/Target for Digital Audio Interface
    - Secure Digital Interface Supports SD 3.0/SDIO 3.0/eMMC 4.51

  - Convolutional Neural Network (CNN) Accelerator

    - Highly Optimized for Deep CNNs
    - 2 Million 8-Bit Weight Capacity with 1-, 2-, 4-, and 8-bit Weights
    - 1.3MB CNN Data Memory
    - Programmable Input Image Size up to 2048 x 2048 Pixels
    - Programmable Network Depth up to 128 Layers
    - Programmable per Layer Network Channel Widths up to 1024 Channels
    - 1- and 2-Dimensional Convolution Processing
    - Capable of Processing VGA Images at 30fps

  - Power Management for Extending Battery Life

    - Integrated Single-Inductor Multiple-Output (SIMO) Switch-Mode Power Supply (SMPS)
    - 2.85V to 3.6V Supply Voltage Range
    - Support of Optional External Auxiliary CNN Power Supply
    - Dynamic Voltage Scaling Minimizes Active Core Power Consumption
    - 23.9μA/MHz While Loop Execution at 3.3V from Cache (CM4 only)
    - Selectable SRAM Retention in Low-Power Modes with Real-Time Clock (RTC) Enabled

  - Security and Integrity

    - Available Secure Boot
    - AES 128/192/256 Hardware Acceleration Engine
    - True Random Number Generator (TRNG) Seed Generator

  - Ultra-Low-Power Wireless Microcontroller

    - Internal 100MHz Oscillator
    - Flexible Low-Power Modes with 7.3728MHz System Clock Option
    - 512KB Flash and 128KB SRAM (Optional ECC on One 32KB SRAM Bank)
    - 16KB Instruction Cache

  - Bluetooth 5.2 LE Radio

    - Dedicated, Ultra-Low-Power, 32-Bit RISC-V Coprocessor to Offload Timing-Critical Bluetooth Processing
    - Fully Open-Source Bluetooth 5.2 Stack Available
    - Supports AoA, AoD, LE Audio, and Mesh
    - High-Throughput (2Mbps) Mode
    - Long-Range (125kbps and 500kbps) Modes
    - Rx Sensitivity: -97.5dBm; Tx Power: +4.5dBm
    - Single-Ended Antenna Connection (50Ω)

  - Power Management Maximizes Battery Life

    - 2.0V to 3.6V Supply Voltage Range
    - Integrated SIMO Power Regulator
    - Dynamic Voltage Scaling (DVS)
    - 23.8μA/MHz Active Current at 3.0V
    - 4.4μA at 3.0V Retention Current for 32KB
    - Selectable SRAM Retention + RTC in Low-Power Modes

  - Multiple Peripherals for System Control

    - Up to Two High-Speed SPI Master/Slave
    - Up to Three High-Speed I2C Master/Slave (3.4Mbps)
    - Up to Four UART, One I2S Master/Slave
    - Up to 8-Input, 10-Bit Sigma-Delta ADC 7.8ksps
    - Up to Four Micro-Power Comparators
    - Timers: Up to Two Four 32-Bit, Two LP, TwoWatchdog Timers
    - 1-Wire® Master
    - Up to Four Pulse Train (PWM) Engines
    - RTC with Wake-Up Timer
    - Up to 52 GPIOs

  - Security and Integrity​

    - Available Secure Boot
    - TRNG Seed Generator
    - AES 128/192/256 Hardware Acceleration Engine

- External devices connected to the MAX78002 EVKIT:

  - Color TFT Display
  - Audio Stereo Codec Interface
  - Digital Microphone
  - A 8Mb QSPI ram

Supported Features
==================

The ``max78002evkit/max78002/m4`` board target supports the following interfaces:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| SYSTICK   | on-chip    | systick                             |
+-----------+------------+-------------------------------------+
| CLOCK     | on-chip    | clock and reset control             |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial                              |
+-----------+------------+-------------------------------------+
| TRNG      | on-chip    | entropy                             |
+-----------+------------+-------------------------------------+
| I2C       | on-chip    | i2c                                 |
+-----------+------------+-------------------------------------+
| DMA       | on-chip    | dma controller                      |
+-----------+------------+-------------------------------------+
| Watchdog  | on-chip    | watchdog                            |
+-----------+------------+-------------------------------------+
| SPI       | on-chip    | spi                                 |
+-----------+------------+-------------------------------------+
| ADC       | on-chip    | adc                                 |
+-----------+------------+-------------------------------------+
| Timer     | on-chip    | counter                             |
+-----------+------------+-------------------------------------+
| PWM       | on-chip    | pwm                                 |
+-----------+------------+-------------------------------------+
| W1        | on-chip    | one wire master                     |
+-----------+------------+-------------------------------------+
| Flash     | on-chip    | flash                               |
+-----------+------------+-------------------------------------+

Connections and IOs
===================

+-----------+-------------------+----------------------------------------------------------------------------------+
| Name      | Signal            | Usage                                                                            |
+===========+===================+==================================================================================+
| JP1       | 3V3 MON           | Normal operation in conjunction with JP3 jumpered 1-2                            |
+-----------+-------------------+----------------------------------------------------------------------------------+
| JP2       | 3V3 SW PM BYPASS  | Power monitor shunts for main 3.3 V system power are bypassed                    |
+-----------+-------------------+----------------------------------------------------------------------------------+
| JP3       | CNN MON           | Normal operation in conjunction with JP6 jumpered 1-2                            |
+-----------+-------------------+----------------------------------------------------------------------------------+
| JP4       | VCOREA PM BYPASS  | Power monitor shunts for U4's share of VCOREA + CNN loads are bypassed           |
+-----------+-------------------+----------------------------------------------------------------------------------+
| JP5       | VCOREB PM BYPASS  | Power monitor shunts for VCOREB are bypassed                                     |
+-----------+-------------------+----------------------------------------------------------------------------------+
| JP6       | VREGO_A PM BYPASS | Power monitor shunts for VREGO_A are bypassed                                    |
+-----------+-------------------+----------------------------------------------------------------------------------+
| JP7       | VBAT              | Enable/Disable 3V3 VBAT power                                                    |
+-----------+-------------------+----------------------------------------------------------------------------------+
| JP8       | VREGI             | Enable/Disable 3V3 VREGI power                                                   |
+-----------+-------------------+----------------------------------------------------------------------------------+
| JP9       | VREGI/VBAT        | Onboard 3V3_PM / external source at TP10 supplies VREGI/VBAT                     |
+-----------+-------------------+----------------------------------------------------------------------------------+
| JP10      | VDDIOH            | Onboard 3V3_PM/3V3_SW supplies VDDIOH                                            |
+-----------+-------------------+----------------------------------------------------------------------------------+
| JP11      | VDDA              | VREGO_A_PM powers VDDA                                                           |
+-----------+-------------------+----------------------------------------------------------------------------------+
| JP12      | VDDIO             | VREGO_A_PM powers VDDIO                                                          |
+-----------+-------------------+----------------------------------------------------------------------------------+
| JP13      | VCOREB            | VREGO_B powers VCOREB                                                            |
+-----------+-------------------+----------------------------------------------------------------------------------+
| JP14      | VCOREA            | VREGO_C ties to net VCOREA                                                       |
+-----------+-------------------+----------------------------------------------------------------------------------+
| JP15      | VREF              | DUT ADC VREF is supplied by precision external reference                         |
+-----------+-------------------+----------------------------------------------------------------------------------+
| JP16      | I2C1 SDA          | I2C1 DATA pull-up                                                                |
+-----------+-------------------+----------------------------------------------------------------------------------+
| JP17      | I2C1 SCL          | I2C1 CLOCK pull-up                                                               |
+-----------+-------------------+----------------------------------------------------------------------------------+
| JP18      | TRIG1             | PWR accumulator trigger signal 1 ties to port 1.6                                |
+-----------+-------------------+----------------------------------------------------------------------------------+
| JP19      | TRIG2             | PWR accumulator trigger signal 2 ties to port 1.7                                |
+-----------+-------------------+----------------------------------------------------------------------------------+
| JP20      | UART0 EN          | Connect/Disconnect USB-UART bridge to UART0                                      |
+-----------+-------------------+----------------------------------------------------------------------------------+
| JP21      | I2C0_SDA          | I2C0 DATA pull-up                                                                |
+-----------+-------------------+----------------------------------------------------------------------------------+
| JP22      | I2C0_SCL          | I2C0 CLOCK pull-up                                                               |
+-----------+-------------------+----------------------------------------------------------------------------------+
| JP23      | UART1 EN          | Connect/Disconnect USB-UART bridge to UART1                                      |
+-----------+-------------------+----------------------------------------------------------------------------------+
| JP24      | EXT I2C0 EN       | Enable/Disable QWIIC interface for I2C0                                          |
+-----------+-------------------+----------------------------------------------------------------------------------+
| JP25      | PB1 PU            | Enable/Disable 100kΩ pull-up for pushbutton mode, port 2.6                       |
+-----------+-------------------+----------------------------------------------------------------------------------+
| JP26      | PB2 PU            | Enable/Disable 100kΩ pull-up for pushbutton mode, port 2.7                       |
+-----------+-------------------+----------------------------------------------------------------------------------+
| JP27      | I2C2 SDA          | I2C2 DATA pull-up                                                                |
+-----------+-------------------+----------------------------------------------------------------------------------+
| JP28      | I2C2 SCL          | I2C2 CLOCK pull-up                                                               |
+-----------+-------------------+----------------------------------------------------------------------------------+
| JP29      | VDDB              | USB XCVR VDDB powered from VBUS / powered full time by system 3V3_PM             |
+-----------+-------------------+----------------------------------------------------------------------------------+
| JP30      | EXT I2C2 EN       | Enable/Disable QWIIC interface for I2C2                                          |
+-----------+-------------------+----------------------------------------------------------------------------------+
| JP31      | L/R SEL           | Select MIC ON R/L CH, I2S microphone data stream                                 |
+-----------+-------------------+----------------------------------------------------------------------------------+
| JP32      | MIC-I2S I/O       | External I2S/MIC data from I2S I/O / MIC header connected to I2S SDI             |
+-----------+-------------------+----------------------------------------------------------------------------------+
| JP33      | MIC-I2S/CODEC     | Onboard CODEC data / external I2S data from header connects to I2S SDI           |
+-----------+-------------------+----------------------------------------------------------------------------------+
| JP34      | I2S VDD           | Select 1.8V/3.3V for external MIC and DATA I2S interface                         |
+-----------+-------------------+----------------------------------------------------------------------------------+
| JP35      | I2C1 SDA          | I2C1 DATA pull-up                                                                |
+-----------+-------------------+----------------------------------------------------------------------------------+
| JP36      | I2C1 SCL          | I2C1 CLOCK pull-up                                                               |
+-----------+-------------------+----------------------------------------------------------------------------------+
| JP37      | I2S CK SEL        | Select SMA connector J6 / onboard crystal oscillator for I2S master clock source |
+-----------+-------------------+----------------------------------------------------------------------------------+
| JP38      | DVP CAM PWR       | Enable/Disable OVM7692 for DVP camera PWDN input                                 |
+-----------+-------------------+----------------------------------------------------------------------------------+
| JP39      | SW CAM PWUP       | Camera reset and power up under port pin control                                 |
+-----------+-------------------+----------------------------------------------------------------------------------+
| JP40      | HW PWUP / SW PWUP | Camera will reset and power up as soon as 3.3V reaches a valid level             |
+-----------+-------------------+----------------------------------------------------------------------------------+
| JP41      | CSI CAM I2C EN    | Connect/Disconnect I2C1 to CSI camera Digilent P5C I2C                           |
+-----------+-------------------+----------------------------------------------------------------------------------+
| JP42      | TFT DC            | TFT data/command select connects to port 2.2                                     |
+-----------+-------------------+----------------------------------------------------------------------------------+
| JP43      | TFT CS            | Select port 0.3 / port 1.7 to drive TFT CS                                       |
+-----------+-------------------+----------------------------------------------------------------------------------+
| JP44      | LED1 EN           | Enable/Disable LED1                                                              |
+-----------+-------------------+----------------------------------------------------------------------------------+
| JP45      | LED2 EN           | Enable/Disable LED2                                                              |
+-----------+-------------------+----------------------------------------------------------------------------------+

Programming and Debugging
*************************

Flashing
========

The MAX78002 MCU can be flashed by connecting an external debug probe to the
SWD port. SWD debug can be accessed through the Cortex 10-pin connector, JH8.
Logic levels are fixed to VDDIO (1.8V).

Once the debug probe is connected to your host computer, then you can simply run the
``west flash`` command to write a firmware image into flash.

.. note::

   This board uses OpenOCD as the default debug interface. You can also use
   a Segger J-Link with Segger's native tooling by overriding the runner,
   appending ``--runner jlink`` to your ``west`` command(s). The J-Link should
   be connected to the standard 2*5 pin debug connector (JH8) using an
   appropriate adapter board and cable.

Debugging
=========

Please refer to the `Flashing`_ section and run the ``west debug`` command
instead of ``west flash``.

References
**********

- `MAX78002EVKIT web page`_

.. _MAX78002EVKIT web page:
   https://www.analog.com/en/resources/evaluation-hardware-and-software/evaluation-boards-kits/max78002evkit.html

.. zephyr:board:: myra_sip_baseboard

Overview
********

The Myra SiP Baseboard features Antmicro's **Myra** SiP, which integrates the **STM32G491REI6** MCU,
128kB FRAM, and FTDI FT231XQ USB to UART converter. The board is equipped with temperature,
humidity, and pressure sensors, designed to help monitor conditions in server rooms.

The sensors are placed on a separate island that is detachable from the main PCB and can be
installed directly in the required place. It provides local storage for data logging and a battery
backup for protection against data loss. The board can be used as a building block for PoC solutions
for monitoring environmental parameters.

Key features include:

- STM32G491REI6 MCU (Cortex-M4, 170 MHz)
- 128 KB Fujitsu FRAM
- FTDI FT231XQ USB to UART converter
- 50 mm x 26.5 mm PCB
- USB-C Connector for data and power
- SHT45 temperature + humidity sensor
- BME280 temperature + humidity + pressure sensor
- QWIIC connectors for peripheral expansion
- RTC with battery backup

More information about the board can be found on `Antmicro's Open Hardware Portal <https://openhardware.antmicro.com/boards/environment-sensor-sip-baseboard>`_.

Hardware
********

Myra SiP provides the following hardware:

- **STM32G491REI6 MCU**:

  - ARM Cortex-M4 CPU with FPU, up to 170 MHz
  - Clock Sources:

    - 4 to 48 MHz external crystal oscillator (HSE)
    - 32 kHz crystal oscillator for RTC (LSE)
    - Internal 16 MHz RC (±1%)
    - Internal low-power 32 kHz RC (±5%)
    - 2 PLLs for system clock, USB, audio, ADC
  - RTC: Real-time clock with hardware calendar, alarms, and calibration
  - Timers:

    - 1x 32-bit timer and 2x 16-bit timers with up to 4x IC/OC/PWM or pulse counter and quadrature
      (incremental) encoder input
    - 3x 16-bit advanced motor control timers with up to 8x PWM channels, dead time generation,
      emergency stop
    - 1x 16-bit timer with 2x IC/OC, one OCN/PWM, dead time generation, emergency stop
    - 2x watchdog timers (independent, window)
    - 2x 16-bit basic timers
    - SysTick timer
    - 1x low-power timer
  - I/Os: Up to 86 fast I/Os, most 5V tolerant
  - Memory:

    - 512 KB Flash memory with ECC and PCROP protection
    - 96 KB SRAM including 32 KB with hardware parity check
  - Analog peripherals:

    - 3x 16-bit ADCs with up to 36 channels, hardware oversampling, and resolution up to 16-bit
    - 4x 12-bit DAC channels
    - 4x ultra-fast rail-to-rail analog comparators
    - 4x operational amplifiers with built-in PGA
    - Internal temperature sensor and voltage reference with support for three output voltages
      (2.048 V, 2.5 V, 2.9 V)
  - Communication Interfaces:

    - 2x FDCAN controllers supporting flexible data rate
    - 3x I2C Fast Mode Plus (1 Mbit/s) with 20 mA current sink, SMBus/PMBus support
    - 5x USART/UART (ISO 7816, LIN, IrDA, modem control)
    - 1x LPUART
    - 3x SPI interfaces (2x with multiplexed half-duplex I²S)
    - 1x SAI (serial audio interface)
    - USB 2.0 full-speed with LPM and BCD support
    - IRTIM (infrared interface)
    - USB Type-C™ / USB Power Delivery (UCPD)
  - Other Peripherals:

    - 16-channel DMA controller
    - True Random Number Generator (RNG)
    - CRC calculation unit, 96-bit unique ID
    - Development support: SWD, JTAG, Embedded Trace Macrocell™
    - ECOPACK2® compliant packages

- **128 KB Fujitsu MB85RS1MT FRAM**: Local storage for data logging, allowing non-volatile memory storage.

- **FTDI FT231XQ USB to UART converter**: Provides a reliable USB to UART interface.


More information about STM32G491RE can be found here:

- `STM32G491RE on www.st.com`_

Other board's peripherals:
--------------------------

- USB-C Connector: For data and power.
- SHT45 sensor:

  - Relative humidity accuracy: ±1.0% RH
  - Operating humidity range: 0-100% RH
  - Temperature accuracy: ±0.1°C
  - Operating temperature range: -40°C to 125°C
- BME280 sensor:

  - Relative humidity accuracy: ±3% RH
  - Temperature accuracy: ±1°C
  - Pressure accuracy: ±1 hPa
  - Operating temperature range: -40°C to 85°C
  - Pressure range: 300-1100 hPa
- QWIIC connectors: For easy peripheral expansion.

Supported Features
------------------

.. zephyr:board-supported-hw::

Connections and IOs
-------------------

Antmicro's Myra SiP Baseboard provides the following default pin mappings for peripherals:

.. rst-class:: rst-columns

- LPUART_1_TX : PA2
- LPUART_1_RX : PA3
- I2C_1_SCL : PB8
- I2C_1_SDA : PB9
- SPI_CS2 : PB2
- SPI_CS3 : PA7
- SPI_2_SCK : PB13
- SPI_2_MISO : PB14
- SPI_2_MOSI : PB15
- PWM_2_CH1 : PA5
- USER_PB : PC13
- LD2 : PA5
- ADC1_IN1 : PA0
- DAC1_OUT1 : PA4
- USB_MCU_N : PA11
- USB_MCU_P : PA12
- SWDIO-JMTS : PA13
- SWCLK-JTCK : PA14
- JTDI : PA15
- JTDO : PB3
- JTRST : PB4
- FRAM_HOLD (ACTIVE LOW) : PB10
- FRAM_WP (ACTIVE LOW) : PB11
- FRAM_CS (ACTIVE LOW) : PB12
- GPIO_PC10 : PC10
- GPIO_PC11 : PC11
- GPIO_PC12 : PC12
- PF0_OSC : PF0

System Clock
------------

System clock can be driven by an internal or an external oscillator, as well as by the main PLL
clock. By default, system clock is driven by PLL clock at 170MHz (boost mode selected), which in
turn, is driven by the 8MHz high speed external oscillator (HSE). While the HSE oscillator is
capable of operating at frequencies up to 48 MHz by default, in this configuration, it is
specifically set to 8 MHz.

Serial Port
-----------

The Myra SiP Baseboard has 5 U(S)ARTs. The Zephyr console output is assigned to LPUART1. The default
settings are 115200 8N1.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``myra_sip_baseboard`` board target can be built and flashed in the usual way (see :ref:`build_an_application` and :ref:`application_run` for more details).

Flashing
********

This board has a USB-JTAG interface and can be used with OpenOCD.

Connect the Myra SiP Baseboard to your host computer using the USB port, then build and flash
the application. Here is an example for :zephyr:code-sample:`hello_world`.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: myra_sip_baseboard
   :goals: build flash

Then run a serial host program to connect with the Myra SiP Baseboard, e.g. using picocom:

.. code-block:: console

   $ picocom /dev/ttyUSB0 -b 115200

.. warning::
   The board has only one port that is used for both programming and the console. For this reason, it is
   recommended to set ``CONFIG_BOOT_DELAY`` to an arbitrary value. This is especially important when
   running twister tests on the device. You should then also use the ``--flash-before`` and
   ``--device-flash-timeout=120`` options:

   .. code-block:: console

       $ scripts/twister --device-testing --device-serial /dev/ttyUSB0 --device-serial-baud 115200 -p myra_sip_baseboard --flash-before --device-flash-timeout=120 -v

Debugging
*********

You can debug an application in the usual way. Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: myra_sip_baseboard
   :maybe-skip-config:
   :goals: debug

.. _STM32G491RE on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32g491re.html

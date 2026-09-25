.. zephyr:board:: core2350b

Overview
********

The Waveshare Core2350B is a compact core board based on the Raspberry Pi RP2350B microcontroller.
It brings out 48 GPIOs on 2.54 mm pitch headers for integration into carrier boards. USB is
provided through a 6-pin FPC connector, which connects to a USB Type-C port through an optional
adapter board.

The board is available in three variants that differ only in the populated PSRAM:

- Core2350B0: no PSRAM
- Core2350B1: 2 MB PSRAM
- Core2350B2: 8 MB PSRAM

The RP2350B SoC has two Cortex-M33 cores and two Hazard3 (RISC-V) cores. Zephyr supports builds
targeting either CPU architecture, but currently runs on one core only; running code on the
second core is not supported.

Hardware
********

- RP2350B microcontroller with dual Cortex-M33 or dual Hazard3 processors at up to 150 MHz
- 520 KB of SRAM and 16 MB of QSPI flash (W25Q128JVSIQ)
- Optional 2 MB or 8 MB QSPI PSRAM, depending on the variant
- 12 MHz crystal oscillator
- 6-pin 0.5 mm pitch FPC connector carrying VBUS, USB D+/D-, BOOTSEL and RUN
- ME6217C33M5G 3.3 V LDO regulator (800 mA), enable signal available on the header
- 64 pins on four 2x8 headers with 2.54 mm pitch
- USB 1.1 with device and host support
- Low-power sleep and dormant modes
- Drag-and-drop programming using mass storage over USB
- 48 multi-function GPIO pins, 8 of which can be used as ADC inputs
- 2 SPI, 2 I2C, 2 UART, 8 12-bit ADC channels, 24 controllable PWM channels
- Temperature sensor
- 3 Programmable I/O (PIO) blocks, 12 state machines total
- SWD debug port on the header
- User LED (red) on GPIO39

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The board has no buttons and no USB connector of its own. The ``BOOTSEL`` and ``RUN`` signals, the
USB data lines and ``VBUS`` are routed both to the FPC connector and to header P2. The USB Type-C
adapter board connected through the FPC cable provides the ``BOOT`` and ``RESET`` buttons.

Headers
-------

- P1: GPIO0 to GPIO14
- P3: GPIO15 to GPIO29
- P4: GPIO30 to GPIO44
- P2: GPIO45 to GPIO47, ``SWCLK``, ``SWD``, ``BOOTSEL``, ``RUN``, ``USBD_P``, ``USBD_N``,
  ``ADC_VREF``, ``3V3_EN``, ``VBUS`` and ``3V3``

The board is powered from ``VBUS`` (5 V) through the FPC connector or header P2. Pulling
``3V3_EN`` low disables the 3.3 V regulator.

Default Zephyr Peripheral Mapping
---------------------------------

.. rst-class:: rst-columns

- UART0_TX : GPIO0
- UART0_RX : GPIO1
- I2C0_SDA : GPIO4
- I2C0_SCL : GPIO5
- I2C1_SDA : GPIO6
- I2C1_SCL : GPIO7
- SPI0_RX : GPIO16
- SPI0_CSN : GPIO17
- SPI0_SCK : GPIO18
- SPI0_TX : GPIO19
- LED0 / PWM_11B : GPIO39
- ADC_CH0 : GPIO40
- ADC_CH1 : GPIO41
- ADC_CH2 : GPIO42
- ADC_CH3 : GPIO43
- ADC_CH4 : GPIO44
- ADC_CH5 : GPIO45
- ADC_CH6 : GPIO46

The Zephyr console and shell use USB CDC ACM by default.

GPIO47 (ADC channel 7) is the PSRAM chip select and also reaches header P2 through a 0 ohm
resistor (R11). On the Core2350B1 and Core2350B2 variants, do not use it as a general-purpose pin.
PSRAM is not currently supported in Zephyr.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The overall explanation regarding flashing and debugging is the same as for
:zephyr:board:`rpi_pico`. See :ref:`rpi_pico_programming_and_debugging` in
:zephyr:board:`rpi_pico` documentation. N.b. OpenOCD support requires using Raspberry Pi's forked
version of OpenOCD. The SWD signals are available on header P2.

Flashing with UF2
=================

To enter the UF2 bootloader with the USB Type-C adapter board connected, hold the ``BOOT`` button,
press and release ``RESET``, then release ``BOOT``. Without the adapter board, pull ``BOOTSEL`` to
ground while pulsing ``RUN`` low on header P2. The board then appears on the host as a mass
storage device.

Below is an example of building and flashing the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
    :zephyr-app: samples/basic/blinky
    :board: core2350b/rp2350b/m33_0
    :goals: build flash
    :flash-args: -r uf2

To target the Hazard3 (RISC-V) core instead, use ``core2350b/rp2350b/hazard3_0`` as the board.

References
**********

- `Core2350B Wiki`_
- `Core2350B Schematic`_
- `RP2350 Datasheet`_

.. _Core2350B Wiki:
   https://www.waveshare.com/wiki/Core2350B0

.. _Core2350B Schematic:
   https://files.waveshare.com/wiki/Core2350B0/Core2350B.pdf

.. _RP2350 Datasheet:
   https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf

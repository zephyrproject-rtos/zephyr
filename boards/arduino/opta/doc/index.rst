.. _arduino_opta_board:

Arduino OPTA PLC
################

Overview
********

The Arduino™ Opta® is a secure micro Programmable Logic Controller (PLC)
with Industrial Internet of Things (IoT) capabilities.

Developed in partnership with Finder®, this device supports both the Arduino
programming language and standard IEC-61131-3 PLC programming languages,
such as Ladder Diagram (LD), Sequential Function Chart (SFC),
Function Block Diagram (FBD), Structured Text (ST), and Instruction List (IL),
making it an ideal device for automation engineers.

Additionally, the device features:

- Ethernet compliant with IEEE802.3-2002
- 16MB QSPI Flash
- 4 x green color status LEDs
- 1 x user push-button
- 1 x reset push-button accessible via pinhole
- 8 x analog inputs
- 4 x isolated relay outputs

.. image:: img/arduino_opta.jpeg
     :align: center
     :alt: ARDUINO-OPTA

More information about the board can be found at the `ARDUINO-OPTA website`_.
More information about STM32H747XIH6 can be found here:

- `STM32H747XI on www.st.com`_
- `STM32H747xx reference manual`_
- `STM32H747xx datasheet`_

Supported Features
==================

The current Zephyr ``arduino_opta/stm32h747xx/m7`` board configuration supports the
following hardware features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port-polling;                |
|           |            | serial port-interrupt               |
+-----------+------------+-------------------------------------+
| PINMUX    | on-chip    | pinmux                              |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+
| FLASH     | on-chip    | flash memory                        |
+-----------+------------+-------------------------------------+
| RNG       | on-chip    | True Random number generator        |
+-----------+------------+-------------------------------------+
| I2C       | on-chip    | i2c                                 |
+-----------+------------+-------------------------------------+
| SPI       | on-chip    | spi                                 |
+-----------+------------+-------------------------------------+
| IPM       | on-chip    | virtual mailbox based on HSEM       |
+-----------+------------+-------------------------------------+
| ENET      | on-chip    | Ethernet controller                 |
+-----------+------------+-------------------------------------+
| USB FS    | on-chip    | USB Full Speed                      |
+-----------+------------+-------------------------------------+

And the ``arduino_opta/stm32h747xx/m4`` has the following
support from Zephyr:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port-polling;                |
|           |            | serial port-interrupt               |
+-----------+------------+-------------------------------------+
| PINMUX    | on-chip    | pinmux                              |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+

Other hardware features are not yet supported on Zephyr port.

The default configuration per core can be found in the defconfig files:
:zephyr_file:`boards/arduino/opta/arduino_opta_stm32h747xx_m7_defconfig`
:zephyr_file:`boards/arduino/opta/arduino_opta_stm32h747xx_m4_defconfig`

Pin Mapping
===========

ARDUINO OPTA both M7 and M4 cores have access to the 9 GPIO controllers.
These controllers are responsible for pin muxing, input/output, pull-up, etc.

For more details please refer to `ARDUINO-OPTA website`_.

Default Zephyr Peripheral Mapping
---------------------------------

- Status LED1 : PI0
- Status LED2 : PI1
- Status LED3 : PI3
- Status LED4 : PH15
- User button : PE4
- Input 1 : PA0
- Input 2 : PC2
- Input 3 : PF12
- Input 4 : PB0
- Input 5 : PF10
- Input 6 : PF8
- Input 7 : PF6
- Input 8 : PF4
- Output 1 : PI6
- Output 2 : PI5
- Output 3 : PI7
- Output 4 : PI4

System Clock
============

The STM32H747I System Clock can be driven by an internal or external oscillator,
as well as by the main PLL clock. By default, the CPU2 (Cortex-M4) System clock
is driven at 240MHz. PLL clock is fed by a 25MHz high speed external clock.

Resources sharing
=================

The dual core nature of STM32H747 SoC requires sharing HW resources between the
two cores. This is done in 3 ways:

- **Compilation**: Clock configuration is only accessible to M7 core. M4 core only
  has access to bus clock activation and deactivation.
- **Static pre-compilation assignment**: Peripherals such as a UART are assigned in
  devicetree before compilation. The user must ensure peripherals are not assigned
  to both cores at the same time.
- **Run time protection**: Interrupt-controller and GPIO configurations could be
  accessed by both cores at run time. Accesses are protected by a hardware semaphore
  to avoid potential concurrent access issues.

Programming and Debugging
*************************

Applications for the ``arduino_opta`` use the regular Zephyr build commands.
See :ref:`build_an_application` for more information about application builds.

Flashing
========

Flashing operation will depend on the target to be flashed and the SoC
option bytes configuration. The OPTA has a DFU capable bootloader which
can be accessed by connecting the device to the USB, and then pressing
the RESET button shortly twice, the RESET-LED on the board will fade
indicating the board is in bootloader mode.


Flashing an application to ARDUINO OPTA
---------------------------------------

First, connect the device to your host computer using
the USB port to prepare it for flashing. Then build and flash your application.

Here is an example for the :zephyr:code-sample:`blinky` application on M7 core.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: arduino_opta/stm32h747xx/m7
   :goals: build flash

And gere is an example for the :zephyr:code-sample:`blinky` application on M4 core.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: arduino_opta/stm32h747xx/m4
   :goals: build flash

Starting the application on the ARDUINO OPTA M4 when mixing Arduino Code on M7
------------------------------------------------------------------------------
Different from using both Zephyr in M7 and M4 cores, where they both are automatically
started, some users may want to use the PLC runtime, or even Arduino code running on the
M7 core, the instructions below show how to do that properly.

Make sure the option bytes are set to prevent the M4 from auto-starting, and
that the M7 side starts the M4 at the correct Flash address.

This can be done by selecting in the Arduino IDE's "Tools" / "Flash Split"
menu the "1.5MB M7 + 0.5MB M4" option, and loading a sketch that contains
at least the following code:

 .. code-block:: cpp

    #include <RPC.h>

    void setup() {
        RPC.begin();
    }

    void loop() { }

You should also need to shift the code space of the M4 Zephyr code, the Arduino
code on M7 sets the initial address of the M4 core to 0x80080000, on the other
hand, when using both Zephyr images the flash partition is different, so it is
needed to tell the M4 code to be linked on the start address used by the Arduino
IDE, for doing that, the user can just create an `arduino_opta_stm32h747xx_m4.overlay`
inside of the project folder, and add the contents below:

 .. code-block:: dts

    / {
        chosen {
            zephyr,code-partition = &slot0_partition;
        };
    };

    &flash1 {
        partitions {
           compatible = "fixed-partitions";
           #address-cells = <1>;
            #size-cells = <1>;

           slot0_partition: partition@80000 {
              label = "image-0";
              reg = <0x00080000 DT_SIZE_K(512)>;
           };
        };
    };

Debugging
=========

Debugging is not yet supported by this board, since the debug port does
not have an easy access. On the other hand it is possible to get console
I/O through side expansion connector, the console is mapped to USART6.
Additionally users may use the console and/or shell over the USB,
since it is supported in the M7 core. So after building an application
do as following:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: arduino_opta/stm32h747xx/m7
   :goals: build flash

Run a serial host program to connect with your board:

.. code-block:: console

   $ minicom -D /dev/ttyACM0

You should see the following message on the console:

.. code-block:: console

   Hello World! arduino_opta


.. _ARDUINO-OPTA website:
   https://docs.arduino.cc/hardware/opta

.. _STM32H747XI on www.st.com:
   https://www.st.com/content/st_com/en/products/microcontrollers-microprocessors/stm32-32-bit-arm-cortex-mcus/stm32-high-performance-mcus/stm32h7-series/stm32h747-757/stm32h747xi.html

.. _STM32H747xx reference manual:
   https://www.st.com/resource/en/reference_manual/dm00176879.pdf

.. _STM32H747xx datasheet:
   https://www.st.com/resource/en/datasheet/stm32h747xi.pdf

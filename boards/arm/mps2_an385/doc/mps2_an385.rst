.. _mps2_an385_board:

ARM V2M MPS2
############

Overview
********

The mps2_an385 board configuration is used by Zephyr applications that run on
the V2M MPS2 board. It provides support for the ARM Cortex-M3 (AN385) CPU and
the following devices:

- Nested Vectored Interrupt Controller (NVIC)
- System Tick System Clock (SYSTICK)
- Cortex-M System Design Kit UART

.. image:: img/mps2.png
     :width: 442px
     :align: center
     :height: 335px
     :alt: ARM V2M MPS2

More information about the board can be found at the `V2M MPS2 Website`_.

The Application Note AN385 can be found at `Application Note AN385`_.

Hardware
********

ARM V2M MPS2 provides the following hardware components:

- ARM Cortex-M3 (AN385)
- ARM IoT Subsystem for Cortex-M
- Form factor: 140x120cm
- SRAM: 8MB single cycle SRAM, 16MB PSRAM
- Video: QSVGA touch screen panel, 4bit RGB VGA connector
- Audio: Audio Codec
- Debug:

  - ARM JTAG20 connector
  - ARM parallel trace connector (MICTOR38)
  - 20 pin Cortex debug connector
  - 10 pin Cortex debug connector
  - ILA connector for FPGA debug

- Expansion

  - GPIO
  - SPI


Supported Features
==================

The mps2_an385 board configuration supports the following hardware features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| SYSTICK   | on-chip    | systick                             |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port-polling;                |
|           |            | serial port-interrupt               |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+
| WATCHDOG  | on-chip    | watchdog                            |
+-----------+------------+-------------------------------------+
| TIMER     | on-chip    | counter                             |
+-----------+------------+-------------------------------------+
| DUALTIMER | on-chip    | counter                             |
+-----------+------------+-------------------------------------+

Other hardware features are not currently supported by the port.
See the `V2M MPS2 Website`_ for a complete list of V2M MPS2 board hardware
features.

The default configuration can be found in the defconfig file:

.. code-block:: console

   boards/arm/mps2_an385/mps2_an385_defconfig

Interrupt Controller
====================

MPS2 is a Cortex-M3 based SoC and has 15 fixed exceptions and 45 IRQs.

A Cortex-M3/4-based board uses vectored exceptions. This means each exception
calls a handler directly from the vector table.

Handlers are provided for exceptions 1-6, 11-12, and 14-15. The table here
identifies the handlers used for each exception.

+------+------------+----------------+--------------------------+
| Exc# | Name       | Remarks        | Used by Zephyr Kernel    |
+======+============+================+==========================+
| 1    | Reset      |                | system initialization    |
+------+------------+----------------+--------------------------+
| 2    | NMI        |                | system fatal error       |
+------+------------+----------------+--------------------------+
| 3    | Hard fault |                | system fatal error       |
+------+------------+----------------+--------------------------+
| 4    | MemManage  | MPU fault      | system fatal error       |
+------+------------+----------------+--------------------------+
| 5    | Bus        |                | system fatal error       |
+------+------------+----------------+--------------------------+
| 6    | Usage      | undefined      | system fatal error       |
|      | fault      | instruction,   |                          |
|      |            | or switch      |                          |
|      |            | attempt to ARM |                          |
|      |            | mode           |                          |
+------+------------+----------------+--------------------------+
| 11   | SVC        |                | context switch and       |
|      |            |                | software interrupts      |
+------+------------+----------------+--------------------------+
| 12   | Debug      |                | system fatal error       |
|      | monitor    |                |                          |
+------+------------+----------------+--------------------------+
| 14   | PendSV     |                | context switch           |
+------+------------+----------------+--------------------------+
| 15   | SYSTICK    |                | system clock             |
+------+------------+----------------+--------------------------+

Pin Mapping
===========

The ARM V2M MPS2 Board has 4 GPIO controllers. These controllers are responsible
for pin muxing, input/output, pull-up, etc.

For mode details please refer to `MPS2 Technical Reference Manual (TRM)`_.

System Clock
============

The V2M MPS2 main clock is 24 MHz.

Serial Port
===========

The V2M MPS2 processor has five UARTs. Both the UARTs have only two wires for
RX/TX and no flow control (CTS/RTS) or FIFO. The Zephyr console output, by
default, is utilizing UART0.

Programming and Debugging
*************************

Flashing
========

V2M MPS2 provides:

- A USB connection to the host computer, which exposes a Mass Storage and an
  USB Serial Port.
- A Serial Flash device, which implements the USB flash disk file storage.
- A physical UART connection which is relayed over interface USB Serial port.

Flashing an application to V2M MPS2
-----------------------------------

The sample application hello_world is being used in this tutorial:

.. code-block:: console

   $ZEPHYR_BASE/samples/hello_world

To build the Zephyr kernel and application, enter:

.. code-block:: console

   $ cd $ZEPHYR_BASE
   $ . zephyr-env.sh
   $ cd $ZEPHYR_BASE/samples/hello_world/
   $ make BOARD=mps2_an385

Connect the V2M MPS2 to your host computer using the USB port and you should
see a USB connection which exposes a Mass Storage and a USB Serial Port.
Copy the generated zephyr.bin in the exposed drive.
Reset the board and you should be able to see on the corresponding Serial Port
the following message:

.. code-block:: console

   Hello World! arm


.. _V2M MPS2 Website:
   https://developer.mbed.org/platforms/ARM-MPS2/

.. _MPS2 Technical Reference Manual (TRM):
   http://infocenter.arm.com/help/topic/com.arm.doc.100112_0200_05_en/versatile_express_cortex_m_prototyping_systems_v2m_mps2_and_v2m_mps2plus_technical_reference_100112_0200_05_en.pdf

.. _Application Note AN385:
   http://infocenter.arm.com/help/topic/com.arm.doc.dai0385c/DAI0385C_cortex_m3_on_v2m_mps2.pdf

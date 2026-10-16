.. zephyr:board:: beagley_ai

Overview
********

BeagleY-AI is a computational platform powered by TI AM67A (J722S) SoC, which is
targeted for automotive applications.

Hardware
********
BeagleY-AI is powered by TI AM67A (J722S) SoC, which has two domains (Main,
MCU). This document gives overview of Zephyr running on both Cortex R5.

L1 Memory System
----------------
BeagleY-AI defaults to single-core mode for the R5 subsystem. Changes in that
will impact the L1 memory system configuration.

* 32KB instruction cache
* 32KB data cache
* 64KB tightly-coupled memory (TCM)
  * 32KB TCMA
  * 32KB TCMB

Region Address Translation
--------------------------
The RAT module performs a region based address translation. It translates a
32-bit input address into a 36-bit output address. Any input transaction that
starts inside of a programmed region will have its address translated, if the
region is enabled.

VIM Interrupt Controller
------------------------
The VIM aggregates device interrupts and sends them to the R5F CPU(s). The VIM
module supports 512 interrupt inputs per R5F core. Each interrupt can be either
a level or a pulse (both active-high). The VIM has two interrupt outputs per core
IRQ and FIQ.

Supported Features
******************
One instance of each common peripheral is enabled by default (nRF DK style);
other instances stay ``disabled`` until needed.

HAT pin names match `pinout.beagleboard.io <https://pinout.beagleboard.io/>`_.

Enabled by default:

* **UART1** console (GPIO14/15, HAT pins 8/10): ``&uart1``
* **GPIO** for on-board USR LEDs (``led0`` / ``led1``): ``&main_gpio0_0``
* **I2C1** (GPIO2/GPIO3, HAT pins 3/5): ``&mcu_i2c0`` / aliases ``i2c1``, ``i2c-0``
* **MCU SPI0** (GPIO10/9/11/8, HAT pins 19/21/23/24): ``&mcu_spi0`` / alias ``spi-0``

Sample overlays:

* ``samples/sensor/tmp112/boards/beagley_ai_j722s_*_r5f0_0.overlay``
* ``tests/drivers/spi/spi_loopback/boards/beagley_ai_j722s_*_r5f0_0.overlay``

Inactive pinmux lives in ``beagley_ai_j722s_r5f0_0-pinctrl.dtsi`` (same pattern as
Nordic ``*-pinctrl.dtsi``). Extra GPIO banks and optional I2C4 pin groups
(``hat_15_i2c`` / ``hat_22_i2c``) are defined there; enable the matching
controller with ``status = "okay"`` when you need them
(e.g. ``&main_gpio1_0``, ``&mcu_gpio0``).

Avoid HAT pins 27/28 (GPIO0/GPIO1, I2C0 / WKUP_I2C0); shared with PMIC and
board EEPROM. Future versions will also support a console over RPmsg.

.. zephyr:board-supported-hw::

Running Zephyr
**************

The AM67A does not have a separate flash for the R5 core. Because of this
an A53 core has to load the program for the R5 core to the right memory
address, set the PC and start the processor.
This can be done from Linux on the A53 core via remoteproc.

This is the memory mapping from A53 to the memory usable by the R5. Note that
the R5 core always sees its local TCMA at address 0x00000000 and its TCMB0
at address 0x41010000.

The A53 Linux configuration allocates a region in DDR that is shared with
the R5. The amount of the allocation can be changed in the Linux device tree.
Note that BeagleY-AI has 4GB of DDR.

+-------------------+---------------+--------------+--------+
| Region            | Addr from A53 | MAIN R5F     | Size   |
+===================+===============+==============+========+
| ATCM              | 0x0078400000  | 0x0000000000 | 32KB   |
+-------------------+---------------+--------------+--------+
| BTCM              | 0x0078500000  | 0x0041010000 | 32KB   |
+-------------------+---------------+--------------+--------+
| DDR Shared Region | 0x00A2000000  | 0x00A2000000 | 16MB   |
+-------------------+---------------+--------------+--------+

+-------------------+---------------+--------------+--------+
| Region            | Addr from A53 | MCU R5F      | Size   |
+===================+===============+==============+========+
| ATCM              | 0x0079000000  | 0x0000000000 | 32KB   |
+-------------------+---------------+--------------+--------+
| BTCM              | 0x0079020000  | 0x0041010000 | 32KB   |
+-------------------+---------------+--------------+--------+
| DDR Shared Region | 0x00A1000000  | 0x00A1000000 | 16MB   |
+-------------------+---------------+--------------+--------+

Steps to run the image
----------------------
Here is an example for the :zephyr:code-sample:`hello_world` application
targeting the MAIN domain Cortex R5F on BeagleY-AI:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: beagley_ai/j722s/main_r5f0_0
   :goals: build

For the MCU domain Cortex R5F on BeagleY-AI:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: beagley_ai/j722s/mcu_r5f0_0
   :goals: build

To load the image:

| Copy Zephyr image to the /lib/firmware/ directory.
| ``cp build/zephyr/zephyr.elf /lib/firmware/``
|
| Ensure the Core is not running.
| ``echo stop > /dev/remoteproc/j7-{main,mcu}-r5f0_0/state``
|
| Configuring the image name to the remoteproc module.
| ``echo zephyr.elf > /dev/remoteproc/j7-{main,mcu}-r5f0_0/firmware``
|
| Once the image name is configured, send the start command.
| ``echo start > /dev/remoteproc/j7-{main,mcu}-r5f0_0/state``
|
| (Older docs used ``am67a-*`` names; current Beagle Debian images expose ``j7-*``.)
| Also available via ``/sys/class/remoteproc/remoteproc*``.

Console
-------
The Zephyr on BeagleY-AI Cortex-R5F uses UART 1 (HAT pins 8-TX, 10-RX)
as console.

References
**********
* `BeagleY-AI Homepage <https://beagley.ai>`_
* `AM67A TRM <https://www.ti.com/lit/zip/sprujb3>`_
* `Pinout guide <https://pinout.beagley.ai/>`_
* `Documentation <https://docs.beagleboard.org/latest/boards/beagley/ai>`_

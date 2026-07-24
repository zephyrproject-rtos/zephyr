.. zephyr:board:: sparrowhawk_rcar_v4h

Overview
********
Retronix Sparrow Hawk Single Board Computer (SBC) is powered by the latest Renesas R-Car V4H
System-on-Chip. Sparrow Hawk focuses on robotics, industrial automation, and rapid prototyping,
offering a highly flexible and cost-effective development platform.

The R-Car V4H system-on-chip is tailored for central processing for advanced driver-assistance (ADAS)
and automated driving (AD) systems. The R-Car V4H achieves deep learning performance of up to 34 TOPS
(Tera Operations Per Second), enabling high-speed image recognition and processing of surrounding
objects by automotive cameras, radar, and Light Detection and Ranging (LiDAR).

Hardware
********

Hardware capabilities of the board can be found on `Retronix Sparrow Hawk`_ page.
All the features of Renesas R-Car V4H SoC are described in the product page `Renesas R-Car V4H`_.

Supported Features
==================

We support Zephyr running on Cortex R52 processor that is provided for RTOS purpose.

.. zephyr:board-supported-hw::

Connections and IOs
===================

For the connections and IO interfaces, refer to the official page `Retronix Sparrow Hawk`_

UART
----

Here is information about serial ports provided on Sparrow Hawk board :

+--------------------------+--------------------+--------------------+-------------+---------------------------+
|    Software interface    | Physical Interface | Hardware Interface | Converter   |    Usage Note             |
+==========================+====================+====================+=============+===========================+
| /tty/USBx, COMn (lower)  | CN4 USB Port       |       HSCIF0       | FT2232H     | Used by U-Boot and Linux  |
+--------------------------+--------------------+--------------------+-------------+---------------------------+
| /tty/USBy, COMm (higher) | CN4 USB Port       |       HSCIF1       | FT2232H     | Default for Zephyr        |
+--------------------------+--------------------+--------------------+-------------+---------------------------+

.. note::
   By default, Zephyr console output is assigned to HSCIF1 with 921600 8N1 without
   hardware flow control.

Pins Configuration
------------------

The below table describes the pin layout and the supported function of Sparrow Hawk board.

+--------------------------+-----+-----+------------------------+
|      Pin function        | Pin number|      Pin function      |
+==========================+=====+=====+========================+
|          3.3V            |  1  |  2  |          5V            |
+--------------------------+-----+-----+------------------------+
|                          |  3  |  4  |          5V            |
+--------------------------+-----+-----+------------------------+
|                          |  5  |  6  |          GND           |
+--------------------------+-----+-----+------------------------+
|                          |  7  |  8  |                        |
+--------------------------+-----+-----+------------------------+
|                          |  9  | 10  |                        |
+--------------------------+-----+-----+------------------------+
|         GPIO 17          | 11  | 12  |        PWM3            |
+--------------------------+-----+-----+------------------------+
|         GPIO 27          | 13  | 14  |                        |
+--------------------------+-----+-----+------------------------+
|         GPIO 22          | 15  | 16  |        GPIO 23         |
+--------------------------+-----+-----+------------------------+
|                          | 17  | 18  |        GPIO 24         |
+--------------------------+-----+-----+------------------------+
|                          | 19  | 20  |                        |
+--------------------------+-----+-----+------------------------+
|                          | 21  | 22  |        GPIO 25         |
+--------------------------+-----+-----+------------------------+
|                          | 23  | 24  |                        |
+--------------------------+-----+-----+------------------------+
|                          | 25  | 26  |                        |
+--------------------------+-----+-----+------------------------+
|                          | 27  | 28  |                        |
+--------------------------+-----+-----+------------------------+
|                          | 29  | 30  |        PWM0            |
+--------------------------+-----+-----+------------------------+
|         GPIO 6           | 31  | 32  |                        |
+--------------------------+-----+-----+------------------------+
|         PWM1             | 33  | 34  |                        |
+--------------------------+-----+-----+------------------------+
|                          | 35  | 36  |        GPIO 16         |
+--------------------------+-----+-----+------------------------+
|         GPIO 26          | 37  | 38  |                        |
+--------------------------+-----+-----+------------------------+
|                          | 39  | 40  |                        |
+--------------------------+-----+-----+------------------------+

GPIO
----

There are 9 pins can be used as GPIO. Available pin names are listed in the
Pins Configuration section.

**Supported features:**

* GPIO Output: Push-pull, open-drain with internal pull-up resistor.
  Active high and active low polarity.
* GPIO Input: Only input with internal pull-up. Active high and active low polarity.
* Input interrupt with level and edge trigger.

To use GPIO, you must activate GPIO nodes (gpio0, gpio1, gpio2) in Devivetree overlay:

.. code-block:: devicetree

   &gpio0 {
      status = "okay";
   };

   &gpio1 {
      status = "okay";
   };

   &gpio2 {
      status = "okay";
   };

Get GPIO pin references to ``gpio_dt_spec`` struct using node label:

.. code-block:: C

   static const struct gpio_dt_spec gp16 =
      GPIO_DT_SPEC_GET(DT_NODELABEL(gp16), gpios);

Pulse Width Modulation (PWM)
----------------------------

V4H Sparrow Hawk provides 3 pins for PWM function. You must activate the PWM
node in your Devicetree overlay:

.. code-block:: devicetree

   &pwm {
       status = "okay";
   };

The following output pin nodes are defined: ``pwm0``, ``pwm1``, and ``pwm3``.
Obtain a PWM device reference using a ``pwm_dt_spec`` structure:

.. code-block:: C

   const struct pwm_dt_spec pwm0 = PWM_DT_SPEC_GET(DT_NODELABEL(pwm0));

This provides a fully initialized pwm_dt_spec that you can use with Zephyr's PWM
API functions such as: ``pwm_set_dt()``, ``pwm_set_pulse_dt()``, ``pwm_is_ready_dt``.

When using API functions with device instance and channel (like ``pwm_set_cycles()``,
``pwm_set()``), ensure that the PWM pin is mapped with the SoC PWM channel as follows:

* PWM0: Channel 6
* PWM1: Channel 7
* PWM3: Channel 1

Using incorrect channel does not generate any output.

Programming and Debugging
*************************

You can build the applications as usual. This is the example for Hello World:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: sparrowhawk_rcar_v4h/r8a779g0/r52
   :goals: build

Configuring a Console
=====================

Connect a USB cable from your PC to CN4 USB port. There are two COM ports (or /tty/USB devices) available.
Both of them are used for booting procedure. Use the following settings with your serial terminal of choice
(minicom, putty, etc.):

* Speed: 921600
* Data: 8 bits
* Parity: None
* Stop bits: 1

Flashing
========

The board does not support flashing Zephyr image. However, the image writing and loading
can be done with U-Boot.

Make sure you have already flashed the board with U-Boot, see the guideline at: `Flash_loader`_,
section "3.2.2. Flashing loader".
Connect the terminal software to the serial port of HSCIF0 (lower /tty/USBx or COMn).
Powerup the board by pressing SW1 switch. You would see the boot log:

.. code-block:: console

   U-Boot SPL 2025.07 (Aug 07 2025 - 04:02:12 +0000)
   Trying to boot from SPI


   U-Boot 2025.07 (Aug 07 2025 - 04:02:12 +0000)

   CPU:   Renesas Electronics R8A779G0 rev 3.0
   Model: Retronix Sparrow Hawk board based on r8a779g3
   DRAM:  2 GiB (total 16 GiB)
   Core:  87 devices, 23 uclasses, devicetree: separate
   MMC:   mmc@ee140000: 0
   Loading Environment from SPIFlash... SF: Detected w77q51nw with page size 256 Bytes, erase size 64 KiB, total 64 MiB
   OK
   In:    serial@e6540000
   Out:   serial@e6540000
   Err:   serial@e6540000
   Net:   eth0: ethernet@e6800000
   =>

.. note::
   From U-Boot log, ``r8a779g3`` is the part number of the revision 3.0 of V4H SoC (R8A779G0).
   This number may change with future board version.

Press any key to stop the booting and continue at the U-Boot prompt.

Method 1: Using TFTP to transfer Zephyr image
---------------------------------------------

This assumes that you have already installed a TFTP server in the host PC.
Put the image bin file ``build/zephyr/zephyr.bin`` inside TFTP root directory. Run these
U-Boot commands:

.. code-block:: console

   => setenv ipaddr <board.ip>
   => setenv serverip <tftp.server.ip>
   => tftp 0x40040000 zephyr.bin
   => rproc init; rproc load 0 0x40040000 0x200000; rproc start 0

Method 2: Using serial to transfer Zephyr image
-----------------------------------------------

Some terminal software support transferring file via serial using Kermit protocol. Use this U-Boot commands:

.. code-block:: console

   => loadb 0x40040000 921600
   ## Ready for binary (kermit) download to 0x40040000 at 921600 bps...
   (Transfer zephyr.bin after this line)
   ## Total Size      = 0x00009f2c = 40748 Bytes
   ## Start Addr      = 0x40040000
   => rproc init; rproc load 0 0x40040000 0x200000; rproc start 0

You should see Zephyr boot log in the terminal of HSCIF1:

.. code-block:: console

   *** Booting Zephyr OS build v4.2.0-4945-g8fc6351ef451 ***
   Hello World! sparrowhawk_rcar_v4h/r8a779g0/r52

References
**********

- `Renesas R-Car V4H`_
- `Retronix Sparrow Hawk`_

.. _Renesas R-Car V4H:
   https://www.renesas.com/en/products/r-car-v4h

.. _Retronix Sparrow Hawk:
   https://rcar-community.github.io/Sparrow-Hawk/index.html

.. _Flash_loader:
   https://rcar-community.github.io/Sparrow-Hawk/BSP/yocto_bsp.html

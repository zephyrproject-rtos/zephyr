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

Counter
-------

R-Car V4H provides 3 timer modules that can be used with Counter API. Each timer
support the following features:

* 32-bit counter.
* Counter frequency is fixed at 2,082,500 Hz
* 4 channels each module.
* Each channel provide an independent alarm interrupt.

You can enable the timer instances through a Devicetree overlay:

.. code-block:: devicetree

   &cmt1 {
      status = "okay";
   };

   &cmt2 {
      status = "okay";
   };

   &cmt3 {
      status = "okay";
   };


GPIO
----

There are 9 pins can be used as GPIO. Avialable pin names are listed in the
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

For convenience, you can create a user-defined node to associate a name with pin number:

.. code-block:: devicetree

   #include <zephyr/dt-bindings/gpio/gpio.h>

   / {
      zephyr,user {
         gp6-gpios  = <&sparrowhawkgpio  6 GPIO_ACTIVE_HIGH>;
         gp16-gpios = <&sparrowhawkgpio 16 GPIO_ACTIVE_LOW>;
         gp17-gpios = <&sparrowhawkgpio 17 GPIO_ACTIVE_HIGH>;
         gp22-gpios = <&sparrowhawkgpio 22 GPIO_ACTIVE_LOW>;
         gp23-gpios = <&sparrowhawkgpio 23 GPIO_ACTIVE_HIGH>;
         gp24-gpios = <&sparrowhawkgpio 24 GPIO_ACTIVE_LOW>;
         gp25-gpios = <&sparrowhawkgpio 25 GPIO_ACTIVE_HIGH>;
         gp26-gpios = <&sparrowhawkgpio 26 GPIO_ACTIVE_LOW>;
         gp27-gpios = <&sparrowhawkgpio 27 GPIO_ACTIVE_HIGH>;
      };
   };

and get references to it using ``gpio_dt_spec`` struct:

.. code-block:: C

   static const struct gpio_dt_spec gp16 =
      GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, gp16_gpios);

Pulse Width Modulation (PWM)
----------------------------

V4H Sparrow Hawk provides 3 pins for PWM function. You must activate the PWM
node in your Devicetree overlay:

.. code-block:: devicetree

   &pwm {
       status = "okay";
   };

The pin names are defined: ``pwm0_io``, ``pwm1_io``, ``pwm3_io``. Get the reference
using pwm_dt_spec struct:

.. code-block:: devicetree

   const struct pwm_dt_spec pwm0 =
      PWM_DT_SPEC_GET_BY_NAME(DT_NODELABEL(pwm_io), pwm0_io);

This provides a fully initialized pwm_dt_spec that you can use with Zephyr's PWM
API functions such as: ``pwm_set_dt()``, ``pwm_set_cycles_dt()``, ``pwm_is_ready_dt()``

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
   => rproc init; rproc load 0:3 0x40040000 0x200000; rproc start 0

Method 2: Using serial to transfer Zephyr image
-----------------------------------------------

Some terminal software support transferring file via serial using Kermit protocol. Use this U-Boot commands:

.. code-block:: console

   => loadb 0x40040000 921600
   ## Ready for binary (kermit) download to 0x40040000 at 921600 bps...
   (Transfer zephyr.bin after this line)
   ## Total Size      = 0x00009f2c = 40748 Bytes
   ## Start Addr      = 0x40040000
   => rproc init; rproc load 0:3 0x40040000 0x200000; rproc start 0

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

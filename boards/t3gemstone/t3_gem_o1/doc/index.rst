.. zephyr:board:: t3_gem_o1

Overview
********

The T3 Gemstone O1 is a single-board computer powered by the TI AM67A (J722S) SoC.
T3 Gemstone O1 development board are built from open-source hardware and software components,
and they pair AI-accelerated hardware with a Debian-based GNU/Linux operating system optimized for real-time applications,
making them suitable for manned and unmanned systems, IoT, robotics, and many other fields.

Hardware
********
T3 Gemstone O1 is powered by the TI AM67A (J722S) SoC, which has two domains
(Main, MCU). This document gives an overview of Zephyr running on both Cortex
R5F cores.

The board also features integrated sensors, including the ICM-20948 9-axis motion sensor,
LPS22DF barometric pressure sensor, and HDC2010 temperature and humidity sensor.
Connectivity includes an onboard CAN FD transceiver, two 4-lane MIPI CSI camera interfaces,
and a 4-lane MIPI DSI display interface — one CSI port is multiplexed with DSI,
so either dual CSI or one CSI plus one DSI can be used.
An I2C real-time clock with a battery input preserves system time across power cycles,
while 32 GB of onboard eMMC storage allows the board to run without a microSD card.
Programmable red and green user LEDs are also available.

L1 Memory System
----------------
T3 Gemstone O1 defaults to single-core mode for the R5 subsystem. Changes in
that will impact the L1 memory system configuration.

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
The board configuration supports a console UART and an I2C bus on the 40-pin
header, the MAIN domain GPIO controllers, the on-board I2C and SPI buses
along with the sensors on them, and a console over RPmsg.

.. zephyr:board-supported-hw::

User LEDs
---------
Two user LEDs are connected to the MAIN domain GPIO1 controller. They are
exposed through the ``led0`` and ``led1`` aliases so that samples such as
:zephyr:code-sample:`blinky` work out of the box.

+-------+-------+----------+-------------+
| Alias | Color | Pin      | Polarity    |
+=======+=======+==========+=============+
| led0  | Green | GPIO1_52 | Active high |
+-------+-------+----------+-------------+
| led1  | Red   | GPIO1_53 | Active low  |
+-------+-------+----------+-------------+

The red LED lights up while its pin is left as an input, which is the state
the GPIO driver leaves it in until an application configures it. Applications
that do not drive the red LED should configure it as an inactive output to
keep it off.

On-board sensors
----------------
Two of the sensors of the board are described by the board configuration and
can be read with the sensor shell or with the generic sensor samples.

+----------+---------------------------+-------------------------+
| Sensor   | Measures                  | Connection              |
+==========+===========================+=========================+
| HDC2010  | Temperature and humidity  | MAIN I2C0, address 0x41 |
+----------+---------------------------+-------------------------+
| LPS22DF  | Pressure and temperature  | MCU SPI0, chip select 1 |
+----------+---------------------------+-------------------------+

The LPS22DF is given an output data rate of 10 Hz. The part comes out of
reset powered down and returns zeroes until an output data rate is set,
either from the devicetree or at runtime.

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
Note that T3 Gemstone O1 has 4GB of LPDDR4.

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
targeting the MAIN domain Cortex R5F on T3 Gemstone O1:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: t3_gem_o1/j722s/main_r5f0_0
   :goals: build

For the MCU domain Cortex R5F on T3 Gemstone O1:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: t3_gem_o1/j722s/mcu_r5f0_0
   :goals: build

To load the image, use the remoteproc interface in sysfs. Identify the target
core first. The MAIN domain R5F is ``78400000.r5f`` and the MCU domain R5F is
``79000000.r5f``:

.. code-block:: console

   # grep -H . /sys/class/remoteproc/remoteproc*/name
   /sys/class/remoteproc/remoteproc2/name:79000000.r5f
   /sys/class/remoteproc/remoteproc3/name:78400000.r5f

Copy the Zephyr image to ``/lib/firmware/``, point the remoteproc instance at
it and start the core. Replace ``remoteprocN`` with the instance found above:

.. code-block:: console

   # cp build/zephyr/zephyr.elf /lib/firmware/
   # echo zephyr.elf > /sys/class/remoteproc/remoteprocN/firmware
   # echo start > /sys/class/remoteproc/remoteprocN/state

.. warning::

   Do not stop the MAIN domain R5F with ``echo stop``. The core is not
   given back afterwards: the remoteproc instance refuses to start again
   and a warm reboot does not recover it, only a power cycle does. To load
   another image, copy it to ``/lib/firmware/``, reboot the board and start
   the core again. The MCU domain R5F can be stopped and started freely.

Some distributions expose the same interface under ``/dev/remoteproc/``
through udev rules, but this is not available on every image.

Console
-------
Zephyr on the T3 Gemstone O1 Cortex-R5F uses UART 1 (40-pin GPIO header
pins 8-TX, 10-RX) as console.

A console over RPmsg is also available and requires no extra cabling. Build
the :zephyr:code-sample:`openamp-rsc-table` sample, which enables the RPmsg
shell backend, and load it as described above. Once the core is running,
Linux creates the corresponding rpmsg channels:

.. code-block:: console

   $ ls /sys/bus/rpmsg/devices/
   virtio1.rpmsg-client-sample.-1.1025
   virtio1.rpmsg-tty.-1.1024
   virtio1.rpmsg-tty.-1.1026

Binding the ``rpmsg-tty`` channel to a Linux driver such as ``rpmsg_tty`` or
``rpmsg_char`` gives access to the Zephyr shell from the Linux side.

Debugging
---------
The board provides an ARM Cortex 10-pin JTAG connector which can be used to
debug the Cortex-R5F cores.

References
**********
* `T3 Gemstone Homepage <https://t3gemstone.org>`_
* `T3 Gemstone O1 documentation <https://docs.t3gemstone.org>`_
* `T3 Gemstone on GitHub <https://github.com/t3gemstone>`_
* `AM67A TRM <https://www.ti.com/lit/zip/sprujb3>`_

.. zephyr:board:: zub1cg_r5

Overview
********
This configuration provides support for the real-time processing unit (RPU) on the Avnet
ZUBoard 1CG development board. It can operate as follows:

* Two independent R5 cores with their own TCMs (tightly coupled memories)
* Or, as a single dual lock step unit with double the TCM size.

This processing unit is based on an ARM Cortex-R5 CPU, it also enables the following devices:

* ARM PL-390 Generic Interrupt Controller
* Xilinx Zynq TTC (Cadence TTC)
* Xilinx Zynq UART
* Xilinx Zynq I2C (Cadence I2C)

Hardware
********
Supported Features
==================

.. zephyr:board-supported-hw::

Devices
========
System Timer
------------

This board configuration uses a system timer tick frequency of 1000 Hz.

Serial Port
-----------

This board configuration uses a single serial communication channel with the
on-chip UART1.

Memories
--------

Flash, DDR and OCM memory regions are defined in the DTS file.
Vectors are placed in the ATCM region, while all other code plus
data of the application will be loaded in the sram1 region,
which points to the DDR memory. The sram1 region is defined to
match the Petalinux rproc_0_reserved region so the Zephyr application
can be launched from Petalinux using Remoteproc. The ocm0 memory
area is currently available for usage, although nothing is placed
there by default.

Known Problems or Limitations
==============================

The following platform features are unsupported:

* Dual-redundant Core Lock-step (DCLS) execution is not supported yet.
* Only the first core of the R5 subsystem is supported.
* Xilinx Zynq TTC driver does not support tickless mode operation.
* The Cortex-R5 and the Cortex-A53 shares the same UART controller, more details below.

Programming and Debugging
*************************

Currently the best way to run this sample is by loading it through remoteproc
from the APU, running Linux, to the RPU, assuming the target board has a compatible
Linux kernel. Users can make use of Avnet's pre-built Petalinux BSP as a starting
point to enable remoteproc support, it is based around 6.6 Xilinx maintained kernel.
Building Petalinux is outside the scope of this document, see the `Avnet ZUBoard 1CG
Product Page`_ for more details. 

After getting the Linux image running on the target board, build a Zephyr application,
such as the hello world sample shown below:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: zub1cg_r5
   :goals: build

Due to a hardware limitation, both Linux and Zephyr share the same UART
controller, meaning when the Zephyr application is started it will takeover the
console from Linux.

To avoid this limitation when accessing the Linux shell, the best approach is to
connect to the board using ``ssh`` over the network (not using the FTDI
USB interface on the board), with the dev board and the host computer
connected to the same network.

Alternatively, it is possible to use the other PS UART controller for either Linux
or Zephyr, but it must be enabled in Vivado and routed via MIO/EMIO to accessible
pins on the board.

Assuming you are using the default ``petalinux`` user from the Xilinx
reference image , open a terminal on the host machine and ssh into the
development board with the board's IP address (found via ``ifconfig``):

.. code-block:: console

        $ ssh petalinux@<board-ip-address>

The initial password should be ``petalinux``. On another terminal deploy
the Zephyr application ``.elf`` file using utility like the ``scp`` or ``rsync``,
for example:

.. code-block:: console

        $ scp /path/to/zephyr_app_elf_file  petalinux@<board-ip-address>:/home/petalinux

After that move the file to ``/lib/firmware`` directory, then you be able to start the firmware
on the desired RPU via remoteproc with:

.. code-block:: console

        $ sudo -i # You need to operate the remoteproc as root
        $ echo zephyr.elf > /sys/class/remoteproc/remoteproc0/firmware
        $ echo start > /sys/class/remoteproc/remoteproc0/state

With another terminal connected to UART1 on the host machine
(available via one of the tty ports with the on-board FTDI chip),
you should see the Zephyr application running:

.. code-block:: console

        *** Booting Zephyr OS build v4.1.0-5065-gc3ec37aa2e47 ***
        Hello World! zub1cg_r5/zynqmp_rpu

It is also possible to program and debug this program via AMD's Vitis development platform.
Create a new platform project based on your hardware XSA file (from Vivado), import a new
empty application template, and in the debug configuration point the application to the
zephyr.elf binary.

References
**********

.. target-notes::

.. _Avnet ZUBoard 1CG Product Page:
    https://www.avnet.com/americas/products/avnet-boards/avnet-board-families/zuboard-1cg/

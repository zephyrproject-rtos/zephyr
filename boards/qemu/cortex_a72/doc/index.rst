.. zephyr:board:: qemu_cortex_a72

Overview
********

This board configuration will use QEMU to emulate a Raspberry Pi 4B hardware
platform based on the Broadcom BCM2711 SoC.

This configuration provides support for an ARM Cortex-A72 CPU and these devices:

* ARM architected timer
* ARM GIC-400 interrupt controller
* ARM PL011 UART controller
* Broadcom BCM2835 AUX UART controller
* Broadcom BCM2835 clock/reset manager (CPRMAN)
* Broadcom BCM2835 DMA controller
* Broadcom BCM2835 framebuffer controller
* Broadcom BCM2835 GPIO controller
* Broadcom BCM2835 hardware random number generator
* Broadcom BCM2835 I2C (BSC) controller
* Broadcom BCM2835 mailbox controller
* Broadcom BCM2835 SD/MMC host controller
* Broadcom BCM2835 SPI controller
* Broadcom BCM2835 system timer
* Broadcom BCM2835 thermal sensor
* Broadcom BCM2835 USB host controller
* Raspberry Pi VideoCore firmware property interface
* Synopsys DWC2 USB2 host controller

Hardware
********

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Use this configuration to run basic Zephyr applications and kernel tests in the
QEMU emulated environment, for example, with the
:zephyr:code-sample:`synchronization` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: qemu_cortex_a72
   :goals: run

This will build an image with the synchronization sample app, boot it using
QEMU, and display the following console output:

.. code-block:: console

   *** Booting Zephyr OS build v4.4.0-2646-g284aabd6a6c1 ***
   thread_a: Hello World from cpu 0 on qemu_cortex_a72!
   thread_b: Hello World from cpu 0 on qemu_cortex_a72!
   thread_a: Hello World from cpu 0 on qemu_cortex_a72!
   thread_b: Hello World from cpu 0 on qemu_cortex_a72!
   thread_a: Hello World from cpu 0 on qemu_cortex_a72!
   thread_b: Hello World from cpu 0 on qemu_cortex_a72!
   thread_a: Hello World from cpu 0 on qemu_cortex_a72!
   thread_b: Hello World from cpu 0 on qemu_cortex_a72!

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.

Debugging
=========

Refer to the detailed overview about :ref:`application_debugging`.

To attach GDB over the QEMU GDB stub (enabled via ``-s`` flag):

.. code-block:: console

   gdb-multiarch build/zephyr/zephyr.elf \
     -ex "target remote localhost:1234"

References
**********

.. target-notes::

1. `Raspberry Pi 4 Model B Datasheet <https://datasheets.raspberrypi.com/rpi4/raspberry-pi-4-datasheet.pdf>`_
2. `BCM2711 ARM Peripherals <https://datasheets.raspberrypi.com/bcm2711/bcm2711-peripherals.pdf>`_

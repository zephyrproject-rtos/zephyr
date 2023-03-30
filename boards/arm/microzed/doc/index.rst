.. microzed:

Avnet MicroZed
#############

Overview
********

The Avnet MicroZed is a system on module (SOM) based on the Xilinx Zynq-7000 SoC. It
connects various peripherals with the SoC, which integrates two Cortex-A9 cores, FPGA
(Programmable Logic) and several interfaces.

.. figure:: microzed.jpg
   :width: 800px
   :align: center
   :alt: Board Name

   Avnet MicroZed (Credit: Avnet)

Hardware
********

MicroZed has the following key features:

- Dual core Cortex-A9 running up to 866 MHz
- 1 GB DDR3 memory
- 128 MB QSPI Flash
- 10/100/1000 Ethernet
- Two 100-pin MicroHeader ports
- USB-UART bridge via Micro-USB port
- Digilent PMOD compatible 2x6 header
- MicroSD card slot
- Reset button, user button, LED
- 33.333 MHz on-board oscillator

MicroZed supports the 7010 and 7020 models of the Zynq-7000 SoC.

Supported Features
==================

Zephyr supports these interfaces:

.. list-table::
   :header-rows: 1

   * - Interface
     - Controller
     - Driver
   * - GICv1
     - on-chip
     - Arm Generic Interrupt Controller v1
   * - ARCH TIMER
   	 - on-chip
     - ARM architecture timer
   * - Triple Timer Counters
     - on-chip
	 - Xilinx PSTTC timer
   * - PINCTRL
	 - on-chip
	 - pinctrl
   * - GPIO
	 - on-chip
	 - gpio
   * - UART
     - on-chip
	 - serial port-polling, serial port-interrupt
   * - Ethernet
   	 - on-chip
	 - eth-xilinx-gem

Programming and Debugging
*************************

Flashing
========

The following example shows how to boot Zephyr using Das U-Boot, a second-stage
bootloader, which officially supports this board. See the U-boot documentation
for instructions on how to prepare and flash the bootloader image onto an SD card.

Here, we are running the :ref:`hello_world` sample. Once the system is built, copy ``build/zephyr/zephyr.bin``
file to the SD card, plug the card to the MicroZed board and turn it on. After U-boot
starts up and offers us a shell prompt, run the Zephyr image:

.. code-block:: console

    Zynq> fatload mmc 0 0x0 zephyr.bin
    Zynq> icache flush; icache off; dcache flush; dcache off; go 0x0
    ## Starting application at 0x00000000 ...
    *** Booting Zephyr OS zephyr-v3.3.0-1994-g177434ef845f ***
    Hello World! microzed

Alternatively, the image may be downloaded from a TFTP server:

.. code-block:: console

    Zynq> tftpboot 0x0 ${serverip}:/srv/tftp/zephyr.bin
    Zynq> icache flush; icache off; dcache flush; dcache off; go 0x0
    ## Starting application at 0x00000000 ...
    *** Booting Zephyr OS zephyr-v3.3.0-1994-g177434ef845f ***
    Hello World! microzed

References
**********

.. _MicroZed TRM: https://www.avnet.com/wps/wcm/connect/onesite/58eaef36-f0b2-4dd4-8440-540bdc2acd3d/5276-MicroZed-HW-UG-v1-7-V1.pdf?MOD=AJPERES&CACHEID=ROOTWORKSPACE.Z18_NA5A1I41L0ICD0ABNDMDDG0000-58eaef36-f0b2-4dd4-8440-540bdc2acd3d-nDjezWU

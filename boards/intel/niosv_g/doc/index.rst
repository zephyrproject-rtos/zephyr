.. _niosv_g:

INTEL FPGA niosv_g
####################

Overview
********

niosv_g board is based on Intel FPGA Design Store Nios® V/g Hello World Example Design system and this complete system is consisted of following IP blocks:

.. code-block:: console

	Nios® V/g Processor Intel® FPGA IP
	JTAG UART Intel® FPGA IP
	On-Chip Memory Intel® FPGA IP

Nios® V/g hello world example design system
===========================================

Prebuilt Nios® V/g hello world example design system is available in Intel FPGA Design store.
- https://www.intel.com/content/www/us/en/support/programmable/support-resources/design-examples/design-store.html?s=Newest

For example, Arria10 Nios® V/g processor example design system prebuilt files can be downloaded from following link.
- https://www.intel.com/content/www/us/en/design-example/776196/intel-arria-10-fpga-hello-world-design-on-nios-v-g-processor.html

ready_to_test/top.sof file is the prebuilt SRAM Object File for hello world example design system after the downloaded PAR files extracted successfully.

Create Nios® V/g processor example design system in FPGA
========================================================

Please use Intel Quartus Programmer tool to program Nios® V/g processor based system into the FPGA and execute application.

In order to create the Nios® V/g processor inside the FPGA device, please download the generated .sof file onto the board with the following command.

.. code-block:: console

	quartus_pgm -c 1 -m JTAG -o "p;top.sof@1"

.. code-block:: console

	Note:
	-c 1 is referring to JTAG cable number connected to the Host Computer.
	@1 is referring to device index on the JTAG Chain and may differ for your board.
	top.sof is referring to Nios® V/m processor based system SRAM Object File.

Download Zephyr elf file and run application
============================================

To download the Zephyr Executable and Linkable Format .elf file, please use the niosv-download command within Nios V Command Shell environment.

.. code-block:: console

	niosv-download -g <elf file>

Use the JTAG UART terminal to print the stdout and stderr of the Nios® V/g processor system.

.. code-block:: console

	juart-terminal

Similar message shown below should be appeared in the JTAG UART terminal when using hello world sample code:

.. code-block:: console

	*** Booting Zephyr OS build zephyr-vn.n.nn  ***
	Hello World! niosv_g

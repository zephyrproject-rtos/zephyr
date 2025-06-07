.. zephyr:board:: mps4

Overview
********

The MPS4 board configuration is used by Zephyr applications that run
on the MPS4 board.

`Corstone-320 FVP`_ is an Arm reference subsystem for
secure System on Chips containing an Armv8.1-M Cortex-M85 processor,
LCM, KMU and SAM IPs and, an Ethos-U85 neural network processor.
They are available free of charge for Linux and Windows systems.
The FVPs have been selected for simulation since they provide access to the
Ethos-U85 NPU, which is unavailable in QEMU or other simulation platforms.


Zephyr board options
====================

.. tabs::

  .. tab:: MPS4 Corstone-320 (FVP)

   The MPS4 FVP is an SoC with Cortex-M85 architecture. Zephyr provides support
   for building for both Secure and Non-Secure firmware.

   The BOARD options are summarized below:

   +-------------------------------+-----------------------------------------------+
   |   BOARD                       | Description                                   |
   +===============================+===============================================+
   | ``mps4/corstone320/fvp``      | For building Secure (or Secure-only) firmware |
   +-------------------------------+-----------------------------------------------+
   | ``mps4/corstone320/fvp/ns``   | For building Non-Secure firmware              |
   +-------------------------------+-----------------------------------------------+

   FPGA Usage:
    - N/A.

   FVP Usage:
    - To run with the FVP, first set environment variable ``ARMFVP_BIN_PATH`` before using it. Then you can run it with ``west build -t run``.

    .. code-block:: bash

       export ARMFVP_BIN_PATH=/path/to/fvp/directory
       west build -b {BOARD qualifier from table above} samples/hello_world -t run

   To run the Fixed Virtual Platform simulation tool you must download "FVP model
   for the Corstone-320 MPS4" from Arm and install it on your host PC. This board
   has been tested with version 11.27.25 (Sep 24 2024).

   QEMU Usage:
    - N/A.

.. note::

   - Board qualifier must include the variant name as mentioned above.
     ``mps4/corstone320`` without the variant name is not a valid qualifier.
   - ``mps4/corstone320/fvp/ns`` variant needs latest upstream TF-M release since Zephyr's current
     TF-M doesn't support Corstone-320 FVP yet.

Hardware
********

No H/W available yet, only ARMFVP simulated board variants are supported for now.

Supported Features
===================

.. zephyr:board-supported-hw::

Serial Port
===========

The MPS4 has six UARTs. The Zephyr console output by default, uses
UART0.

Serial port 0 on the Debug USB interface is the MCC board control console.

Serial port 1 on the Debug USB interface is connected to UART 0.

Serial port 2 on the Debug USB interface is connected to UART 1.

Serial port 3 on the Debug USB interface is connected to UART 2.

.. Programming and Debugging:

Programming and Debugging
*************************

Flashing
========

- N/A since the only support available is FVP.

Building an application with Corstone-320
-----------------------------------------

You can build applications in the usual way. Here is an example for
the :zephyr:code-sample:`hello_world` application with Corstone-320.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: mps4/corstone320/fvp
   :goals: run

Run with FVP and you should see the following message:

.. code-block:: console

   Hello World! mps4

For more details refer to:
 - `Corstone SSE-320 Reference Guide`_
 - `Cortex M85 Generic User Guide`_
 - `Arm Corstone-320 Reference Package Technical Overview`_
 - `Arm MPS4 FPGA Prototyping Board Technical Reference Manual`_

.. _Corstone-320 FVP:
   https://developer.arm.com/tools-and-software/open-source-software/arm-platforms-software/arm-ecosystem-fvps

.. _Corstone SSE-320 Reference Guide:
   https://developer.arm.com/documentation/109760/0000/

.. _Cortex M85 Generic User Guide:
   https://developer.arm.com/documentation/101924/latest

.. _Arm Corstone-320 Reference Package Technical Overview:
   https://developer.arm.com/documentation/109761/0000/

.. _Arm MPS4 FPGA Prototyping Board Technical Reference Manual:
   https://developer.arm.com/documentation/102577/0000/

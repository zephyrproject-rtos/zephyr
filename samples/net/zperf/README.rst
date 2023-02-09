.. _zperf-sample:

zperf: Network Traffic Generator
################################

Description
***********

The zperf sample demonstrates the :ref:`zperf shell utility <zperf>`, which
allows to evaluate network bandwidth.

Features
*********

- Compatible with iPerf_2.0.5. Note that in newer iPerf versions,
  an error message like this is printed and the server reported statistics
  are missing.

.. code-block:: console

   LAST PACKET NOT RECEIVED!!!

- Client or server mode allowed without need to modify the source code.

Supported Boards
****************

zperf is board-agnostic. However, to run the zperf sample application,
the target platform must provide a network interface supported by Zephyr.

This sample application has been tested on the following platforms:

- Freedom Board (FRDM K64F)
- QEMU x86
- Arm FVP BaseR AEMv8-R

Requirements
************

- iPerf 2.0.5 installed on the host machine
- Supported board

Depending on the network technology chosen, extra steps may be required
to setup the network environment.

Usage
*****

See :ref:`zperf library documentation <zperf>` for more information about
the library usage.

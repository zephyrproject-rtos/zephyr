.. _sd_dev_shell_sample:

SD Device Shell Sample
######################

Overview
********

This sample demonstrates the SD Device subsystem API using an interactive
shell interface. It allows testing SDIO device-side communication including:

* Device initialization and enumeration
* TX/RX throughput measurement
* Loopback testing between host and device
* SDIO register read/write operations

The sample targets SoCs that expose an SDIO slave interface. The host
processor communicates with the device over the SDIO bus and must enumerate
the SDIO device before any data transfer commands can be used.

.. note::
   This sample requires a host processor connected to the SDIO bus.

Requirements
************

* A board with an SDIO device (slave) interface
* A host processor connected to the SDIO bus

Building and Running
********************

Build and flash for a board with SDIO device support:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/sd_dev/shell
   :board: <board>
   :goals: build flash
   :compact:

If a board-specific overlay is needed:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/sd_dev/shell
   :board: <board>
   :goals: build flash
   :gen-args: -DEXTRA_DTC_OVERLAY_FILE=boards/<board>.overlay
   :compact:

Shell Commands
**************

Once the sample is running, the following shell commands are available:

.. code-block:: none

   sdio_test init          - Initialize SDIO device
   sdio_test tx <len>      - Transmit <len> bytes to host
   sdio_test rx            - Receive data from host
   sdio_test loopback      - Start loopback test
   sdio_test throughput    - Run TX/RX throughput benchmark

Sample Output
*************

.. code-block:: console

   *** Booting Zephyr OS build v4.x.x ***
   [00:00:00.001,000] <inf> sd_dev: SD Device subsystem initialized
   [00:00:00.002,000] <inf> sd_dev: SDIO device initialized
   [00:00:00.003,000] <inf> sd_dev_shell: SDIO Device ready, waiting for host enumeration
   [00:00:01.234,000] <inf> sd_dev_shell: SDIO device enumerated by host

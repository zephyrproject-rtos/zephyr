.. zephyr:code-sample:: amp_mbox
   :name: mbox API AMP sample

   AMP system example for NXP i.MX RTxxx platforms - IPC using the `mbox` API.

Overview
********

This sample demonstrates the use of the DSP domains on supported NXP i.MX RTxxx
platforms in an asymmetric multiprocessing (AMP) scenario. It's a sample with
separate projects for Cortex-M and DSP domains, that are built together into a
single resulting image using Sysbuild. The Cortex-M domain is responsible for
setting up the DSP domain (clock and power setup, code load and start) and
sending a simple message using the ``mbox`` API (which maps to the MU
peripheral on the devices covered). The DSP domain is programmed to print
a "hello world" message to the chosen console UART and to receive the sent
message, printing it out as it is received.

Building and Running
********************

This sample can be built and started on supported boards as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nxp/adsp/rtxxx/amp_mbox
   :board: <board>
   :goals: build flash
   :build-args: --sysbuild
   :flash-args: -r jlink
   :compact:

Currently, these boards are supported:
- ``mimxrt685_evk/mimxrt685s/cm33``

The use of J-Link firmware on integrated debug probes of those boards or a
standalone J-Link probe is desired as the J-Link probes have the ability
to directly debug the Xtensa-based DSP cores.

Sample output
=============

.. code-block:: console

   *** Booting Zephyr OS build v4.1.0-1858-g6bd0a62b0bde ***
   Hello World! mimxrt685_evk/mimxrt685s/cm33
   [ARM] Starting DSP...
   *** Booting Zephyr OS build v4.1.0-1858-g6bd0a62b0bde ***
   [DSP] Hello World! mimxrt685_evk/mimxrt685s/hifi4
   [DSP] Button pressed!

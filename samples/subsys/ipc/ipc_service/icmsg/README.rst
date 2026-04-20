.. zephyr:code-sample:: ipc-icmsg
   :name: IPC service: icmsg backend
   :relevant-api: ipc

   Send messages between two cores using the IPC service and icmsg backend.

Overview
********

This application demonstrates how to use IPC Service and the icmsg backend with
Zephyr. It is designed to demonstrate how to integrate it with Zephyr both
from a build perspective and code.

Building the application for nrf5340dk/nrf5340/cpuapp
*****************************************************

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/ipc/ipc_service/icmsg
   :board: nrf5340dk/nrf5340/cpuapp
   :goals: debug
   :west-args: --sysbuild

Open a serial terminal (minicom, putty, etc.) and connect the board with the
following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Reset the board and the following message will appear on the corresponding
serial port, one is host another is remote:

.. code-block:: console

   *** Booting Zephyr OS build zephyr-v3.2.0-323-g468d4ae383c1  ***
   [00:00:00.415,985] <inf> host: IPC-service HOST demo started
   [00:00:00.417,816] <inf> host: Ep bounded
   [00:00:00.417,877] <inf> host: Perform sends for 1000 [ms]
   [00:00:01.417,114] <inf> host: Sent 488385 [Bytes] over 1000 [ms]
   [00:00:01.417,175] <inf> host: Wait 500ms. Let net core finish its sends
   [00:00:01.917,266] <inf> host: Stop network core
   [00:00:01.917,297] <inf> host: Reset IPC service
   [00:00:01.917,327] <inf> host: Run network core
   [00:00:01.924,865] <inf> host: Ep bounded
   [00:00:01.924,896] <inf> host: Perform sends for 1000 [ms]
   [00:00:02.924,194] <inf> host: Sent 489340 [Bytes] over 1000 [ms]
   [00:00:02.924,224] <inf> host: IPC-service HOST demo ended


.. code-block:: console

   *** Booting Zephyr OS build zephyr-v3.2.0-323-g468d4ae383c1  ***
   [00:00:00.006,256] <inf> remote: IPC-service REMOTE demo started
   [00:00:00.006,378] <inf> remote: Ep bounded
   [00:00:00.006,439] <inf> remote: Perform sends for 1000 [ms]
   [00:00:00.833,160] <inf> sync_rtc: Updated timestamp to synchronized RTC by 1)
   [00:00:01.417,572] <inf> remote: Sent 235527 [Bytes] over 1000 [ms]
   [00:00:01.417,602] <inf> remote: IPC-service REMOTE demo ended
   *** Booting Zephyr OS build zephyr-v3.2.0-323-g468d4ae383c1  ***
   [00:00:00.006,256] <inf> remote: IPC-service REMOTE demo started
   [00:00:00.006,347] <inf> remote: Ep bounded
   [00:00:00.006,378] <inf> remote: Perform sends for 1000 [ms]
   [00:00:01.006,164] <inf> remote: Sent 236797 [Bytes] over 1000 [ms]
   [00:00:01.006,195] <inf> remote: IPC-service REMOTE demo ended

Building for AMD Versal RPU AMP (Cortex-R5F, two Zephyr images)
****************************************************************

IPI and message-buffer addresses match ``samples/boards/amd/amp_ipi_exchange``
(IPI1 / IPI2 at ``0xff340000`` / ``0xff350000``).  Shared ICMSG rings use the
last 8 KiB of the 256 KiB OCM at ``0xfffc0000`` (``0xffffa000`` and
``0xffffb000``).

.. code-block:: console

   west build -b versal_rpu/versal_rpu/amp/core0 -d build_ipc0 samples/subsys/ipc/ipc_service/icmsg
   west build -b versal_rpu/versal_rpu/amp/core1 -d build_ipc1 samples/subsys/ipc/ipc_service/icmsg/remote

Flash both images with XSDB and ``--second-elf`` (use the PDI supplied for your
Versal RPU platform / lab flow).

Building for AMD VersalNet RPU AMP (two Zephyr images)
*******************************************************

The devicetree uses ``zephyr,ipc-icmsg`` (see ``dts/bindings/ipc/zephyr,ipc-icmsg.yaml``)
with two small non-cacheable OCM regions and Xilinx IPI mailboxes.  Because
sysbuild is not yet reliable for all HWMv2 AMP board strings, build the host and
remote projects separately (same pattern as ``samples/boards/amd/amp_ipi_exchange``):

.. code-block:: console

   west build -b versalnet_rpu/amd_versalnet_rpu/amp/core0 -d build_ipc0 samples/subsys/ipc/ipc_service/icmsg
   west build -b versalnet_rpu/amd_versalnet_rpu/amp/core1 -d build_ipc1 samples/subsys/ipc/ipc_service/icmsg/remote

Shared ICMSG rings occupy ``0xbbffe000`` and ``0xbbfff000`` (last 8 KiB of the
1 MiB ``memory@bbf00000`` span in ``dts/vendor/amd/versalnet.dtsi``).

Flash both ``zephyr.elf`` files with your board flow (for example XSDB with
``--second-elf``).

Building for AMD Versal Gen 2 RPU AMP (two Zephyr images)
**********************************************************

The devicetree uses the same buffer layout as ``dts/vendor/amd/versalnet.dtsi``
OCM and Xilinx IPI mailboxes.

.. code-block:: console

   west build -b versal2_rpu/amd_versal2_rpu/amp/core0 -d build_ipc0 samples/subsys/ipc/ipc_service/icmsg
   west build -b versal2_rpu/amd_versal2_rpu/amp/core1 -d build_ipc1 samples/subsys/ipc/ipc_service/icmsg/remote

Flash both images with XSDB (Versal Gen 2 RPU PDI for your lab) using
``--second-elf`` for the core1 ELF.

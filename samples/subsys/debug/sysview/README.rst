.. _segger_sysview:

SEGGER SystemView Sample
########################

Overview
********
This sample application uses the kernel system logger and publishes events
through the SEGGER RTT protocol, making it available to the SEGGER
SystemView application.

Requirements
************

* Board supported by J-Link (`list of supported boards <https://www.segger.com/jlink_supported_devices.html#DeviceList>`_)
* `SEGGER J-Link Software and Documentation pack <https://www.segger.com/downloads/jlink>`_
* The board might require a `special bootloader <https://www.segger.com/opensda.html>`_, also available from SEGGER
* `SEGGER SystemView <https://www.segger.com/systemview.html?p=1731>`_


Building and Running
********************

* Follow the instructions to install J-Link and SystemView software on your
  computer
* Open J-Link Commander.  On Linux, its executable is named ``JLinkExe``:

.. code-block:: console

  SEGGER J-Link Commander V6.10m (Compiled Nov 10 2016 18:38:45)
  DLL version V6.10m, compiled Nov 10 2016 18:38:36

  Connecting to J-Link via USB...O.K.
  Firmware: J-Link OpenSDA 2 compiled Feb 28 2017 19:27:22
  Hardware version: V1.00
  S/N: 621000000
  VTref = 3.300V


  Type "connect" to establish a target connection, '?' for help
  J-Link>

* Issue the "connect" command.  If it's the only connected board, ``Enter``
  can be pressed at the ``Device>`` prompt.
* Select the target interface.  Some devices only support the ``SWD`` type,
  so select it by typing ``S`` followed by ``Enter``.
* At the ``Speed>`` prompt, select the interface polling frequency.  The
  default of 4000kHz is sufficient, but a higher frequency can be specified.
* Once the connection has been successful, an output similar to this one
  should be produced:

.. code-block:: console

  Device "MK64FN1M0XXX12" selected.


  Found SWD-DP with ID 0x2BA01477
  Found SWD-DP with ID 0x2BA01477
  AP-IDR: 0x24770011, Type: AHB-AP
  Found Cortex-M4 r0p1, Little endian.
  FPUnit: 6 code (BP) slots and 2 literal slots
  CoreSight components:
  ROMTbl 0 @ E00FF000
  ROMTbl 0 [0]: FFF0F000, CID: B105E00D, PID: 000BB00C SCS
  ROMTbl 0 [1]: FFF02000, CID: B105E00D, PID: 003BB002 DWT
  ROMTbl 0 [2]: FFF03000, CID: B105E00D, PID: 002BB003 FPB
  ROMTbl 0 [3]: FFF01000, CID: B105E00D, PID: 003BB001 ITM
  ROMTbl 0 [4]: FFF41000, CID: B105900D, PID: 000BB9A1 TPIU
  ROMTbl 0 [5]: FFF42000, CID: B105900D, PID: 000BB925 ETM
  ROMTbl 0 [6]: FFF43000, CID: B105900D, PID: 003BB907 ETB
  ROMTbl 0 [7]: FFF44000, CID: B105900D, PID: 001BB908 CSTF
  Cortex-M4 identified.
  J-Link>

* Now open SystemView.  Select the option *Start Recording* from the
  *Target* menu (or press ``F5``), choose USB, the target device (in this
  case, ``MK64FN1M0XXX12``), and confirm that the target interface and speed
  matches the ones selected in J-Link Commander.  The *RTT Control Block
  Detection* can be left on *Auto Detection*.
* Once OK is clicked, information will be pulled from the device as usual:
  threads, interrupts, and other information will be populated
  automatically.

References
**********

* `Segger SystemView: Realtime Analysis and Visualization for FreeRTOS <https://mcuoneclipse.com/2015/11/16/segger-systemview-realtime-analysis-and-visualization-for-freertos/>`_
* `RTT Protocol <https://www.segger.com/jlink-rtt.html>`_


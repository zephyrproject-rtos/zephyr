Bluetooth Tester application
############################

The Tester application uses binary protocol to control Zephyr stack and is aimed at
automated testing. It requires two serial ports to operate.
The first serial is used by Bluetooth Testing Protocol (BTP) to drive Bluetooth
stack. BTP commands and events are received and buffered for further processing
over the same serial.

BTP specification can be found in auto-pts project repository:
https://github.com/intel/auto-pts
The auto-pts is an automation framework for PTS Bluetooth testing tool provided
by Bluetooth SIG.

See https://docs.zephyrproject.org/latest/guides/bluetooth/index.html for full
documentation about how to use this test.


Supported Profiles and Services
*******************************

Host/Core
=========

* GAP
* GATT
* IAS
* L2CAP
* OTS
* SM

LE Audio
========

* AICS
* ASCS
* BAP
* CAP
* CAS
* CCP
* CSIP
* CSIS
* HAP
* HAS
* MCP
* MCS
* MCIP
* MICS
* PACS
* PBP
* TBS
* TMAP
* VCP
* VCS
* VOCS

Mesh
====

* Mesh Node
* Mesh Model

Building and running on QEMU
****************************

QEMU should have connection with the external host Bluetooth hardware.
The btproxy tool from BlueZ can be used to give access to a Bluetooth controller
attached to the Linux host OS:

$ sudo tools/btproxy -u
Listening on /tmp/bt-server-bredr

/tmp/bt-server-bredr option is already set in Makefile through QEMU_EXTRA_FLAGS.

To build tester application for QEMU use BOARD=qemu_cortex_m3 and
CONF_FILE=qemu.conf. After this qemu can be started through the "run"
build target.

Note: Target board have to support enough UARTs for BTP and controller.
      We recommend using qemu_cortex_m3.

'bt-stack-tester' UNIX socket (previously set in Makefile) can be used for now
to control tester application.

Next, build and flash tester application by employing the "flash" build
target.

Use serial client, e.g. PUTTY to communicate over the serial port
(typically /dev/ttyUSBx) with the tester using BTP.

Building and running on :zephyr:board:`native_sim <native_sim>` on Linux
************************************************************************

It is possible to build and run the tester application using the
:zephyr:board:`native_sim <native_sim>` board, and any compatible HCI controller.
This has the advantage of allowing the use of Linux debugging tools like valgrind and gdb,
as well as tools like btmon.
It is also faster to apply changes to the code,
as building for :zephyr:board:`native_sim <native_sim>` is usually faster,
and there is no flashing step involved.

Building for :zephyr:board:`native_sim <native_sim>` is just

.. code-block::

    west build -b native_sim

Which will generate a zephyr.exe file that can be executed as

.. code-block::

    zephyr.exe --bt-dev=hciX

Where ``hciX`` is the HCI controller (like ``hci0``).
However for the purpose of running the tester application with auto-pts,
running the application is left to the auto-pts client.


Building the Zephyr controller for a :zephyr:board:`native_sim <native_sim>` host on Linux
==========================================================================================

To build and flash the Zephyr controller as an HCI controller usable by Linux,
either the :zephyr:code-sample:`bluetooth_hci_uart` or :zephyr:code-sample:`bluetooth_hci_usb`
samples can be used.
See also :ref:`bluetooth-tools`.
When building these samples, the tester application controller overlay should be supplied.
For example

.. code-block::

    west build -b nrf5340_audio_dk/nrf5340/cpunet -d ${ZEPHYR_BASE}/build/nrf5340_audio_dk_nrf5340_cpunet ${ZEPHYR_BASE}/samples/bluetooth/hci_ipc/ -- -DEXTRA_CONF_FILE=${ZEPHYR_BASE}/tests/bluetooth/tester/overlay-bt_ll_sw_split.conf
    west flash -d ${ZEPHYR_BASE}/build/nrf5340_audio_dk_nrf5340_cpunet
    west build -b nrf5340_audio_dk/nrf5340/cpuapp -d ${ZEPHYR_BASE}/build/nrf5340_audio_dk_nrf5340_cpuapp ${ZEPHYR_BASE}/samples/bluetooth/hci_uart/
    west flash -d ${ZEPHYR_BASE}/build/nrf5340_audio_dk_nrf5340_cpuapp

Will build and prepare a nRF5340 Audio DK to be an HCI controller over UART.

For single core boards like the nRF52840 DK it is a bit simpler and can be done like

.. code-block::

    west build -b nrf52840dk/nrf52840 ${ZEPHYR_BASE}/samples/bluetooth/hci_uart/ -- -DEXTRA_CONF_FILE=${ZEPHYR_BASE}/tests/bluetooth/tester/overlay-bt_ll_sw_split.conf
    west flash

The :zephyr:code-sample:`bluetooth_hci_usb` sample can also be used,
but support for Bluetooth Isochronous channels is not yet fully supported.

Building for LE Audio
*********************

The tester application can be built with support for BT LE Audio by applying the
the ``overlay-le-audio.conf`` and ``hci_ipc.conf`` with ``--sysbuild`` for the supported boards,
e.g.:

.. code-block::

    west build -b nrf5340dk/nrf5340/cpuapp --sysbuild \
        -- -DEXTRA_CONF_FILE=overlay-le-audio.conf;hci_ipc.conf

Building with support for btsnoop and rtt logs
**********************************************

Add following options in desired configuration file:

CONFIG_LOG=n
CONFIG_LOG_BACKEND_RTT=y
CONFIG_LOG_BACKEND_RTT_BUFFER=1
CONFIG_LOG_BACKEND_RTT_MODE_DROP=n

CONFIG_USE_SEGGER_RTT=y
CONFIG_SEGGER_RTT_SECTION_CUSTOM=y

CONFIG_BT_DEBUG_MONITOR_RTT=y
CONFIG_BT_DEBUG_MONITOR_RTT_BUFFER=2

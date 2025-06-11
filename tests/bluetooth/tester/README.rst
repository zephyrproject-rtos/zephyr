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

Building for LE Audio
*********************

The tester application can be built with support for BT LE Audio by applying the
the ``overlay-le-audio.conf`` and ``hci_ipc.conf`` with ``--sysbuild`` for the supported boards,
e.g.:

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

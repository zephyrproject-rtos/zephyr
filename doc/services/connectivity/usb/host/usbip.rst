.. _usbip:

USB/IP protocol support
#######################

Overview
********

New USB support includes initial support for the USB/IP protocol. It is still
under development and is currently limited to supporting only one device
connected to the host controller being exported.

USB/IP uses TCP/IP. Both of the underlying connectivity stacks, USB and
networking, require significant memory resources, which must be considered when
choosing a platform.

In the USB/IP protocol, a server exports the USB devices and a client imports
them. USB/IP support in the Zephyr RTOS implements server functionality and
exports a device connected to a host controller on a device running the Zephyr
RTOS. A client, typically running the Linux kernel, imports this device. The
USB/IP protocol is described in `USB/IP protocol documentation`_.

To use USB/IP support, make sure the required modules are loaded on the client side.

.. code-block:: console

   modprobe vhci_hcd
   modprobe usbip-core
   modprobe usbip-host

On the client side, you will also need the **usbip** user tool. It can be installed
using your Linux distribution's package management system or built from Linux
kernel sources.

There are a few basic commands for everyday use. To list exported USB devices,
run the following command:

.. code-block:: console

   $ usbip list -r 192.0.2.1
   Exportable USB devices
   ======================
    - 192.0.2.1
           1-1: NordicSemiconductor : unknown product (2fe3:0001)
              : /sys/bus/usb/devices/usb1/1-1
              : Miscellaneous Device / ? / Interface Association (ef/02/01)
              :  0 - Communications / Abstract (modem) / None (02/02/00)
              :  1 - CDC Data / Unused / unknown protocol (0a/00/00)

To attach an exported device with busid 1-1:

.. code-block:: console

   $ sudo usbip attach -r 192.0.2.1 -b 1-1

To detach an exported device on port 0:

.. code-block:: console

   $ sudo usbip detach -p 0

USB/IP with native_sim
**********************

The preferred method to develop with USB/IP support enabled is to use
:zephyr:board:`native_sim <native_sim>`. Use on real hardware is not really tested yet.
USB/IP requires a network connection, see :ref:`networking_with_native_sim`
for how to set up the interface on the client side.

Building and running a sample with USB/IP requires extensive configuration,
you can use usbip-native-sim snippet to configure host and USB/IP support.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/cdc_acm
   :board: native_sim/native/64
   :gen-args: -DSNIPPET=usbip-native-sim -DEXTRA_DTC_OVERLAY_FILE=app.overlay
   :goals: build

.. _USB/IP protocol documentation: https://www.kernel.org/doc/html/latest/usb/usbip_protocol.html

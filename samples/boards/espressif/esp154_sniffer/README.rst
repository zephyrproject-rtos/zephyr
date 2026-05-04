.. zephyr:code-sample:: esp154-sniffer
   :name: IEEE 802.15.4 sniffer
   :relevant-api: ieee802154 uart_interface

   Capture IEEE 802.15.4 frames with an Espressif radio and feed them to Wireshark.

Overview
********

This sample turns an Espressif SoC with an IEEE 802.15.4 radio into a packet sniffer. The
radio is driven directly through :c:struct:`ieee802154_radio_api` in raw mode, with no
network stack and no L2 above it, so every frame the hardware accepts is reported.

Each captured frame is printed on the console UART as a single line:

.. code-block:: none

   received: <psdu-hex> power: <rssi-dbm> lqi: <lqi> time: <us>

``psdu-hex``
   The PSDU as lowercase hexadecimal, without separators and **without an FCS**. The radio
   drops frames whose checksum is wrong and overwrites the two checksum bytes with RSSI and
   LQI, so there is no checksum left to report. ``CONFIG_IEEE802154_L2_PKT_INCL_FCS=n``
   keeps those two bytes off the wire.

``power``
   Signal strength in dBm, signed. An unavailable value is reported as ``-32768``
   (:c:macro:`IEEE802154_MAC_RSSI_DBM_UNDEFINED`) so that every line has the same shape.

``lqi``
   Link quality indicator, 0 to 255.

``time``
   Microseconds since the device booted, as a 32-bit counter that wraps roughly every
   71 minutes. The host tool anchors it to UNIX time using the first frame it receives.

The shell shares the same UART. Lines that do not match the pattern above, such as log
messages and the shell prompt, are ignored by the host tool, so no second serial port is
needed.

Requirements
************

A supported board with a built-in IEEE 802.15.4 radio:

* ``esp32c6_devkitc/esp32c6/hpcore``
* ``esp32h2_devkitm``
* ``esp32c5_devkitc/esp32c5/hpcore``

The host tool needs Wireshark 4.0 or later, Python 3.10 or later, and pySerial.

Building and running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/boards/espressif/esp154_sniffer
   :board: esp32c6_devkitc/esp32c6/hpcore
   :goals: build flash
   :compact:

The sample raises the console to 921600 baud, because a maximum-length frame takes about
26 ms to print at 115200 baud, which is slower than a busy channel delivers frames. Serial
terminals must be opened at that rate, and so must the host tool. To go back to the board
default, drop :file:`app.overlay`.

Shell commands
==============

.. code-block:: console

   uart:~$ sniffer channel 15
   channel 15
   uart:~$ sniffer start
   started
   received: 41881234567890abcdef power: -47 lqi: 103 time: 123456789
   uart:~$ sniffer stats
   enq=1 drop=0 tx_rec=1 tx_bytes=68 q_hwm=1 q_used=0/16
   uart:~$ sniffer stop
   stopped

The channel can only be changed while the radio is stopped. ``sniffer stats`` reports how
many frames were queued, how many were dropped because the queue or the UART could not keep
up, and the queue's high-water mark.

Capturing with Wireshark
************************

:file:`extcap/esp154_sniffer.py` is a Wireshark extcap plugin. It drives the shell, parses
the capture lines and writes a pcap stream using the IEEE 802.15.4 TAP link type, which
carries the channel, RSSI and LQI into Wireshark as per-packet metadata.

Install it into Wireshark's extcap directory, shown under
:menuselection:`Help --> About Wireshark --> Folders --> Extcap path`:

.. code-block:: console

   $ mkdir -p ~/.config/wireshark/extcap
   $ cp samples/boards/espressif/esp154_sniffer/extcap/esp154_sniffer.py ~/.config/wireshark/extcap/
   $ chmod +x ~/.config/wireshark/extcap/esp154_sniffer.py

Restart Wireshark, or use :menuselection:`Capture --> Refresh Interfaces`. The board appears
in the interface list; the gear icon selects the channel and the baud rate.

The plugin also runs standalone, which is the quickest way to tell a firmware problem from a
plugin problem:

.. code-block:: console

   $ ./extcap/esp154_sniffer.py --extcap-interfaces
   $ ./extcap/esp154_sniffer.py --capture --extcap-interface /dev/ttyUSB0 \
       --channel 15 --fifo capture.pcap

Add ``--metadata none`` to write bare frames without the TAP header, for checking that the
frames themselves dissect.

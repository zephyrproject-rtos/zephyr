.. zephyr:code-sample:: ieee802154-sniffer
   :name: IEEE 802.15.4 sniffer
   :relevant-api: ieee802154 uart_interface

   Capture IEEE 802.15.4 frames and feed them to Wireshark.

Overview
********

This sample turns a board with a built-in IEEE 802.15.4 radio into a packet sniffer. The
radio is driven directly through :c:struct:`ieee802154_radio_api` in raw mode, with no
network stack and no L2 above it, so every frame the hardware accepts is reported.

Each captured frame is printed on the console UART as a single line:

.. code-block:: none

   psdu: <hex> power: <rssi-dbm> lqi: <lqi> time: <us>

``psdu``
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

Any board with a built-in IEEE 802.15.4 radio, that is, one whose devicetree gives a
``zephyr,ieee802154`` chosen node. The sample has been built and run on:

* ``esp32c6_devkitc/esp32c6/hpcore``
* ``esp32h2_devkitm``
* ``esp32c5_devkitc/esp32c5/hpcore``

The host tool needs Wireshark 4.0 or later, Python 3.10 or later, and pySerial.

Building and running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/net/ieee802154/sniffer
   :board: esp32c6_devkitc/esp32c6/hpcore
   :goals: build flash
   :compact:

The console keeps the board's own rate, normally 115200 baud, so a serial terminal and the
shell work as they do in any other sample. That rate is not enough for a busy channel: a
maximum-length frame takes about 26 ms to print at 115200 baud, which is slower than frames
arrive on a busy channel. The host tool therefore raises the console for the duration of a
capture and puts it back afterwards, which is what ``sniffer speed`` is for.

Shell commands
==============

.. code-block:: console

   uart:~$ sniffer channel 15
   channel 15
   uart:~$ sniffer start
   started
   psdu: 41881234567890abcdef power: -47 lqi: 103 time: 123456789
   uart:~$ sniffer stats
   enq=1 drop=0 tx_rec=1 tx_bytes=68 q_hwm=1 q_used=0/16
   uart:~$ sniffer stop
   stopped
   uart:~$ sniffer speed
   speed 115200

The channel can only be changed while the radio is stopped. ``sniffer stats`` reports how
many frames were queued, how many were dropped because the queue or the UART could not keep
up, and the queue's high-water mark. ``sniffer speed <baud>`` reconfigures the console:
everything after the reply comes out at the new rate, so a terminal has to be reopened there.

Capturing with Wireshark
************************

:file:`samples/net/ieee802154/sniffer/scripts/espressif_ieee802154_sniffer.py` is a Wireshark extcap
plugin. It drives the shell, parses the capture lines and writes a pcap stream using the
IEEE 802.15.4 TAP link type, which carries the channel, RSSI and LQI into Wireshark as
per-packet metadata.

The plugin recomputes the two FCS bytes from the PSDU and appends them, so that the capture
holds complete frames, and declares them through the TAP FCS Type field. The radio verified
the checksum of every frame it reports before overwriting it with RSSI and LQI, so the
regenerated value always matches the frame; it is not the checksum that arrived over the air.

Install it into Wireshark's extcap directory, shown under
:menuselection:`Help --> About Wireshark --> Folders --> Extcap path`:

.. code-block:: console

   $ mkdir -p <your_home_extcap_path>
   $ cp samples/net/ieee802154/sniffer/scripts/espressif_ieee802154_sniffer.py <your_home_extcap_path>
   $ chmod +x <your_home_extcap_path>/espressif_ieee802154_sniffer.py

Restart Wireshark, or use :menuselection:`Capture --> Refresh Interfaces`. The board appears
in the interface list; the gear icon selects the channel, the two baud rates and the diagnostic
log.

.. note::

   The user running the Wireshark must be in the ``pcap`` group. Also it needs read and write
   access to the serial port, for example via the ``dialout`` group (some distributions use
   ``uucp`` group).

:guilabel:`Baud rate` is the rate the console runs at when the capture starts, 115200 by
default. :guilabel:`Capture baud rate` is what the plugin raises it to for the capture, 921600
by default; the console is put back when the capture ends. Set it to 0 to leave the rate alone,
for a board whose console is already fast or one built without
:kconfig:option:`CONFIG_UART_USE_RUNTIME_CONFIGURE`.

The plugin also runs standalone, which is the quickest way to tell a firmware problem from a
plugin problem:

.. code-block:: console

   $ ./samples/net/ieee802154/sniffer/scripts/espressif_ieee802154_sniffer.py --extcap-interfaces
   $ ./samples/net/ieee802154/sniffer/scripts/espressif_ieee802154_sniffer.py --capture \
       --extcap-interface /dev/ttyUSB0 \
       --channel 15 --log-file sniffer.log --fifo capture.pcap

Add ``--metadata none`` to write bare frames without the TAP header, using the
``IEEE802_15_4_WITHFCS`` link type, for checking that the frames themselves dissect.

Troubleshooting
===============

An empty packet list means the plugin is not getting whole capture lines from the serial port:

* Close any serial monitor on the same port before capturing. A monitor that is already
  attached keeps reading, the two readers then take turns on the incoming bytes, and every line
  the plugin sees is cut short while the monitor still looks healthy.
* Check that the channel in the interface settings is the one the traffic is on. The plugin
  retunes the radio when the capture starts, so the channel the board booted with does not
  matter.
* Check the baud rate. A capture that was killed outright leaves the console at the capture rate,
  and so does firmware built with a raised console. The plugin tries the other common rate on its
  own when the board does not answer, puts the rate back the next time a capture ends normally,
  and names the rate it settled on in the log, but setting the right one avoids the delay.
* Fill in :guilabel:`Diagnostic log` in the interface settings, or pass ``--log-file``, and read
  the result. It holds the replies the board gave to ``sniffer speed``, ``sniffer channel``,
  ``sniffer start`` and ``sniffer stats``, and the first lines that did not parse. ``enq=0`` in
  the stats line means the radio has not heard anything at all.

The shell commands are best effort, so a board that answers none of them is still captured from.
The sample raises the shell thread above the thread that prints the capture lines
(:kconfig:option:`CONFIG_SHELL_THREAD_PRIORITY`), because that one busy waits on the UART and
would otherwise starve the shell on a busy channel. Replies can still come out mixed into a
capture line, which is why every command is sent more than once. The log names the commands that
went unanswered, and an unconfirmed channel means the radio kept the one it was last set to: set
it from a terminal before starting the capture if the frames are not the ones expected.

Opening the port does not reset the board: the plugin leaves DTR and RTS deasserted, which keeps
the auto-reset circuit of the devkits alone.

.. _usbd_midi2:

MIDI 2.0 Class device API
#########################

USB MIDI 2.0 device specific API defined in :zephyr_file:`include/zephyr/usb/class/usbd_midi2.h`.

Overview
********

The ``usbd_midi2`` class driver implements the USB MIDI Streaming interface for
the :ref:`USB device_next stack <usb_device_stack_next>`. From a single driver
instance it exposes the legacy USB-MIDI 1.0 alternate setting alongside the
USB-MIDI 2.0 streaming alternate, so the same firmware can serve both legacy
hosts and MIDI 2.0 capable hosts.

The application surface is unified around the Universal MIDI Packet (UMP)
representation defined in :zephyr_file:`include/zephyr/audio/midi.h`. When the
host has selected the USB-MIDI 1.0 alternate, the driver converts incoming
USB-MIDI 1.0 event packets to UMPs and outgoing UMPs to USB-MIDI 1.0 event
packets transparently. When the host has selected the USB-MIDI 2.0 alternate,
UMPs flow natively. UMPs that have no MIDI 1.0 representation (for example
:c:macro:`UMP_MT_MIDI2_CHANNEL_VOICE`) are rejected with ``-ENOTSUP`` while the
MIDI 1.0 alternate is active.

Key features:

* Up to 16 group terminals defined in Devicetree, each exposed both as a MIDI
  2.0 Group Terminal Block on the USB-MIDI 2.0 alternate and as the matching
  pair of USB-MIDI 1.0 jacks on the legacy alternate.
* Optional MIDI-CI stream responder helper integration.
* UMP-only application interface, regardless of which USB-MIDI alternate the
  host selects.
* Notification of the interface status (disabled, or the active USB-MIDI
  alternate) through :c:member:`usbd_midi_ops.status_changed_cb`.

Configuration
*************

Enable the class with :kconfig:option:`CONFIG_USBD_MIDI2_CLASS`. The driver
always builds both the USB-MIDI 1.0 and USB-MIDI 2.0 alternate settings.

The Devicetree node describes the group terminal blocks the interface reports
to the host. Each block currently corresponds to exactly one MIDI 2.0 group and
one USB-MIDI 1.0 cable. Bidirectional blocks contribute one embedded IN jack
plus one external OUT jack (host-to-device path) *and* one embedded OUT jack
plus one external IN jack (device-to-host path); ``input-only`` and
``output-only`` blocks contribute only the corresponding pair. USB-MIDI 1.0
cable numbers are assigned in Devicetree child order, separately for each
direction.

The sample at :zephyr_file:`samples/subsys/usb/midi` shows a minimal
configuration with one bidirectional block:

.. code-block:: devicetree

   usb_midi: usb_midi {
	   compatible = "zephyr,midi2-device";
	   status = "okay";

	   midi_in_out@0 {
		   reg = <0 1>;
		   protocol = "midi1-up-to-128b";
	   };
   };

Runtime API
***********

Register the application's callbacks with :c:func:`usbd_midi_set_ops`. The
callback structure supports:

* :c:member:`usbd_midi_ops.rx_packet_cb` for Universal MIDI Packet traffic.
  Received USB-MIDI 1.0 events are translated to UMPs before this callback is
  invoked, so the application never sees raw USB-MIDI 1.0 events.
* :c:member:`usbd_midi_ops.status_changed_cb` to track interface availability
  and which USB-MIDI alternate the host is currently using. The status is one
  of :c:enumerator:`USBD_MIDI_STATUS_DISABLED`,
  :c:enumerator:`USBD_MIDI_STATUS_MIDI1` or
  :c:enumerator:`USBD_MIDI_STATUS_MIDI2`.

Transmit:

* :c:func:`usbd_midi_send` queues a UMP. The driver routes it to the active
  alternate, converting to USB-MIDI 1.0 event packets when needed.

Example
*******

The `USB MIDI sample <samples/subsys/usb/midi>`_ uses
:c:member:`usbd_midi_ops.rx_packet_cb` to echo channel voice messages back to
the host and :c:member:`usbd_midi_ops.status_changed_cb` to log the interface
status. Button presses are forwarded as
:c:macro:`UMP_MT_MIDI1_CHANNEL_VOICE` UMPs, which are valid on both alternates.

API Reference
*************

.. doxygengroup:: usbd_midi2
.. doxygengroup:: midi_ump

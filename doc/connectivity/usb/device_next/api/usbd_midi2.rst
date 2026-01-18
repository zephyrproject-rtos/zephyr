.. _usbd_midi2:

MIDI 2.0 Class device API
#########################

USB MIDI 2.0 device specific API defined in :zephyr_file:`include/zephyr/usb/class/usbd_midi2.h`.

Overview
********

The ``usbd_midi2`` class driver implements the USB MIDI Streaming interface for
the :ref:`USB device_next stack <usb_device_stack_next>`. It exposes both the legacy
USB-MIDI 1.0 alternate setting and the MIDI 2.0 streaming alternate from a
single driver instance and automatically maps incoming and outgoing traffic to
either raw USB-MIDI events or Universal MIDI Packets (UMPs).

Key features:

* Support for up to 16 bidirectional group terminals defined in Devicetree.
* Optional MIDI-CI stream responder helper integration.
* Runtime callbacks for UMP traffic and raw USB-MIDI 1.0 packets.
* Automatic conversion between MIDI 1.0 channel voice events and UMP channel
  voice packets when the host selects a different alternate setting.
* API to limit enumeration to MIDI 1.0 only, MIDI 2.0 only, or both alternates.

Configuration
*************

Enable the class with :kconfig:option:`CONFIG_USBD_MIDI2_CLASS`. Compile-time
options :kconfig:option:`CONFIG_USBD_MIDI2_ALTSETTING_MIDI1` and
:kconfig:option:`CONFIG_USBD_MIDI2_ALTSETTING_MIDI2` control whether the legacy
USB-MIDI 1.0 alternate setting, the MIDI 2.0 alternate, or both are built into
the driver (both default to ``y``). Disabling an alternate removes its
descriptors, callbacks, and helper APIs from the build.

The Devicetree node describes the group terminal blocks the interface reports
to the host. The sample at :zephyr_file:`samples/subsys/usb/midi` shows a
minimal configuration with one bidirectional block:

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
* :c:member:`usbd_midi_ops.rx_midi1_cb` for raw USB-MIDI 1.0 events when the
  host stays on the legacy alternate (only available if
  :kconfig:option:`CONFIG_USBD_MIDI2_ALTSETTING_MIDI1` is enabled). If not
  provided, the driver will send all incoming MIDI 1.0 messages to the
  :c:member:`usbd_midi_ops.rx_packet_cb` callback.
* :c:member:`usbd_midi_ops.ready_cb` to track interface availability.

Transmit helpers:

* :c:func:`usbd_midi_send` queues complete UMP words.
* :c:func:`usbd_midi_send_midi1` (available when
  :kconfig:option:`CONFIG_USBD_MIDI2_ALTSETTING_MIDI1` is enabled) accepts
  traditional MIDI 1.0 status/data bytes and routes them to the active
  alternate, converting to UMP as needed.

To restrict which alternates enumerate, call :c:func:`usbd_midi_set_mode`
before enabling the USB stack.

Example
*******

The `USB MIDI sample <samples/subsys/usb/midi>`_ configures
:c:member:`usbd_midi_ops.rx_midi1_cb` to echo raw USB-MIDI events while using
:c:member:`usbd_midi_ops.rx_packet_cb` for UMP traffic. Button presses are
forwarded through :c:func:`usbd_midi_send_midi1`, demonstrating how a single
code path works for both host alternates.

API Reference
*************

.. doxygengroup:: usbd_midi2
.. doxygengroup:: midi_ump

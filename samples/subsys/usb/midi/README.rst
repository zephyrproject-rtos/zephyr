.. zephyr:code-sample:: usb-midi2-device
   :name: USB MIDI2 device
   :relevant-api: usbd_api usbd_midi2 input_events

   Implements a USB MIDI loopback and keyboard device with MIDI 1.0 fallback.

Overview
********

This sample demonstrates how to implement a USB MIDI device using the
``USB device_next`` MIDI 2.0 class driver. It can run on any board with a USB
device controller and exposes both the legacy USB-MIDI 1.0 alternate
setting and the MIDI 2.0 streaming alternate. Regardless of which alternate the
host selects, all channel voice messages are looped back to the host and button
presses generate MIDI note on/off events.

The application exposes a single USB-MIDI interface with a single bidirectional
group terminal. This allows exchanging data with the host on a "virtual wire"
that carries MIDI 1.0 messages or Universal MIDI Packets, like a
standard USB-MIDI in/out adapter would provide. The loopback acts as if a real
MIDI cable was connected between the output and the input, and the input keys
act as a MIDI keyboard.

Building and Running
********************

The code can be found in :zephyr_file:`samples/subsys/usb/midi`.

To build and flash the application:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/midi
   :board: nucleo_f429zi
   :goals: build flash
   :compact:

Using the MIDI interface
************************

Once this sample is flashed, connect the device USB port to a host computer
with MIDI support. For example, on Linux, you can use alsa to access the device:

.. code-block:: console

  $ amidi -l
  Dir Device    Name
  IO  hw:2,1,0  Group 1 (USBD MIDI Sample)

On Mac OS you can use the system tool "Audio MIDI Setup" to view the device,
see https://support.apple.com/guide/audio-midi-setup/set-up-midi-devices-ams875bae1e0/mac

The "USBD MIDI Sample" interface should also appear in any program with MIDI
support; like your favorite Digital Audio Workstation or synthetizer. If you
don't have any such program at hand, there are some webmidi programs online,
for example: https://muted.io/piano/.

MIDI 1.0 compatibility
**********************

During enumeration the interface advertises both the USB-MIDI 1.0 (alternate
setting 0) and USB-MIDI 2.0 (alternate setting 1) streaming descriptors. Legacy
hosts remain on the USB-MIDI 1.0 alternate; MIDI 2.0 aware hosts switch to the
USB-MIDI 2.0 alternate. The application always exchanges Universal MIDI Packets
(UMPs) with the driver: incoming USB-MIDI 1.0 events are translated to UMPs
before being delivered via ``rx_packet_cb``, and outgoing UMPs sent through
``usbd_midi_send`` are translated to USB-MIDI 1.0 events when the legacy
alternate is active.

The sample observes the interface status through ``status_changed_cb`` (which
the sample logs) so that an application can tell when the interface is available
and which alternate the host selected, in order to gate features that have no
MIDI 1.0 representation, such as MIDI 2.0 channel voice messages with per-note
attributes. Button presses are emitted as ``UMP_MT_MIDI1_CHANNEL_VOICE``
packets and therefore round-trip cleanly to either kind of host.

Testing loopback
****************

Open a first shell, and start dumping MIDI events:

.. code-block:: console

  $ amidi -p hw:2,1,0 -d


Then, in a second shell, send some MIDI events (for example note-on/note-off):

.. code-block:: console

  $ amidi -p hw:2,1,0 -S "90427f 804200"

These events should then appear in the first shell (dump)

On devboards with a user button, press it and observe that there are some note
on/note off events delivered to the first shell (dump)

.. code-block:: console

  $ amidi -p hw:2,1,0 -d

  90 40 7F
  80 40 7F
  90 40 7F
  80 40 7F
  [...]

.. zephyr:code-sample:: netmidi2
   :name: MIDI2 network transport
   :relevant-api: midi_ump net_midi2

   Exchange Universal MIDI Packets over the Network MIDI 2.0 protocol

Overview
********

This sample demonstrates usage of the Network MIDI 2.0 stack:

* start a UMP Endpoint host reachable on the network
* respond to UMP Stream discovery messages, so that clients can discover the
  topology described in the device tree
* if ``midi_serial`` port is defined in the device tree,
  send MIDI1 data from UMP group 9 there
* if ``midi_green_led`` node is defined in the device tree,
  light up the led when sending data on the serial port

Requirements
************

This sample requires a board with IP networking support. To perform anything
useful against the running sample, you will also need a Network MIDI2.0 client
to connect to the target, for example `pymidi2`_

Building and Running
********************

The easiest way to try out this sample without any hardware is using
``native_sim``. See :ref:`native_sim ethernet driver <nsim_per_ethe>` to setup
networking on your computer accordingly

.. zephyr-app-commands::
   :zephyr-app: samples/net/midi2/
   :board: native_sim
   :goals: run

The Network MIDI 2.0 endpoint should now be reachable on the network:

.. code-block:: console

  $ pymidi2 find
  Zephyr-UDP-MIDI2 (udp://192.0.2.1:45486)
  - Block #0 [io : Recv/Send] 'Synthesizer' UMP groups {0, 1, 2, 3} [MIDI1 + MIDI2]
  - Block #1 [i- : Recv     ] 'Keyboard' UMP groups {8} [MIDI1 only]
  - Block #2 [-o :      Send] 'External output (MIDI DIN-5)' UMP groups {9} [MIDI1 31.25kb/s]


Furthermore, this sample pairs well with the :ref:`olimex_shield_midi`,
that conveniently defines the device tree nodes for the external MIDI OUT
and its led. For example, using this shield on the ST Nucleo F429zi:

.. zephyr-app-commands::
   :zephyr-app: samples/net/midi2
   :board: nucleo_f429zi
   :shield: olimex_shield_midi
   :goals: build flash

Using authentication
********************

To enable shared secret authentication to connect to the UMP endpoint host,
enable :kconfig:option:`CONFIG_NET_SAMPLE_MIDI2_AUTH_SHARED_SECRET`,
and then configure the desired shared secret in
:kconfig:option:`CONFIG_NET_SAMPLE_MIDI2_SHARED_SECRET`

To enable user/password authentication instead, enable
:kconfig:option:`CONFIG_NET_SAMPLE_MIDI2_AUTH_USER_PASSWORD`, and then
configure the desired username/password in
:kconfig:option:`CONFIG_NET_SAMPLE_MIDI2_USERNAME`
and :kconfig:option:`CONFIG_NET_SAMPLE_MIDI2_PASSWORD`

.. _pymidi2:
   https://github.com/titouanc/pymidi2

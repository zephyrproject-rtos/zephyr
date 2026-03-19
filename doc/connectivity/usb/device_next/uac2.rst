.. _usbd_uac2:

USB Audio Class 2 (UAC2)
########################

The USB Audio Class 2 (UAC2) function enables Zephyr devices to present
themselves as USB audio devices to a host operating system. Most modern
operating systems (Windows 10+, macOS, Linux) include native UAC2 drivers
and will recognise the device without additional host software.

Audio topology
==============

A UAC2 device is described in devicetree as a set of Audio Control entities
that form a signal processing chain. The entities are defined as child nodes
of a :dtcompatible:`zephyr,uac2` node.

The following entity types are supported:

- **Clock Source** (:dtcompatible:`zephyr,uac2-clock-source`): Defines the
  sample clock for the audio function. Can be internal or external,
  fixed or programmable, and optionally SOF-synchronised.

- **Input Terminal** (:dtcompatible:`zephyr,uac2-input-terminal`): Entry point
  for audio data into the function. For USB playback, this represents the
  streaming data received from the host.

- **Output Terminal** (:dtcompatible:`zephyr,uac2-output-terminal`): Exit point
  for audio data from the function, typically connected to a speaker or
  headphone output.

- **Feature Unit** (:dtcompatible:`zephyr,uac2-feature-unit`): Processing unit
  that provides per-channel audio processing such as volume and mute. See
  :ref:`uac2_feature_unit` below.

- **Audio Streaming interface** (:dtcompatible:`zephyr,uac2-audio-streaming`):
  Defines the isochronous endpoint and audio format for streaming data between
  host and device.

A minimal playback topology consists of a clock source, an input terminal, an
output terminal, and an audio streaming interface:

.. code-block:: devicetree

	uac2_headphones: usb_audio2 {
		compatible = "zephyr,uac2";
		status = "okay";
		full-speed;
		high-speed;
		audio-function = <AUDIO_FUNCTION_OTHER>;

		uac_aclk: aclk {
			compatible = "zephyr,uac2-clock-source";
			clock-type = "internal-programmable";
			frequency-control = "host-programmable";
			sampling-frequencies = <48000>;
		};

		out_terminal: out_terminal {
			compatible = "zephyr,uac2-input-terminal";
			clock-source = <&uac_aclk>;
			terminal-type = <USB_TERMINAL_STREAMING>;
			front-left;
			front-right;
		};

		headphones_output: headphones {
			compatible = "zephyr,uac2-output-terminal";
			data-source = <&out_terminal>;
			clock-source = <&uac_aclk>;
			terminal-type = <OUTPUT_TERMINAL_HEADPHONES>;
		};

		as_iso_out: out_interface {
			compatible = "zephyr,uac2-audio-streaming";
			linked-terminal = <&out_terminal>;
			subslot-size = <2>;
			bit-resolution = <16>;
		};
	};

Application callbacks
=====================

The application registers event handlers via :c:func:`usbd_uac2_set_ops`,
passing a :c:struct:`uac2_ops` structure. See :ref:`uac2_device` for the full
API reference.

.. _uac2_feature_unit:

Feature Unit
============

The Feature Unit (FU) is a multi-channel processing unit, part of the UAC 2.0
standard, that provides basic manipulation of Audio Controls on each audio
channel (e.g. left, right) independently. It can also apply controls to all
audio channels at once, implementing 'master' Controls that are cascaded after
the individual channel Controls.

For each audio channel, the Feature Unit optionally provides Audio Controls for
the following features:

- Mute
- Volume
- Tone Control (Bass, Mid, Treble)
- Graphic Equalizer
- Automatic Gain Control
- Delay
- Bass Boost
- Loudness
- Input Gain
- Input Gain Pad
- Phase Inverter

The Zephyr UAC2 implementation supports all of these through
:c:enum:`usb_audio_fucs`. It is up to the application to handle requests, for
example by changing the settings on an external amplifier or by implementing DSP
functionality.

Devicetree configuration
-------------------------

A Feature Unit is inserted into the signal chain between a terminal (or another
unit) and the output terminal. The ``data-source`` property links it to its
upstream entity:

.. code-block:: devicetree

	out_feature_unit: out_feature_unit {
		compatible = "zephyr,uac2-feature-unit";
		data-source = <&out_terminal>;
		mute-control = "host-programmable";
		volume-control =
			"host-programmable",   /* Master (channel 0) */
			"host-programmable",   /* Channel 1 */
			"host-programmable";   /* Channel 2 */
	};

	headphones_output: headphones {
		compatible = "zephyr,uac2-output-terminal";
		data-source = <&out_feature_unit>;
		clock-source = <&uac_aclk>;
		terminal-type = <OUTPUT_TERMINAL_HEADPHONES>;
	};

Each control property is a string array where the first entry applies to the
master channel (channel 0), and subsequent entries apply to individual logical
channels. Each entry can be ``"host-programmable"``, ``"read-only"``, or
omitted if the control is not present.

Application callbacks
---------------------

The application provides Feature Unit callbacks through
:c:struct:`uac2_feature_unit_ops`, which is referenced from :c:struct:`uac2_ops`
via the ``feature_unit_ops`` pointer. The callbacks handle host requests to set
or get current control values (``set_cur_cb``, ``get_cur_cb``) and to query
supported ranges (``get_range_cb``).

The ``get_range_cb`` returns a ``const`` pointer, so the range data can be
stored in read-only memory (ROM). Use :c:macro:`UAC2_RANGE_DEFINE` to declare
a range with a specific number of subranges:

.. code-block:: c

	static const UAC2_RANGE_DEFINE(volume_range, 1) = {
		.num_subranges = 1,
		.ranges = {{
			.min = -40 * 256,  /* -40 dB in 1/256 dB steps */
			.max = 0,          /*   0 dB */
			.res = 256,        /*   1 dB steps */
		}},
	};

A complete reference implementation can be found in
:zephyr:code-sample:`uac2-explicit-feedback`.

Samples
=======

- :zephyr:code-sample:`uac2-explicit-feedback` — Asynchronous playback with
  explicit feedback and Feature Unit support (volume, mute).
- :zephyr:code-sample:`uac2-implicit-feedback` — Asynchronous playback with
  implicit feedback.

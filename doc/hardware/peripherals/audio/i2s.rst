.. _i2s_api:

Inter-IC Sound (I2S) Bus
########################

Overview
********

The I2S (Inter-IC Sound) API provides support for the standard I2S interface
as well as common non-standard extensions such as PCM Short/Long Frame Sync
and Left/Right Justified Data Formats.

Shell
*****

When :kconfig:option:`CONFIG_I2S_SHELL` is enabled, an ``i2s`` shell command is
available to stream a generated sine test tone over any I2S controller without
writing application code. It is transport only; pair it with the ``codec`` shell
to route the audio through a codec.

The tone commands are grouped under ``i2s tone``:

* ``i2s tone start <device> [frequency_hz] [sample_rate] [bits]`` starts a
  stereo test tone on the given I2S device. ``frequency_hz`` defaults to 440 Hz,
  ``sample_rate`` defaults to 48000 Hz, and ``bits`` defaults to 16 (valid
  values are 16, 24 and 32).
* ``i2s tone stop`` stops the running tone.
* ``i2s tone info`` shows the current tone stream status.

For example, to stream a 1 kHz, 48 kHz, 16-bit tone:

.. code-block:: console

   uart:~$ i2s tone start i2s@0 1000 48000 16
   Streaming 1000 Hz tone on i2s@0 @ 48000 Hz, 16-bit stereo
   uart:~$ i2s tone info
   state    : streaming
   i2s dev  : i2s@0
   tone     : 1000 Hz
   rate     : 48000 Hz
   format   : 16-bit stereo, 64-frame blocks x 8
   uart:~$ i2s tone stop
   Stopped

The TX buffering used by the shell is controlled by
:kconfig:option:`CONFIG_I2S_SHELL_BLOCK_FRAMES` and
:kconfig:option:`CONFIG_I2S_SHELL_BLOCK_COUNT`.

Configuration Options
*********************

Related configuration options:

* :kconfig:option:`CONFIG_I2S`
* :kconfig:option:`CONFIG_I2S_SHELL`

API Reference
*************

.. doxygengroup:: i2s_interface

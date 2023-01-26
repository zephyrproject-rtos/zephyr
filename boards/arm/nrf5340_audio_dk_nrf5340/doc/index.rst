.. _nrf5340_audio_dk_nrf5340:

nRF5340 Audio DK
################

Overview
********

The nRF5340 Audio DK (PCA10121) is designed for showcasing, developing and experimenting
with Bluetooth® LE Audio.

You can use this board for developing LE-Audio-compatible applications that support Auracast™,
connected isochronous streams (CIS) and broadcast isochronous streams (BIS),
and offer support for acting as a audio source, audio sink and source + sink.

Zephyr uses the nrf5340_audio_dk_nrf5340 board configuration for building
for the nRF5340 Audio DK.

Hardware
********

The nRF5340 Audio DK comes with the following hardware features:

* nRF5340 dual-core SoC based on the Arm® Cortex®-M33 architecture
* CS47L63 Low-Power Audio DSP with mono differential headphone driver
* nPM1100 Ultra-small form-factor Power Management IC
* On-board digital microphone
* On-board power measurement
* SD card slot
* Built-in debugger
* Stereo analog input using 3.5 mm jack
* USB soundcard capability

.. figure:: img/nrf5340_audio_dk.jpg
     :align: center
     :alt: nRF5340 DK

More information about the board can be found at the `nRF5340 Audio DK website`_. The `Nordic Semiconductor Infocenter`_
contains the processor's information and the datasheet.

nRF5340 SoC
===========

The nRF5340 Audio DK is built around the nRF5340 SoC, which has the following characteristics:

* A full-featured Arm Cortex-M33F core with DSP instructions,
  FPU, and Armv8-M Security Extension, running at up to 128 MHz,
  referred to as the **application core**.
* A secondary Arm Cortex-M33 core, with a reduced feature set,
  running at a fixed 64 MHz, referred to as the **network core**.

The nrf5340_audio_dk_nrf5340_cpuapp build target provides support for the application
core on the nRF5340 SoC. The nrf5340_audio_dk_nrf5340_cpunet build target provides
support for the network core on the nRF5340 SoC.


The `Nordic Semiconductor Infocenter`_ contains the processor's information and
the datasheet.

Supported Features
==================

See :ref:`nrf5340dk_nrf5340` and `Nordic Semiconductor Infocenter`_
for a complete list of nRF5340 Audio DK board hardware features.


Programming and Debugging
*************************

Flashing
========

Follow the instructions in the :ref:`nordic_segger` page to install
and configure all the necessary software. Further information can be
found in :ref:`nordic_segger_flashing`. Then you can build and flash
applications as usual (:ref:`build_an_application` and
:ref:`application_run` for more details).

.. warning::

   The nRF5340 has a flash read-back protection feature. When flash read-back
   protection is active, you will need to recover the chip before reflashing.
   If you are flashing with :ref:`west <west-build-flash-debug>`, run
   this command for more details on the related ``--recover`` option:

   .. code-block:: console

      west flash -H -r nrfjprog --skip-rebuild

.. note::

   Flashing and debugging applications on the nRF5340 Audio DK requires
   upgrading the nRF Command Line Tools to version 10.12.0. Further
   information on how to install the nRF Command Line Tools can be
   found in :ref:`nordic_segger_flashing`.

Debugging
=========

Refer to the :ref:`nordic_segger` page to learn about debugging Nordic
boards with a Segger IC.

References
**********

.. target-notes::

.. _nRF5340 Audio DK website:
   https://www.nordicsemi.com/Products/Development-hardware/nrf5340-audio-dk
.. _Nordic Semiconductor Infocenter: https://infocenter.nordicsemi.com

.. _audio_dmic_api:

Digital Microphone (DMIC)
#########################

Overview
********

The audio DMIC interface provides access to digital microphones.

Digital microphones typically output a PDM (Pulse Density Modulation) bit stream instead of analog
audio. PDM uses a 1-bit, high-rate signal in which audio amplitude is represented by the density of
1s and 0s over time.

Because applications usually consume PCM (Pulse Code Modulation) samples rather than raw PDM data, a
DMIC controller converts the microphone stream into PCM audio. This conversion includes filtering
and decimation: filtering removes the high-frequency noise shaped into the PDM stream, and
decimation reduces the bit-stream rate to the desired PCM sample rate.

From a Zephyr point of view, a DMIC device is an audio capture peripheral. The application
configures the microphone and controller, starts capture, and reads PCM buffers from the driver.

Key concepts
************

The DMIC API separates configuration into three pieces that mirror the capture path:

#. How the microphone is driven on the PDM side,
#. How incoming PDM channels are arranged, and
#. How converted PCM samples are delivered to the application.

These pieces are combined in :c:struct:`dmic_cfg` and passed to :c:func:`dmic_configure`.

**PDM I/O configuration** (:c:member:`dmic_cfg.io`)
  This describes the electrical and timing requirements of the microphone interface, such as the
  supported PDM clock frequency range, duty cycle, and any controller-specific signal polarity
  settings.

**Channel configuration** (:c:member:`dmic_cfg.channel`)
  This tells the driver which physical PDM controller and left or right microphone lane should
  appear as each logical audio channel in the PCM output. It also declares how many channels and
  streams the application wants to use.

**PCM stream configuration** (:c:member:`dmic_cfg.streams`)
  This defines the PCM data delivered by the driver, including sample rate, sample width, block
  size, and the :c:struct:`k_mem_slab` used to allocate receive buffers for each enabled stream.

Typical application flow
========================

Typical use of the DMIC API is:

#. Get the DMIC device, usually from devicetree.
#. Fill a :c:struct:`dmic_cfg` structure with I/O, channel, and stream settings. See the
   :ref:`dmic_buffering` section below for more details on how to define the memory buffer the DMIC
   uses to store received PCM data.
#. Call :c:func:`dmic_configure`, passing the configuration structure.
#. Start capture with :c:func:`dmic_trigger` using :c:enumerator:`DMIC_TRIGGER_START`.
#. Fetch PCM data with :c:func:`dmic_read`.
#. Stop, pause, or reset capture with additional trigger commands as needed.

.. _dmic_buffering:

Buffering
=========

Received PCM data is returned through buffers owned by the driver. Applications provide the
backing memory through a :c:struct:`k_mem_slab` referenced by each configured PCM stream.

A common pattern is to declare this slab statically:

.. code-block:: c
  :caption: Declaring a memory slab statically for PCM receive buffers

   K_MEM_SLAB_DEFINE_STATIC(mem_slab,
                            SAMPLES_PER_BUFFER * sizeof(int16_t),
                            BUFFER_COUNT,
                            sizeof(void *));

In this example, each slab block stores one PCM buffer and ``BUFFER_COUNT`` sets how many buffers
can be queued internally before the application reads them using :c:func:`dmic_read`.

Configuration Options
*********************

Related configuration options:

* :kconfig:option:`CONFIG_AUDIO_DMIC`

API Reference
*************

.. doxygengroup:: audio_dmic_interface

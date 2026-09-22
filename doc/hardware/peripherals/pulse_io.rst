.. _pulse_io_api:

Pulse IO
########

Overview
********

The Pulse IO subsystem provides a vendor-neutral API for hardware that
generates and captures timed digital edges on a single GPIO line. Several
MCU families ship a dedicated peripheral for this purpose under different
names. Pulse IO abstracts that hardware class so client drivers such as
addressable LED strips, infrared transmit and receive, stepper pulse
generation, frequency and duty measurement, and one-wire style protocols
can bind once and run on any SoC that provides a backend.

Related configuration options:

* :kconfig:option:`CONFIG_PULSE_IO`

Submission modes
****************

A channel is locked to one submission mode at configure time, selected
from those the backend advertises through its capabilities.

``PULSE_IO_MODE_SYMBOL``
   The application submits an array of :c:struct:`pulse_symbol`, each
   carrying an explicit level and a duration in ticks of the configured
   resolution. Consecutive symbols may have arbitrary, unrelated
   durations. Use it for protocols whose edge lengths vary within one
   stream, such as infrared remotes, one-wire and stepper acceleration
   ramps.

``PULSE_IO_MODE_CELL``
   The application submits an array of :c:struct:`pulse_cell`. Every cell
   has the same period, set once in the channel configuration, and each
   cell carries either a level or a duty value within that period. Use it
   for naturally periodic streams where only the per-period level or duty
   changes, such as addressable LED bit shaping or varying-duty PWM. Cells
   are more memory efficient than symbols for these cases.

Backends advertise the supported modes in
:c:struct:`pulse_io_caps`. Requesting an unsupported mode causes
:c:func:`pulse_io_channel_configure` to return ``-ENOTSUP``.

Configuration
*************

Clients query :c:func:`pulse_io_get_capabilities` at probe time to decide
which mode and feature set to use, reserve a channel with
:c:func:`pulse_io_channel_get`, and configure it with
:c:func:`pulse_io_channel_configure` before any transfer. Blocking
transfers use :c:func:`pulse_io_transmit_sync` and
:c:func:`pulse_io_receive_sync`; asynchronous and streaming transfers use
the RTIO path.

The byte-to-symbol helper :c:func:`pulse_io_encode_bytes` and its inverse
:c:func:`pulse_io_decode_bytes` convert between a protocol byte stream and
pulse symbols using a per-bit template.

RTIO
****

A backend may optionally integrate with :ref:`rtio`, exposing an iodev
submit path along with encoder and decoder vtables fetched through
:c:func:`pulse_io_get_encoder` and :c:func:`pulse_io_get_decoder`. With
RTIO the client encodes a payload into a symbol buffer, submits it through
an RTIO queue, and decodes any captured reply, gaining queued and chained
transfers, streaming receive, and the userspace path provided by the RTIO
framework. The RTIO ops are optional; a backend that does not implement
them leaves the corresponding driver API members ``NULL`` and the
accessors return ``-ENOSYS``.

API Reference
*************

.. doxygengroup:: pulse_io_interface

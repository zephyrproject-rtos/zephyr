.. _mipi_stp_decoder:

MIPI STP Decoder
################

The MIPI System Trace Protocol (MIPI STP) was developed as a generic base protocol that can
be shared by multiple application-specific trace protocols. It serves as a wrapper protocol
that merges disparate streams that typically contain different trace protocols from different
trace sources. Stream consists of opcode (shortest is 4 bit long) followed by optional data and
optional timestamp. There are opcodes for data (8, 16, 32, 64 bit data marked/not marked, with or
without timestamp), stream recognition (master and channel), synchronization (ASYNC opcode) and
others.

One example where protocol is used is ARM Coresight STM (System Trace Macrocell) where data
written to Stimulus Port registers maps directly to STP stream.

This module can be used to perform on-chip decoding of the data stream. STP v2 is used.

Usage
*****

Decoder is initialized with a callback. A callback is called on each decoded opcode.
Decoder has internal state since there are dependency between opcodes (e.g. timestamp can be
relative). Decoder can be in synchronization or not. Initial state is configurable.
If decoder is not synchronized to the stream then it decodes each nibble in search for ASYNC opcode.
Loss of synchronization can be indicated to the decoder by calling
:c:func:`mipi_stp_decoder_sync_loss`. :c:func:`mipi_stp_decoder_decode` is used to decode the data.

Limitations
***********

There are following limitations:

* Decoder supports only little endian architectures.
* When decoding nibbles, it is more efficient when core supports unaligned memory access.
  Implementation supports optimized version with unaligned memory access and generic one.
  Optimized version is used for ARM Cortex-M (expect for M0).
* Limited set of the most common opcodes is implemented.

API documentation
*****************

.. doxygengroup:: mipi_stp_decoder_apis

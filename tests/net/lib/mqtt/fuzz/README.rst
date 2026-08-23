.. SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
.. SPDX-FileCopyrightText: Copyright (c) 2026 Dev It Wise
.. SPDX-License-Identifier: Apache-2.0

MQTT receive-path fuzz harness
##############################

What it covers
**************

Everything an MQTT client decodes comes from the broker it is connected
to, so every byte reaching :c:func:`mqtt_input` is chosen by whoever
holds the other end of the connection. The harness hands the fuzz case
to the client as that byte stream, through a custom transport, and then
drives :c:func:`mqtt_input` the way an application's poll loop does
until the client rejects the stream or the peer runs out of data.

Driving the real entry point rather than a decoder is what keeps a
finding meaningful. The length fields worth aiming at are read in
:file:`mqtt_rx.c` before any decoder sees them:

* the fixed header's variable-length remaining length, which decides how
  much of the stream is buffered,
* the PUBLISH topic length and, under 5.0, the property length, which
  :c:func:`mqtt_read_publish_var_header` adds up to decide how much of
  the variable header to read before handing it on,
* the UTF-8 string, binary-data and variable-integer lengths inside the
  packet, which the decoders convert into offsets into the receive
  buffer,
* the 5.0 property length, which is a second budget the property walk
  spends alongside the buffer end.

Past the decoder the harness does what an application does with the
result: it reads both ends of the PUBLISH topic, so a topic that escaped
the receive buffer - or that was resolved out of the client's 5.0
topic-alias store - is reported here rather than in the application that
would have copied it out, and it drains the payload with
:c:func:`mqtt_read_publish_payload`, which is the only thing that moves
the client's remaining-payload accounting.

A connection reaches the packet types worth fuzzing only once the broker
has accepted it, so the transport serves a CONNACK of its own before the
fuzz case and the fuzz case is the stream that follows it. The same
stream is offered twice, to a client speaking 3.1.1 and to one speaking
5.0, because the version is settled when the client connects and the two
decoders diverge from the CONNACK onwards.

Building and running
********************

libFuzzer needs clang, and the fuzzing support in the POSIX
architecture is available on ``native_sim`` and
``native_sim/native/64``. The harness is restricted to the 64-bit
variant, as :file:`samples/subsys/debug/fuzz` is:

.. code-block:: console

   python3 tests/net/lib/mqtt/fuzz/gen_corpus.py /tmp/mqtt-corpus
   export ZEPHYR_TOOLCHAIN_VARIANT=host/llvm
   west build -p -b native_sim/native/64 tests/net/lib/mqtt/fuzz
   ./build/zephyr/zephyr.exe /tmp/mqtt-corpus \
       -dict=tests/net/lib/mqtt/fuzz/mqtt.dict -max_len=256 \
       -max_total_time=300

``CONFIG_ASAN`` and ``CONFIG_UBSAN`` are set by the harness's
:file:`prj.conf`, and the build turns off sanitizer recovery: UBSan
reports and continues by default, which would let libFuzzer record a
clean run over an input that triggered undefined behaviour.

The harness gives the client a 256-byte receive buffer and skips an
input longer than that. In-tree the buffer an application hands the
client is anything from 64 bytes, in the MQTT shell backend, to 1024
in the Azure sample; 256 is the size the 5.0 packet test uses. The
buffer is what bounds a single packet - a message the client cannot
buffer is refused rather than parsed - so ``-max_len=256`` keeps the
mutator inside the range that is executed at all.

Replaying the corpus alone, without fuzzing, is a fast regression check:

.. code-block:: console

   ./build/zephyr/zephyr.exe -runs=0 /tmp/mqtt-corpus

Proof the harness can fail
**************************

A clean run says nothing until the harness has been shown to report a
defect it is aimed at. Deleting the bounds check on the 5.0 topic alias
in :c:func:`publish_topic_alias_check` - the check that keeps a value
taken from the wire from indexing the client's own array of aliases -
makes the corpus abort on the seed written for it:

.. code-block:: console

   $ ./build/zephyr/zephyr.exe -runs=0 \
         /tmp/mqtt-corpus/publish_topic_alias_out_of_range.bin
   mqtt_decoder.c:783:18: runtime error: index 65534 out of bounds for
   type 'struct mqtt_topic_alias[5]'
   SUMMARY: UndefinedBehaviorSanitizer: undefined-behavior
   $ echo $?
   1

With the check in place the same seed is parsed and rejected without a
sanitizer report. The client and its buffers are allocated per run
rather than kept as globals for the same reason: an index or an offset
that leaves one of them lands in guarded memory and is reported, instead
of reaching whatever the linker placed next.

What it does not cover
**********************

The transport hands the client a short read when the rest of a message
has not arrived, which is what a stream socket does, so the
``-EAGAIN`` that :c:func:`mqtt_read_message_chunk` returns for a
partial message is reached. What is not reached is a transport that
fails: the harness's read never returns a negative value, so the
branch in :c:func:`mqtt_read_message_chunk` that forwards a transport
error, and the non-blocking early return in the publish payload
reader, are both dead here. Nothing else in the receive path depends
on them, so this is a coverage gap rather than a source of false
findings.

The other gap is the size regime above the harness's own buffer. The
client is given 256 bytes and an input longer than that is skipped, so
the 257 to 1024 byte packets an application with a larger buffer would
accept are never parsed. Raising ``FUZZ_RX_BUF_SIZE`` and
``-max_len`` together is what covers them.

Seed corpus
***********

An MQTT packet is dense with zero bytes: every string length, every
short remaining length and every success reason code is one. Git calls a
file binary as soon as it contains a NUL and the tree takes no binary
files, so the seeds are kept as hex in :file:`seeds.txt` and
:file:`gen_corpus.py` writes the directory of binary seeds libFuzzer
wants. Each seed is the stream a broker sends after the connection has
been accepted.

.. list-table::
   :header-rows: 1

   * - Seed
     - Stream
   * - ``publish_qos0``
     - The shortest PUBLISH carrying a topic and a payload.
   * - ``publish_topic_alias``
     - A QoS 1 PUBLISH that sets a 5.0 topic alias, followed by one that
       carries no topic and resolves it from the alias, which is the
       only path that reads a topic out of the client's own store.
   * - ``publish_user_property``
     - A user property, whose name and value lengths are spent against
       the property-length budget rather than the buffer end.
   * - ``connack_properties``
     - A CONNACK with properties, decoded on an already connected
       client. A conformant broker never sends a second one; the
       decoder takes it without checking the connection state.
   * - ``suback_reason_codes``
     - A message id, an empty property list and three reason codes,
       where the packet length decides how many codes are read.
   * - ``disconnect_reason``
     - A DISCONNECT with a reason code and no properties.
   * - ``auth_method``
     - An AUTH continuing authentication, with an authentication method
       string.
   * - ``publish_topic_alias_out_of_range``
     - A topic alias past the configured maximum, which is the input the
       alias bounds check exists for.
   * - ``truncated_remaining_length``
     - A fixed header announcing the largest remaining length the
       encoding allows, with nothing behind it.

The corpus is a seed set, not an accumulating one: a new seed belongs in
it only when it reaches code the existing ones do not, and it is
minimised (``-minimize_crash=1``, then ``-merge=1``) before its hex is
added to :file:`seeds.txt`.

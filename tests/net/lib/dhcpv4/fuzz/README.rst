.. SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
.. SPDX-FileCopyrightText: Copyright (c) 2026 Dev It Wise
.. SPDX-License-Identifier: Apache-2.0

DHCPv4 client receive-path fuzz harness
#######################################

What it covers
**************

A DHCPv4 client accepts its configuration from whichever server answers
its DISCOVER first, so every byte the client decodes is chosen by
whoever is on the LAN. There is no authentication anywhere in the
exchange, and the client is running before the device has an address,
which makes this one of the first parsers a Zephyr device exposes.

The harness does not call the decoder directly. It registers a dummy
interface, exactly as :file:`tests/net/dhcpv4/client/src/main.c` does,
starts the real client with :c:func:`net_dhcpv4_start`, builds a full
IPv4 + UDP + DHCP packet around the fuzz case and hands it to the stack
with :c:func:`net_recv_data`, which is the entry point a driver uses for
an inbound frame. What gets fuzzed is therefore the path a packet
actually takes, including the checks in :file:`dhcpv4.c` that run before
the option parser.

Two fields are patched into the case rather than left to the mutator:
the transaction id, which the client compares against the one it chose,
and the client hardware address, which it compares against the
interface's own. Both are in the client's broadcast DISCOVER for anyone
on the LAN to read, so pinning them is what an attacker would do anyway;
leaving them to the mutator would mean nearly every case is dropped
before the option parser runs.

The option decoder has a second entry point besides its own switch: an
application can register a plain option callback and a vendor-specific
(option 43) callback, each backed by a fixed-size buffer the decoder
fills before invoking the handler. The harness registers one of each,
with buffers deliberately shorter than the options the seeds carry, so
the clamp the decoder applies is exercised rather than assumed.

Building and running
********************

libFuzzer needs clang, and the fuzzing support in the POSIX architecture
is available on ``native_sim`` and ``native_sim/native/64``. The harness
is restricted to the 64-bit variant, as :file:`samples/subsys/debug/fuzz`
is:

.. code-block:: console

   python3 tests/net/lib/dhcpv4/fuzz/gen_corpus.py /tmp/dhcpv4-corpus
   export ZEPHYR_TOOLCHAIN_VARIANT=host/llvm
   west build -p -b native_sim/native/64 tests/net/lib/dhcpv4/fuzz
   ./build/zephyr/zephyr.exe /tmp/dhcpv4-corpus -max_len=512 \
       -max_total_time=300

``CONFIG_ASAN`` and ``CONFIG_UBSAN`` are set by the harness's
:file:`prj.conf`, and the build turns off sanitizer recovery: UBSan
reports and continues by default, which would let libFuzzer record a
clean run over an input that triggered undefined behaviour.

How a case is executed
**********************

The case is not executed by ``LLVMFuzzerTestOneInput`` itself. It is
stored, an interrupt is raised, and the native simulator is allowed to
run, so an ordinary Zephyr thread picks the case up and drives it. This
follows :file:`samples/subsys/debug/fuzz`. The client arms a delayable
work item when it is started and takes the interface's state apart under
a lock as it decodes, none of which the kernel expects to be driven from
outside a thread.

``FUZZ_TICKS`` is what bounds the window. The sample needs two ticks for
a semaphore handover; this harness needs the woken thread to start the
client and get a packet through :c:func:`net_recv_data` to the client's
own input callback, each a scheduling point of its own. The value in the
source was found by doubling until every seed reached the option parser
inside its own case, then kept with headroom.

``CONFIG_NET_TC_RX_COUNT=0`` covers the other half. With a receive
queue configured, :c:func:`net_recv_data` hands the packet to the
receive thread and the parse happens after the fuzz case has already
returned, so a crash would be attributed to whichever input libFuzzer
executed next and the reproducer it wrote would not replay. With no
queues the packet is processed in the caller's context, which is what
makes a finding here reproducible.

A case shorter than the fixed DHCP header, or longer than 512 bytes, is
skipped rather than padded or truncated, so the case the corpus keeps is
the case that was executed.

Replaying the corpus alone, without fuzzing, is a fast regression check:

.. code-block:: console

   ./build/zephyr/zephyr.exe -runs=0 /tmp/dhcpv4-corpus

Proof the harness can fail
**************************

A clean run says nothing until the harness has been shown to report a
defect it is aimed at. In :c:func:`dhcpv4_parse_options` the subnet mask
option's length is checked against the four byte address it is read
into. Disabling that one check makes the corpus abort on the seed
written for it:

.. code-block:: console

   $ ./build/zephyr/zephyr.exe -runs=0 \
         /tmp/dhcpv4-corpus/opt_subnet_mask_oversize.bin
   ERROR: AddressSanitizer: stack-buffer-overflow on address 0x7f7403aa40b4
   WRITE of size 16 at 0x7f7403aa40b4 thread T3

16 is what that seed's option declares. With the check back in place the
whole corpus replays without a sanitizer report.

The callback path is covered the same way, and it is worth being precise
about what it proves. Replacing ``MIN(cb->max_length, length)`` with
``length`` in the harness's own handler makes it read past the buffer the
decoder filled, and ASan reports a ``global-buffer-overflow``, ``READ of
size 1``, on ``plain_callback_domain_name.bin``. That is a check on the
harness, not on the decoder: it shows the handler's sink notices a read
outside ``cb->data``, which is what would catch a decoder that one day
writes more into that buffer than it should.

What it does not cover
**********************

The client's transmit path is not fuzzed. What the client sends is
encoded from its own state, not from anything a peer chose, so it is
outside what this harness is aimed at, and the dummy driver discards it.

The DHCPv4 server in :file:`subsys/net/lib/dhcpv4/dhcpv4_server.c`
parses messages from clients on the same LAN and is a separate surface
this harness does not touch. It would want a harness of its own.

Each case runs against a client restarted from a known state, so a
defect that needs two messages in sequence - an OFFER that steers the
client into a state a later ACK then abuses - is not reachable here.
Fuzzing a sequence rather than a message is what would cover it.

Seed corpus
***********

A DHCP message is mostly zero bytes: the fixed header, SNAME and FILE
fields are almost entirely padding. Git calls a file binary as soon as
it contains a NUL and the tree takes no binary files, so the seeds are
kept as hex in :file:`seeds.txt` and :file:`gen_corpus.py` writes the
directory of binary seeds libFuzzer wants. Each seed is the DHCP message
body, which is what the harness appends after the IPv4 and UDP headers.

.. list-table::
   :header-rows: 1

   * - Seed
     - Message
   * - ``offer``
     - The OFFER from :file:`tests/net/dhcpv4/client/src/main.c`, byte
       for byte: the common option set plus three vendor-specific edge
       cases in one message.
   * - ``ack``
     - The ACK from the same file, the same option set in the order the
       server really encodes it.
   * - ``nak``
     - A NAK and nothing else, which is the branch the OFFER and ACK
       seeds never reach.
   * - ``opt_length_overrun``
     - A subnet mask declaring four bytes with two left in the message,
       and no end marker behind it.
   * - ``opt_zero_length``
     - A subnet mask declaring no payload at all, on a path that expects
       a fixed four bytes.
   * - ``no_end_marker``
     - One well-formed option and then the buffer ends, so the loop
       leaves through the read failure rather than through the end
       marker.
   * - ``vendor_bad_inner_length``
     - An option 43 whose inner sub-option declares more than the outer
       option's payload holds.
   * - ``plain_callback_domain_name``
     - A domain name longer than the plain callback's buffer, which is
       what makes the callback depend on the decoder's clamp.
   * - ``vendor_callback_hit``
     - A well-formed inner sub-option matching the registered vendor
       callback code, the counterpart to the malformed seed above.
   * - ``opt_subnet_mask_oversize``
     - A subnet mask declaring sixteen bytes, with sixteen bytes behind
       it, against the four byte address the decoder reads it into. This
       is the input the option's length check exists for.

The corpus is a seed set, not an accumulating one: a new seed belongs in
it only when it reaches code the existing ones do not, and it is
minimised (``-minimize_crash=1``, then ``-merge=1``) before its hex is
added to :file:`seeds.txt`.

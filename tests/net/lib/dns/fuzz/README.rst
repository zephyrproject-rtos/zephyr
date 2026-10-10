.. SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
.. SPDX-FileCopyrightText: Copyright (c) 2026 Dev It Wise
.. SPDX-License-Identifier: Apache-2.0

DNS parser fuzz harness
#######################

What it covers
**************

The harness feeds the fuzz case in as a received datagram and drives it
through both consumers the tree has for one, each following the call
sequence of the code that owns it.

The responder path, as :file:`subsys/net/lib/dns/mdns_responder.c` and
:file:`subsys/net/lib/dns/llmnr_responder.c` run it:

* :c:func:`mdns_unpack_query_header`, then one
  :c:func:`dns_unpack_query` per announced question, which walks the
  name through :c:func:`dns_unpack_name` and unpacks the query type and
  class after it
* the step the responder takes on the unpacked name, the ``.local``
  suffix test, so a name that reached the buffer is read back out here
  rather than only in the responder

The resolver path, as :c:func:`dns_read` in
:file:`subsys/net/lib/dns/resolve.c` runs it: :c:func:`dns_validate_msg`,
which re-reads the header, walks the question, walks every answer record
through :c:func:`dns_unpack_answer`, converts each record's rdata to an
address, a name, a text record or an SRV target, and copies a CNAME out
with :c:func:`dns_copy_qname`.

A resolver reply is accepted from whoever wins the race to the socket, and
an mDNS or LLMNR query is accepted from anyone on the link, so every byte
either path sees is attacker chosen.

Reaching the answer walk
************************

:c:func:`dns_validate_msg` only looks at a reply that matches a query the
resolver has outstanding: it hashes the question the reply carries and
looks for a pending query with that hash and that DNS id. A harness that
left the pending query fixed would have every fuzz case rejected at that
comparison, and the answer walk would never be reached.

So the harness sets the pending query up from the question the fuzz case
carries, computing the hash the way :c:func:`update_query_idx` does, on a
copy, so the bytes handed to the parser are untouched. That is the state a
spoofer produces by echoing the question back at the resolver, which is
what an off-path attacker sends, so a record reached this way is reached
by a packet a peer can send and not by a state the resolver cannot be in.

Building and running
********************

libFuzzer needs clang, and the fuzzing support in the POSIX architecture
is available on ``native_sim/native/64`` only:

.. code-block:: console

   export ZEPHYR_TOOLCHAIN_VARIANT=host/llvm
   west build -p -b native_sim/native/64 tests/net/lib/dns/fuzz
   ./build/zephyr/zephyr.exe corpus \
       -dict=tests/net/lib/dns/fuzz/dns.dict -max_len=512

``CONFIG_ASAN`` and ``CONFIG_UBSAN`` are set by the harness's
:file:`prj.conf`, and the build turns off sanitizer recovery: UBSan
reports and continues by default, which would let libFuzzer record a
clean run over an input that triggered undefined behaviour.

The harness rejects an input longer than ``DNS_RESOLVER_MAX_BUF_SIZE``,
which is what both consumers clamp a received datagram to before the
parser sees it, so pass ``-max_len=512`` and let the mutator spend its
executions inside the range that is executed at all.

No seed corpus
**************

The tree accepts no binary files, and git calls a file binary as soon as
it contains a NUL byte. Every DNS message contains one: a name ends in
the zero octet of the root label, the class of a record that parses at
all is ``0x0001``, and the counts in the header are small. Unlike the
CoAP harness next door, there is no useful seed that avoids one, so no
corpus is committed.

The structure lives in :file:`dns.dict` instead, which is a text file and
takes escapes: whole 12-byte headers, label runs, compression pointers,
question trailers and record trailers. Point the run at an empty
directory and libFuzzer builds its own corpus there, which is worth
keeping between runs.

.. code-block:: console

   mkdir -p /tmp/dns-corpus
   ./build/zephyr/zephyr.exe /tmp/dns-corpus \
       -dict=tests/net/lib/dns/fuzz/dns.dict -max_len=512 \
       -max_total_time=300

Where the message is placed
***************************

The harness copies the fuzz case flush against the end of its buffer
rather than to the start of it. A record whose declared rdata length runs
past the end of the message is the defect class this parser has had
before, and read into the slack of an oversized receive buffer it is
silent: the bytes are whatever the previous datagram left there, which
is a leak rather than a fault. Ending the copy on the last byte of the
object puts the sanitizer's red zone immediately after the message, so a
read past the end of the packet is reported instead of quietly resolving
to stale data.

Proof the harness finds what it targets
***************************************

commit 58b46c81c679 ("net: dns: validate rdata length in
dns_unpack_answer") fixed exactly that: an RR whose ``rdlength``
extended past the packet, which the TXT and SRV consumers in
:file:`resolve.c` then read out. Drop that check again and replaying the
corpus stops on it:

.. code-block:: console

   $ ./build/zephyr/zephyr.exe /tmp/dns-corpus -runs=0
   ==3335765==ERROR: AddressSanitizer: global-buffer-overflow
   SUMMARY: AddressSanitizer: global-buffer-overflow in __asan_memcpy
   ==3335765==ABORTING

With the check in place the same corpus replays clean.

What the parser survived
************************

2556236 executions over 901 seconds in a single process produced no
ASan or UBSan report. The corpus reaches :c:func:`dns_unpack_name`,
:c:func:`dns_unpack_answer`, :c:func:`dns_unpack_query`,
:c:func:`dns_copy_qname`, :c:func:`dns_unpack_response_query` and
:c:func:`dns_validate_record`, which ``-print_coverage=1`` will confirm
on a fresh checkout.

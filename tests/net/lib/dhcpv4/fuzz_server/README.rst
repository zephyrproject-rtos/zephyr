.. SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
.. SPDX-FileCopyrightText: Copyright (c) 2026 Dev It Wise
.. SPDX-License-Identifier: Apache-2.0

DHCPv4 server fuzz harness
##########################

What it covers
**************

The DHCPv4 server answers any client on the link, and a client is
unauthenticated by definition: it has no address yet, which is what it is
asking for. Every byte the server decodes is therefore chosen by whoever
is on the LAN, and the server holds state on their behalf, a lease table
keyed by what they send.

The harness registers a dummy interface, starts the real server with
:c:func:`net_dhcpv4_server_start`, builds a full IPv4 + UDP + DHCP packet
around the fuzz case with destination port 67, and hands it to the stack
with :c:func:`net_recv_data`, which is the entry point a driver uses for
an inbound frame. What gets fuzzed is the path a packet really takes,
including the socket read in the server's own callback.

The server is restarted for each case, so a lease reserved, allocated or
declined by one case cannot steer how the next case is handled.

How a case is executed
**********************

The case is not executed by ``LLVMFuzzerTestOneInput`` itself. It is
stored, an interrupt is raised, and the native simulator is allowed to
run, so an ordinary Zephyr thread picks the case up and drives it. This
follows :file:`samples/subsys/debug/fuzz`, and here it is a requirement
rather than a preference: the server reads its socket from the socket
service thread, and calling into the kernel from the fuzzer's own context
does not reach it.

``FUZZ_TICKS`` is what bounds the window: the ISR's semaphore handover,
and then everything the woken thread sets in motion, restarting the
server and getting :c:func:`net_recv_data` through to the socket service
thread. Every seed already reaches the parser inside its own case at the
sample's default of two ticks. The larger value in the source is
headroom for a mutated case that needs more scheduling points than any
seed does, and it is not free: 100 ticks costs roughly a fifth of the
throughput against 2, and 500 costs about half.

``CONFIG_NET_TC_RX_COUNT=0`` covers the other half. With a receive queue
configured, :c:func:`net_recv_data` hands the packet to the receive
thread, which is a second way for a case to be parsed after the harness
has moved on.

Building and running
********************

libFuzzer needs clang, and the fuzzing support in the POSIX architecture
is available on ``native_sim`` and ``native_sim/native/64``. The harness
is restricted to the 64-bit variant, as :file:`samples/subsys/debug/fuzz`
is:

.. code-block:: console

   python3 tests/net/lib/dhcpv4/fuzz_server/gen_corpus.py /tmp/dhcpv4-server-corpus
   export ZEPHYR_TOOLCHAIN_VARIANT=host/llvm
   west build -p -b native_sim/native/64 tests/net/lib/dhcpv4/fuzz_server
   ./build/zephyr/zephyr.exe /tmp/dhcpv4-server-corpus -max_len=512 \
       -max_total_time=300

``CONFIG_ASAN`` and ``CONFIG_UBSAN`` are set by the harness's
:file:`prj.conf`, and the build turns off sanitizer recovery: UBSan
reports and continues by default, which would let libFuzzer record a
clean run over an input that triggered undefined behaviour.

A case shorter than the fixed DHCP header, or longer than 512 bytes, is
skipped rather than padded or truncated, so the case the corpus keeps is
the case that was executed.

Replaying the corpus alone, without fuzzing, is a fast regression check:

.. code-block:: console

   ./build/zephyr/zephyr.exe -runs=0 /tmp/dhcpv4-server-corpus

Executions per second are lower here than in a harness that decodes a
buffer directly, because each case restarts the server and spends a fixed
simulated-time window. That is the cost of driving the real server rather
than a function inside it.

Proof the harness can fail
**************************

A clean run says nothing until the harness has been shown to report a
defect it is aimed at. In :c:func:`dhcpv4_find_client_id_option` the
option's declared length is checked against the buffer that receives it
before the copy. Disabling that one check makes the corpus abort on the
seed written for it:

.. code-block:: console

   $ ./build/zephyr/zephyr.exe -runs=0 \
         /tmp/dhcpv4-server-corpus/client_id_oversize.bin
   ERROR: AddressSanitizer: stack-buffer-overflow on address 0x77b78eb865c7
   WRITE of size 32 at 0x77b78eb865c7 thread T7

32 is what that seed's option declares, against the 20 bytes
``DHCPV4_CLIENT_ID_MAX_SIZE`` gives the caller. With the check back in
place the whole corpus, that seed included, replays without a sanitizer
report.

What it does not cover
**********************

The server's replies are not examined. They are encoded from the server's
own state rather than from anything a peer chose, and the dummy driver
discards them.

Each case runs against a freshly restarted server, so a defect that needs
two messages in sequence, where one message leaves the lease table in a
state the next one abuses, is not reachable here. Fuzzing a sequence
rather than a message is what would cover it, and it would need the
per-case reset to go, which is what keeps a finding reproducible.

The client side, in :file:`subsys/net/lib/dhcpv4/dhcpv4.c`, is a separate
receive path with its own message types and its own state, and is not
touched here.

Seed corpus
***********

A DHCP message is mostly zero bytes: the fixed header, SNAME and FILE
fields are almost entirely padding. Git calls a file binary as soon as it
contains a NUL and the tree takes no binary files, so the seeds are kept
as hex in :file:`seeds.txt` and :file:`gen_corpus.py` writes the directory
of binary seeds libFuzzer wants. Each seed is the DHCP message body, which
is what the harness appends after the IPv4 and UDP headers.

The seeds are listed with what each one reaches in :file:`seeds.txt`
itself, next to the bytes, so the two cannot drift apart.

The corpus is a seed set, not an accumulating one: a new seed belongs in
it only when it reaches code the existing ones do not, and it is minimised
(``-minimize_crash=1``, then ``-merge=1``) before its hex is added.

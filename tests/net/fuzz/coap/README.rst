.. _net_coap_fuzz:

CoAP parser fuzz target
#######################

Overview
********

A coverage-guided `libFuzzer <https://llvm.org/docs/LibFuzzer.html>`_ target for
:c:func:`coap_packet_parse` and the accessors an application calls on the packet
it returns.

CoAP datagrams arrive from the network, so the parser runs on wholly
attacker-controlled bytes. Each input is copied into a private buffer, parsed,
and then walked the way a real application walks it: header fields, token,
payload and a URI-Path option lookup. Running the accessors matters as much as
running the parser, because a parse which succeeds but records inconsistent
offsets only shows up when something reads them.

The target calls the parser directly from ``LLVMFuzzerTestOneInput()``, which
runs in the host environment outside the embedded OS. There is no scheduling and
no simulated time, so every input is deterministic and costs about a
microsecond. See :zephyr_file:`samples/subsys/debug/fuzz` for the other model,
where input reaches a running Zephyr instance through a simulated interrupt;
that is the right choice for code which cannot be called directly, at the cost
of scheduling the OS once per input.

Requirements
************

Clang, and a host with libFuzzer available. GCC cannot build this target.

Building and Running
********************

.. note::

   The toolchain variant must be ``host/llvm``. Plain ``llvm`` is silently
   resolved to ``host`` for native targets, and the build then fails inside an
   unrelated file with ``gcc: error: unrecognized argument to '-fsanitize='
   option: 'fuzzer'``.

.. code-block:: console

   ZEPHYR_TOOLCHAIN_VARIANT=host/llvm \
   west build -b native_sim/native/64 tests/net/lib/coap/fuzz

Run it with a working corpus of your own, listing the in-tree seeds after it:

.. code-block:: console

   mkdir -p /tmp/coap-corpus
   build/zephyr/zephyr.exe /tmp/coap-corpus tests/net/lib/coap/fuzz/corpus

.. important::

   libFuzzer writes newly discovered inputs into the **first** corpus directory
   given and only reads the rest. Always pass a working directory first and
   :file:`corpus/` after it. Passing :file:`corpus/` on its own makes it the
   output directory, and a short run will drop hundreds of generated files into
   the source tree.

Useful invocations:

.. code-block:: console

   # bounded run, for CI or a quick check
   build/zephyr/zephyr.exe -max_total_time=60 /tmp/coap-corpus <seeds>

   # reproduce a crash artifact
   build/zephyr/zephyr.exe ./crash-<hash>

Seed corpus
***********

Five PDUs, taken from the unit tests in :zephyr_file:`tests/net/lib/coap`, so
the fuzzer starts from structurally correct packets rather than discovering the
four-byte header by chance. Three are well formed and two are malformed in
ways the parser has to reject:

.. list-table::
   :header-rows: 1
   :widths: 26 74

   * - Seed
     - Contents
   * - ``empty_con_get``
     - Confirmable GET, no token, no options. The minimal valid packet.
   * - ``con_get_one_option``
     - The same, plus one zero-length option.
   * - ``non_response_with_token``
     - Non-confirmable 5.05 response carrying a five-byte token.
   * - ``truncated_token``
     - Declares a five-byte token but supplies four bytes.
   * - ``truncated_header``
     - Three bytes, shorter than the fixed four-byte header.

``CONFIG_ASAN=y`` is enabled in :file:`prj.conf`; add ``CONFIG_UBSAN=y`` to
catch undefined behaviour that AddressSanitizer does not.

Adding more targets
*******************

The harness is deliberately small. To fuzz another parser that takes a buffer
and a length, copy this directory, change the parse call and the accessors that
follow it, and seed the corpus from that subsystem's unit tests.

Two things are worth preserving when you do:

* Copy the input before parsing. Fuzzer-owned memory must not be written to, and
  a parser that mutates its input would corrupt the corpus entry underneath
  libFuzzer.
* Bound the input length to something the subsystem would really see. Letting
  the fuzzer explore far beyond a realistic MTU spends the budget on length
  handling rather than on parser structure.

CI
**

Twister builds this target but does not run it: a libFuzzer binary with no time
bound never returns. Running it belongs in a job with its own time budget.

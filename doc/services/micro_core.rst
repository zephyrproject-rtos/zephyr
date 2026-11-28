.. SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
.. SPDX-License-Identifier: Apache-2.0

.. _micro_core:

Micro Core communication
########################

Micro Core transfers one payload per frame over application-provided frame callbacks.
:kconfig:option:`CONFIG_MC_COMM` adds CRC-16 checking, sequence numbers, acknowledgments,
duplicate suppression, and bounded retransmission. The application supplies all persistent
instance storage; the subsystem does not allocate from a heap.

The callbacks exchange complete frames. A UART is a byte stream, so a UART adapter must provide
frame boundaries and recover after damaged or missing bytes. The
:zephyr:code-sample:`micro-core-uart` sample uses SLIP byte escaping and delimiters for
this purpose. CRC-16 detects damage within a received frame; it does not authenticate a peer.

Memory and limits
*****************

Call :c:func:`mc_comm_req_size` with a completed configuration and provide at least that many
bytes of eight-byte-aligned storage to :c:func:`mc_comm_init`. Keep the storage and transport
context valid for the instance's lifetime. The send and receive frame sizes include a 12-byte
header and a four-byte payload-length prefix. A payload must fit in one frame. Window capacities
may be zero or one; both select one outstanding frame. The receive capacity must fit inside the
configured receive frame size.

The following shows the storage setup; the callback functions must be supplied by the
application::

   static uint8_t storage[512] __aligned(8);
   struct mc_comm *comm;
   struct mc_comm_cfg config = {
       .io.receive = read_frame,
       .io.transmit = write_frame,
       .io.context = transport_context,
       .recv.size = 64U,
       .send.size = 64U,
       .receive_capacity = 32U,
   };

   size_t required = mc_comm_req_size(&config);
   if (required == 0U || required > sizeof(storage)) {
       /* Increase storage or revise the configuration. */
   } else {
       int err = mc_comm_init(&comm, storage, sizeof(storage), &config);
       /* Check err before using comm. */
   }

Delivery
********

:c:func:`mc_comm_send` waits for an acknowledgment, retrying within its retry count and
timeout. If the call times out or a transport error occurs, the peer may have accepted the
payload. Retry the same payload; a different payload returns ``-EBUSY`` until the pending
payload is acknowledged. The receiver acknowledges duplicates without delivering them again.
An application must serialize calls that share an instance. The wire format is specific to this
Zephyr implementation and does not interoperate with the original Linux ``micro_core``.

Verification
************

The ``mc.comm`` Twister suite injects frame loss and corruption and checks retries, malformed
frames, duplicate suppression, and storage bounds. Run it in a west workspace::

   export ZEPHYR_BASE="$PWD"
   west twister -p native_sim/native/64 -s mc.comm -i

The UART sample has build-only Twister metadata because it needs two wired
``nrf54l15dk/nrf54l15/cpuapp`` boards to execute.

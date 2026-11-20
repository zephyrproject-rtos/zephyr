.. SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
.. SPDX-License-Identifier: Apache-2.0

GATT subscriber fanout
#####################

Run with BabbleSim installed and ``BSIM_OUT_PATH`` and ``BSIM_COMPONENTS_PATH`` set::

   west twister -p nrf52_bsim/native \
     -T tests/bsim/bluetooth/host/gatt/notify_fanout --fixture bsim_multi_test

Each configuration uses a server and two subscribers. The scripts test both orders of a small
and a large peer, as well as two small peers and two large peers. Connections and subscriptions
are established sequentially to make the subscriber iteration order deterministic.

The default configuration exchanges the UATT MTU only for large peers. It checks 100-byte and
10-byte values, notification entry points, indication confirmations and parameter destruction,
empty broadcasts, and recovery after exhausting the real ATT buffer and request pools.

The EATT configuration keeps the UATT MTU at 23 for every peer and explicitly establishes one
enhanced bearer for each large peer. A 100-byte value therefore requires EATT. Notifications
use completion callbacks before sending phase markers, so markers on another bearer cannot
overtake an outstanding value. Indications wait for confirmation and destruction.

The audio configuration registers real AICS and VOCS server instances. Two encrypted clients
subscribe to their descriptions. Each service sends a 100-byte description and then a 10-byte
description while the test holds all ATT buffers. A linker wrapper observes the return value
of the real ``bt_gatt_notify_cb()``; it does not replace the stack or inject return values.
The test checks that partial success does not schedule another broadcast, that all-MTU failures
retain the existing ``-ENOMEM`` retry behavior, and that buffer exhaustion retries until release.
The clients check exact payloads and reject duplicates. The observation window after each update
is 350 ms, more than ten default audio retry intervals. Short descriptions stop the persistent
retries in the all-small case before moving to the next service.

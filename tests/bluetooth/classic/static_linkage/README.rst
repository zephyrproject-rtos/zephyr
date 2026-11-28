.. SPDX-FileCopyrightText: Copyright 2026 NXP
.. SPDX-License-Identifier: Apache-2.0

Classic file-local callback regression tests
###########################################

This ZTEST suite exercises AVDTP signaling and media callbacks through the L2CAP
channel operations installed by the normal AVDTP interfaces. The production
sources are compiled separately; the tests do not include implementation files
or expose file-local functions.

Three intentionally global test guards use the signaling callbacks' names.
Removing ``static`` from a production callback causes a duplicate-symbol link
failure. The runtime cases also fail if a channel operation invokes a test guard
instead of the real callback.

Only L2CAP server registration and outgoing channel connection are mocked.
Source-specific compile definitions rename their real implementations to
``real_bt_l2cap_br_server_register`` and ``real_bt_l2cap_chan_connect``.
The tests define the original API names, without linker ``--wrap`` or a dependency
on inter-object reference rewriting. AVDTP callbacks remain unmodified, including
with LTO enabled. No controller, peer, or hardware connection is required.

Cases cover connection notifications, pending-request completion on disconnect,
reassembly cleanup, accepted and unexpected signaling responses, truncated
headers, media notifications and data forwarding, incoming connection rejection,
and AT CME error parsing.

Twister runs the suite with default, full-feature/debug, and minimal-feature
configurations. All configurations enable the changed Classic components and
shell sources and treat compiler warnings as errors. The runtime assertions
verify callback behavior; they do not inspect ELF symbol visibility.

.. code-block:: console

   west twister -p qemu_x86 -T tests/bluetooth/classic/static_linkage --short-build-path -i

In a Git worktree, set ``ZEPHYR_BASE`` to that worktree before running Twister.
The short build paths avoid Windows toolchain path-length limits.

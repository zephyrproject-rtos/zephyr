.. SPDX-License-Identifier: Apache-2.0

LTK request hook
================

Purpose
-------
This test demonstrates and verifies the experimental application hook for
handling missing LTKs.

Details
-------
Two simulated devices (a central and a peripheral) establish an LE
connection. The central initiates encryption using a pre-shared LTK. On
the peripheral, when the Controller raises
``HCI_LE_Long_Term_Key_Request``, the SMP code path does not have an LTK
for the peer, so the application hook ``bt_hook_conn_ltk_request`` is
invoked to give the application a chance to provide the LTK. The
application hook compares the connection to a pre-selected connection
and returns ``true`` if it matches. When it returns ``true``, the Host
does not immediately send a negative reply. Instead, the application
must send an LTK reply or a negative reply. The link then becomes
encrypted at security level 2 (encrypted but not authenticated).

The test then performs one read requiring encryption and one read
requiring authentication. The read requiring encryption succeeds with
``BT_ATT_ERR_SUCCESS``. The read requiring authentication fails with
``BT_ATT_ERR_AUTHENTICATION``.

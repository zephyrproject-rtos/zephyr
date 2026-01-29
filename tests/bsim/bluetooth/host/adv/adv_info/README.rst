.. SPDX-License-Identifier: Apache-2.0

Advertising info retrieval test
===============================

Test Purpose
------------

Verify that ``bt_le_adv_get_info`` API correctly retrieves BLE advertising
information including identity, address type, and address format based on
privacy settings.

Test Procedure
--------------

1. Call ``bt_enable`` and assert it succeeds.
2. Call ``bt_le_adv_start`` to start connectable advertising using a simple
   advertising data payload and assert advertising starts successfully.
3. Call ``bt_le_adv_get_info`` to fetch the active advertising info into a
   ``bt_le_adv_info`` struct and assert the call succeeds.
4. Verify identity handle is default (BT_ID_DEFAULT).
5. Verify address type is LE Random and format: if CONFIG_BT_PRIVACY is
   enabled, assert the address is an RPA; otherwise assert it is a static
   random address.
6. If CONFIG_BT_PRIVACY is enabled, wait for the RPA timeout to elapse and
   verify the advertising address rotates.
7. Stop advertising.
8. Fetch the default identity address with ``bt_id_get``.
8. Start identity advertising (``BT_LE_ADV_NCONN_IDENTITY``) and verify
   ``bt_le_adv_get_info`` returns the identity address.

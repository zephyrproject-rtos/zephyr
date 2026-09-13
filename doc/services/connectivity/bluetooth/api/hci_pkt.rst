.. _bt_hci_pkt:

HCI Packet Helpers
##################

Helpers for framing HCI command packets and parsing command responses in
:c:struct:`net_buf_simple` buffers and packet bytes, independent of the Bluetooth
Host and of the HCI driver interface, together with a lockstep helper for HCI
drivers that exchange HCI commands with the controller over their own transport,
for example for vendor-specific controller initialization.

The packet helpers are part of every build with :kconfig:option:`CONFIG_BT`
enabled. The lockstep helper is enabled with
:kconfig:option:`CONFIG_BT_HCI_LOCKSTEP`, which the HCI drivers that use it
select in their Kconfig.

These are not general application APIs: the intended users are HCI drivers and
Bluetooth stack internals. Applications that need to send HCI commands
alongside a running Host use the higher-level :c:func:`bt_hci_cmd_alloc`,
:c:func:`bt_hci_cmd_send` and :c:func:`bt_hci_cmd_send_sync` APIs instead,
which cooperate with the Host's command flow control.

API Reference
*************

.. doxygengroup:: bt_hci_pkt

.. doxygengroup:: bt_hci_lockstep

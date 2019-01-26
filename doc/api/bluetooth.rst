.. _bluetooth_api:

Bluetooth API
#############

.. contents::
   :depth: 2
   :local:
   :backlinks: top

This is the full set of available Bluetooth APIs. It's important to note
that the set that will in practice be available for the application
depends on the exact Kconfig options that have been chosen, since most
of the Bluetooth functionality is build-time selectable. E.g. any
connection-related APIs require :option:`CONFIG_BT_CONN` and any
BR/EDR (Bluetooth Classic) APIs require :option:`CONFIG_BT_BREDR`.

.. comment
   not documenting
   .. doxygengroup:: bluetooth
   .. doxygengroup:: bt_test_cb



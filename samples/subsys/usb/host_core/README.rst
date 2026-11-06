.. zephyr:code-sample:: usb-host-core
   :name: USB host core descriptor helpers
   :relevant-api: usb_host_core_api

   Validate bounded configuration-descriptor walks on ``native_sim``.

Overview
********

Uses :c:func:`usbh_desc_cfg_end`, :c:func:`usbh_desc_header_in_bounds`, and
:c:func:`usbh_desc_get_next_function` on a synthetic descriptor block. Deeper
host class and hub behavior is covered by ``tests/subsys/usb/host``.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/host_core
   :board: native_sim
   :goals: build run
   :compact:

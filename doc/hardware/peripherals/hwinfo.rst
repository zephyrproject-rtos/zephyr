.. _hwinfo_api:

Hardware Information
####################

Overview
********

The HW Info API provides access to hardware information such as device
identifiers and reset cause flags.

Reset cause flags can be used to determine why the device was reset; for
example due to a watchdog timeout or due to power cycling. Different devices
support different subset of flags. Use
:c:func:`hwinfo_get_supported_reset_cause` to retrieve the flags that are
supported by that device.

Configuration Options
*********************

Related configuration options:

* :kconfig:option:`CONFIG_HWINFO`
* :kconfig:option:`CONFIG_HWINFO_SHELL`

Shell commands
**************

.. zephyr:shell-module:: hwinfo
   :kconfig: CONFIG_HWINFO_SHELL
   :depends: CONFIG_HWINFO

   The ``hwinfo`` command prints the unique identifier of the device and the cause of its last
   reset, which is a quick way to check what the hardware information API reports for a board.

.. rubric:: Command reference

.. zephyr:shell-command-reference::

API Reference
*************

.. doxygengroup:: hwinfo_interface

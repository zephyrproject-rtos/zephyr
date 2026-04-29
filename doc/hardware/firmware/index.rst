Firmware
########

Overview
********

This class of drivers provides support for interacting with various service-providing
(e.g. clock management, power management, pinctrl, etc.) firmware. The list of firmware
currently supported by Zephyr is summarized below:

.. list-table::
   :align: center

   * - Name
     - Vendor
     - Description
     - Documentation

   * - SCMI
     - ARM
     - System Control and Management Interface
     - `Link <https://developer.arm.com/documentation/den0056/latest/>`__

   * - TISCI
     - TI
     - TI System Controller Interface
     - `Link <https://software-dl.ti.com/tisci/esd/latest/index.html>`__

   * - QEMU FWCFG
     - QEMU
     - QEMU Firmware Configuration
     - `Link <https://www.qemu.org/docs/master/specs/fw_cfg.html>`__

API Reference
*************

Usually, the firmware drivers are meant to provide functionality to other classes
of drivers. Furthermore, the purpose and functionality may differ from one firmware
to another. Consequently, this class of drivers doesn't implement a standard API
meant to be used by the end user.

Samples
*******

See :zephyr:code-sample-category:`firmware`.

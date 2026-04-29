Firmware
########

Overview
********

This class of drivers provides support for interacting with various service-providing
(e.g. clock management, power management, pinctrl, etc.) firmware. The list of firmware
currently supported by Zephyr is summarized below:

.. list-table:: List of supported firmware
   :align: center

   * - Name
     - Vendor
     - Description
     - Specification
     - Zephyr documentation

   * - SCMI
     - ARM
     - System Control and Management Interface
     - `Link <https://developer.arm.com/documentation/den0056/latest/>`__
     - N/A

   * - TISCI
     - TI
     - TI System Controller Interface
     - `Link <https://software-dl.ti.com/tisci/esd/latest/index.html>`__
     - N/A

   * - QEMU FWCFG
     - QEMU
     - QEMU Firmware Configuration
     - `Link <https://www.qemu.org/docs/master/specs/fw_cfg.html>`__
     - N/A

API Reference
*************

Usually, the firmware drivers are meant to provide functionality to other classes
of drivers. Furthermore, the purpose and functionality may differ from one firmware
to another. Consequently, this class of drivers doesn't implement a standard API
meant to be used by the end user.

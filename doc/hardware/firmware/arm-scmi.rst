.. _arm_scmi:

ARM System Control and Management Interface
###########################################

Overview
********

System Control and Management Interface (SCMI) is a specification developed by
ARM, which describes a set of OS-agnostic software interfaces used to perform
system management (e.g: clock control, pinctrl, etc...). In this context, Zephyr
acts as an SCMI agent.

.. note::

   The Zephyr implementation may only include a subset of the features
   or functionalities mentioned throughout this document.

Standard protocols
******************

The set of supported **standard** [#]_ protocols is summarized below:

.. list-table::
   :align: center

   * - ID
     - Name
     - Supported version

   * - 0x10
     - Base protocol
     - 2.1

   * - 0x11
     - Power domain management protocol
     - 3.1

   * - 0x12
     - System power management protocol
     - 2.1

   * - 0x14
     - Clock management protocol
     - 3.0

   * - 0x19
     - Pin Control protocol
     - 1.0

Transports
**********

The set of supported transports is summarized below:

.. list-table::
   :align: center

   * - Name
     - Description
     - Compatible

   * - MBOX
     - Shared memory with mailbox-based doorbell
     - :dtcompatible:`arm,scmi`

   * - SMC
     - Shared memory with SMC-based doorbell
     - :dtcompatible:`arm,scmi-smc`

Vendor extensions
*****************

The SCMI specification allows vendors to introduce additional protocols and
provides a degree of freedom with respect to how the platform firmware can
behave in certain situations. In the context of this documentation, these
are collectively known as **vendor extensions**.

NXP
===

NXP provides an SCMI-compliant platform firmware known as **System Manager (SM)**.
You may find its documentation `here <https://github.com/nxp-imx/imx-sm>`__.

The set of supported NXP-specific protocols is summarized below:

.. list-table::
   :align: center

   * - ID
     - Name
     - Supported version

   * - 0x82
     - CPU
     - 1.0

The set of supported NXP-specific quirks is summarized below:

#. Depending on how the platform firmware is configured, the shared memory
   may contain a message CRC field that's checked by the firmware on each
   received message. See :kconfig:option:`CONFIG_ARM_SCMI_NXP_VENDOR_EXTENSIONS`.

.. rubric:: Footnotes

.. [#] Meaning protocols covered by the SCMI specification.

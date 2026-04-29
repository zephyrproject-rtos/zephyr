:orphan:

.. _arm_scmi:

ARM System Control and Management Interface
###########################################

Overview
********

System Control and Management Interface (SCMI) is a specification developed by
ARM, which describes a set of OS-agnostic software interfaces used to perform
system management (e.g: clock control, pinctrl, etc...). In this context, Zephyr
acts as an SCMI agent.

Supported protocols
*******************

The set of supported **standard** [#]_ protocols is summarized below:

.. list-table::
   :align: center

   * - ID
     - Name
     - Supported version

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

.. note::

   The Zephyr implementation may only include a subset of the protocol commands.

Supported transports
********************

The set of supported transports is summarized below:

.. list-table::
   :align: center

   * - Name
     - Compatible

   * - Shared memory with MBOX-based doorbell
     - :dtcompatible:`arm,scmi`

   * - Shared memory with SMC-based doorbell
     - :dtcompatible:`arm,scmi-smc`

Vendor extensions
*****************

The SCMI specification allows vendors to introduce additional protocols and
provides a degree of freedom with respect to how certain structure fields are
used. In the context of this documentation, these are collectively known as
**vendor extensions**. Below you may find a list of all vendor extensions
currently supported in Zephyr.

NXP
===

**Overview**

NXP provides an SCMI-compliant platform firmware known as **System Manager (SM)**.

**Specification**

You may find the specification for the SM firmware `here <https://github.com/nxp-imx/imx-sm>`__.

**Supported protocols**

The set of supported NXP-specific protocols is summarized below:

.. list-table::
   :align: center

   * - ID
     - Name
     - Supported version

   * - 0x82
     - CPU
     - 1.0

**Quirks**

All NXP-related quirks are controlled via :kconfig:option:`CONFIG_ARM_SCMI_NXP_VENDOR_EXTENSIONS`.

#. Depending on the platform firmware configuration, the shared memory area may
   also include a CRC of the message.

.. [#] Meaning protocols covered by the SCMI specification. Also known as generic
       protocols.

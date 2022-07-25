.. _devicetree_api:

Devicetree API
##############

This is a reference page for the ``<devicetree.h>`` API. The API is macro
based. Use of these macros has no impact on scheduling. They can be used from
any calling context and at file scope.

Some of these require a special macro named ``DT_DRV_COMPAT`` to be defined
before they can be used; these are discussed individually below. These macros
are generally meant for use within :ref:`device drivers <device_model_api>`,
though they can be used outside of drivers with appropriate care.

.. _devicetree-generic-apis:

Generic APIs
************

The APIs in this section can be used anywhere and do not require
``DT_DRV_COMPAT`` to be defined.

Node identifiers and helpers
============================

A *node identifier* is a way to refer to a devicetree node at C preprocessor
time. While node identifiers are not C values, you can use them to access
devicetree data in C rvalue form using, for example, the
:ref:`devicetree-property-access` API.

The root node ``/`` has node identifier ``DT_ROOT``. You can create node
identifiers for other devicetree nodes using :c:func:`DT_PATH`,
:c:func:`DT_NODELABEL`, :c:func:`DT_ALIAS`, and :c:func:`DT_INST`.

There are also :c:func:`DT_PARENT` and :c:func:`DT_CHILD` macros which can be
used to create node identifiers for a given node's parent node or a particular
child node, respectively.

The following macros create or operate on node identifiers.

.. doxygengroup:: devicetree-generic-id

.. _devicetree-property-access:

Property access
===============

The following general-purpose macros can be used to access node properties.
There are special-purpose APIs for accessing the :ref:`devicetree-ranges-property`,
:ref:`devicetree-reg-property` and :ref:`devicetree-interrupts-property`.

Property values can be read using these macros even if the node is disabled,
as long as it has a matching binding.

.. doxygengroup:: devicetree-generic-prop

.. _devicetree-ranges-property:

``ranges`` property
===================

Use these APIs instead of :ref:`devicetree-property-access` to access the
``ranges`` property. Because this property's semantics are defined by the
devicetree specification, these macros can be used even for nodes without
matching bindings. However, they take on special semantics when the node's
binding indicates it is a PCIe bus node, as defined in the
`PCI Bus Binding to: IEEE Std 1275-1994 Standard for Boot (Initialization Configuration) Firmware`_

.. _PCI Bus Binding to\: IEEE Std 1275-1994 Standard for Boot (Initialization Configuration) Firmware:
    https://www.openfirmware.info/data/docs/bus.pci.pdf

.. doxygengroup:: devicetree-ranges-prop

.. _devicetree-reg-property:

``reg`` property
================

Use these APIs instead of :ref:`devicetree-property-access` to access the
``reg`` property. Because this property's semantics are defined by the
devicetree specification, these macros can be used even for nodes without
matching bindings.

.. doxygengroup:: devicetree-reg-prop

.. _devicetree-interrupts-property:

``interrupts`` property
=======================

Use these APIs instead of :ref:`devicetree-property-access` to access the
``interrupts`` property.

Because this property's semantics are defined by the devicetree specification,
some of these macros can be used even for nodes without matching bindings. This
does not apply to macros which take cell names as arguments.

.. doxygengroup:: devicetree-interrupts-prop

For-each macros
===============

There is currently only one "generic" for-each macro,
:c:func:`DT_FOREACH_CHILD`, which allows iterating over the children of a
devicetree node.

There are special-purpose for-each macros, like
:c:func:`DT_INST_FOREACH_STATUS_OKAY`, but these require ``DT_DRV_COMPAT`` to
be defined before use.

.. doxygengroup:: devicetree-generic-foreach

Existence checks
================

This section documents miscellaneous macros that can be used to test if a node
exists, how many nodes of a certain type exist, whether a node has certain
properties, etc. Some macros used for special purposes (such as
:c:func:`DT_IRQ_HAS_IDX` and all macros which require ``DT_DRV_COMPAT``) are
documented elsewhere on this page.

.. doxygengroup:: devicetree-generic-exist

.. _devicetree-dep-ord:

Inter-node dependencies
=======================

The ``devicetree.h`` API has some support for tracking dependencies between
nodes. Dependency tracking relies on a binary "depends on" relation between
devicetree nodes, which is defined as the `transitive closure
<https://en.wikipedia.org/wiki/Transitive_closure>`_ of the following "directly
depends on" relation:

- every non-root node directly depends on its parent node
- a node directly depends on any nodes its properties refer to by phandle
- a node directly depends on its ``interrupt-parent`` if it has an
  ``interrupts`` property

A *dependency ordering* of a devicetree is a list of its nodes, where each node
``n`` appears earlier in the list than any nodes that depend on ``n``. A node's
*dependency ordinal* is then its zero-based index in that list. Thus, for two
distinct devicetree nodes ``n1`` and ``n2`` with dependency ordinals ``d1`` and
``d2``, we have:

- ``d1 != d2``
- if ``n1`` depends on ``n2``, then ``d1 > d2``
- ``d1 > d2`` does **not** necessarily imply that ``n1`` depends on ``n2``

The Zephyr build system chooses a dependency ordering of the final devicetree
and assigns a dependency ordinal to each node. Dependency related information
can be accessed using the following macros. The exact dependency ordering
chosen is an implementation detail, but cyclic dependencies are detected and
cause errors, so it's safe to assume there are none when using these macros.

There are instance number-based conveniences as well; see
:c:func:`DT_INST_DEP_ORD` and subsequent documentation.

.. doxygengroup:: devicetree-dep-ord

Bus helpers
===========

Zephyr's devicetree bindings language supports a ``bus:`` key which allows
bindings to declare that nodes with a given compatible describe system buses.
In this case, child nodes are considered to be on a bus of the given type, and
the following APIs may be used.

.. doxygengroup:: devicetree-generic-bus

.. _devicetree-inst-apis:

Instance-based APIs
*******************

These are recommended for use within device drivers. To use them, define
``DT_DRV_COMPAT`` to the lowercase-and-underscores compatible the device driver
implements support for. Here is an example devicetree fragment:

.. code-block:: devicetree

   serial@40001000 {
           compatible = "vnd,serial";
           status = "okay";
           current-speed = <115200>;
   };

Example usage, assuming serial@40001000 is the only enabled node
with compatible "vnd,serial":

.. code-block:: c

   #define DT_DRV_COMPAT vnd_serial
   DT_DRV_INST(0)                  // node identifier for serial@40001000
   DT_INST_PROP(0, current_speed)  // 115200

.. warning::

   Be careful making assumptions about instance numbers. See :c:func:`DT_INST`
   for the API guarantees.

As shown above, the ``DT_INST_*`` APIs are conveniences for addressing nodes by
instance number. They are almost all defined in terms of one of the
:ref:`devicetree-generic-apis`. The equivalent generic API can be found by
removing ``INST_`` from the macro name. For example, ``DT_INST_PROP(inst,
prop)`` is equivalent to ``DT_PROP(DT_DRV_INST(inst), prop)``. Similarly,
``DT_INST_REG_ADDR(inst)`` is equivalent to ``DT_REG_ADDR(DT_DRV_INST(inst))``,
and so on. There are some exceptions: :c:func:`DT_ANY_INST_ON_BUS_STATUS_OKAY`
and :c:func:`DT_INST_FOREACH_STATUS_OKAY` are special-purpose helpers without
straightforward generic equivalents.

Since ``DT_DRV_INST()`` requires ``DT_DRV_COMPAT`` to be defined, it's an error
to use any of these without that macro defined.

Note that there are also helpers available for
specific hardware; these are documented in :ref:`devicetree-hw-api`.

.. doxygengroup:: devicetree-inst

.. _devicetree-hw-api:

Hardware specific APIs
**********************

The following APIs can also be used by including ``<devicetree.h>``;
no additional include is needed.

.. _devicetree-can-api:

CAN
===

These conveniences may be used for nodes which describe CAN
controllers/transceivers, and properties related to them.

.. doxygengroup:: devicetree-can

Clocks
======

These conveniences may be used for nodes which describe clock sources, and
properties related to them.

.. doxygengroup:: devicetree-clocks

DMA
===

These conveniences may be used for nodes which describe direct memory access
controllers or channels, and properties related to them.

.. doxygengroup:: devicetree-dmas

.. _devicetree-flash-api:

Fixed flash partitions
======================

These conveniences may be used for the special-purpose ``fixed-partitions``
compatible used to encode information about flash memory partitions in the
device tree. See See :dtcompatible:`fixed-partition` for more details.

.. doxygengroup:: devicetree-fixed-partition

.. _devicetree-gpio-api:

GPIO
====

These conveniences may be used for nodes which describe GPIO controllers/pins,
and properties related to them.

.. doxygengroup:: devicetree-gpio

IO channels
===========

These are commonly used by device drivers which need to use IO
channels (e.g. ADC or DAC channels) for conversion.

.. doxygengroup:: devicetree-io-channels

.. _devicetree-mbox-api:

MBOX
====

These conveniences may be used for nodes which describe MBOX controllers/users,
and properties related to them.

.. doxygengroup:: devicetree-mbox

.. _devicetree-pinctrl-api:

Pinctrl (pin control)
=====================

These are used to access pin control properties by name or index.

Devicetree nodes may have properties which specify pin control (sometimes known
as pin mux) settings. These are expressed using ``pinctrl-<index>`` properties
within the node, where the ``<index>`` values are contiguous integers starting
from 0. These may also be named using the ``pinctrl-names`` property.

Here is an example:

.. code-block:: DTS

   node {
       ...
       pinctrl-0 = <&foo &bar ...>;
       pinctrl-1 = <&baz ...>;
       pinctrl-names = "default", "sleep";
   };

Above, ``pinctrl-0`` has name ``"default"``, and ``pinctrl-1`` has name
``"sleep"``. The ``pinctrl-<index>`` property values contain phandles. The
``&foo``, ``&bar``, etc. phandles within the properties point to nodes whose
contents vary by platform, and which describe a pin configuration for the node.

.. doxygengroup:: devicetree-pinctrl

PWM
===

These conveniences may be used for nodes which describe PWM controllers and
properties related to them.

.. doxygengroup:: devicetree-pwms

Reset Controller
================

These conveniences may be used for nodes which describe reset controllers and
properties related to them.

.. doxygengroup:: devicetree-reset-controller

SPI
===

These conveniences may be used for nodes which describe either SPI controllers
or devices, depending on the case.

.. doxygengroup:: devicetree-spi

.. _devicetree-chosen-nodes:

Chosen nodes
************

The special ``/chosen`` node contains properties whose values describe
system-wide settings. The :c:func:`DT_CHOSEN()` macro can be used to get a node
identifier for a chosen node.

.. doxygengroup:: devicetree-generic-chosen
   :project: Zephyr

There are also conveniences for commonly used zephyr-specific properties of the
``/chosen`` node.

.. doxygengroup:: devicetree-zephyr
   :project: Zephyr

Zephyr-specific chosen nodes
****************************

The following table documents some commonly used Zephyr-specific chosen nodes.

Sometimes, a chosen node's label property will be used to set the default value
of a Kconfig option which in turn configures a hardware-specific device. This
is usually for backwards compatibility in cases when the Kconfig option
predates devicetree support in Zephyr. In other cases, there is no Kconfig
option, and the devicetree node is used directly in the source code to select a
device.

.. Documentation maintainers: please keep this sorted by property name

.. list-table:: Zephyr-specific chosen properties
   :header-rows: 1
   :widths: 25 75

   * - Property
     - Purpose
   * - zephyr,bt-c2h-uart
     - Selects the UART used for host communication in the
       :ref:`bluetooth-hci-uart-sample`
   * - zephyr,bt-mon-uart
     - Sets UART device used for the Bluetooth monitor logging
   * - zephyr,bt-uart
     - Sets UART device used by Bluetooth
   * - zephyr,canbus
     - Sets the default CAN controller
   * - zephyr,ccm
     - Core-Coupled Memory node on some STM32 SoCs
   * - zephyr,code-partition
     - Flash partition that the Zephyr image's text section should be linked
       into
   * - zephyr,console
     - Sets UART device used by console driver
   * - zephyr,display
     - Sets the default display controller
   * - zephyr,dtcm
     - Data Tightly Coupled Memory node on some Arm SoCs
   * - zephyr,entropy
     - A device which can be used as a system-wide entropy source
   * - zephyr,flash
     - A node whose ``reg`` is sometimes used to set the defaults for
       :kconfig:option:`CONFIG_FLASH_BASE_ADDRESS` and :kconfig:option:`CONFIG_FLASH_SIZE`
   * - zephyr,flash-controller
     - The node corresponding to the flash controller device for
       the ``zephyr,flash`` node
   * - zephyr,gdbstub-uart
     - Sets UART device used by the :ref:`gdbstub` subsystem
   * - zephyr,ipc
     - Used by the OpenAMP subsystem to specify the inter-process communication
       (IPC) device
   * - zephyr,ipc_shm
     - A node whose ``reg`` is used by the OpenAMP subsystem to determine the
       base address and size of the shared memory (SHM) usable for
       interprocess-communication (IPC)
   * - zephyr,itcm
     - Instruction Tightly Coupled Memory node on some Arm SoCs
   * - zephyr,ocm
     - On-chip memory node on Xilinx Zynq-7000 and ZynqMP SoCs
   * - zephyr,osdp-uart
     - Sets UART device used by OSDP subsystem
   * - zephyr,ot-uart
     - Used by the OpenThread to specify UART device for Spinel protocol
   * - zephyr,pcie-controller
     - The node corresponding to the PCIe Controller
   * - zephyr,ppp-uart
     - Sets UART device used by PPP
   * - zephyr,settings-partition
     - Fixed partition node. If defined this selects the partition used
       by the NVS and FCB settings backends.
   * - zephyr,shell-uart
     - Sets UART device used by serial shell backend
   * - zephyr,sram
     - A node whose ``reg`` sets the base address and size of SRAM memory
       available to the Zephyr image, used during linking
   * - zephyr,tracing-uart
     - Sets UART device used by tracing subsystem
   * - zephyr,uart-mcumgr
     - UART used for :ref:`device_mgmt`
   * - zephyr,uart-pipe
     - Sets UART device used by serial pipe driver
   * - zephyr,usb-device
     - USB device node. If defined and has a ``vbus-gpios`` property, these
       will be used by the USB subsystem to enable/disable VBUS

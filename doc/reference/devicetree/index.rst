.. _devicetree_api:

Devicetree
##########

This page contains reference documentation for ``<devicetree.h>``. See
:ref:`dt-guide` for an introduction. Use of these macros has no impact on
scheduling. They can be used from any calling context and at file scope.

Some of these require a special macro named ``DT_DRV_COMPAT`` to be defined
before they can be used; these are discussed individually below. These macros
are generally meant for use within :ref:`device drivers <device_model_api>`,
though they can be used outside of drivers with appropriate care.

.. _devicetree-generic-apis:

Generic APIs
************

The APIs in this section can be used anywhere and do not require
``DT_DRV_COMPAT`` to be defined.

Node identifiers
================

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

.. doxygengroup:: devicetree-generic-id
   :project: Zephyr

.. _devicetree-property-access:

Property access
===============

The following general-purpose macros can be used to access node properties.
There are special-purpose APIs for accessing the :ref:`devicetree-reg-property`
and :ref:`devicetree-interrupts-property`.

Property values can be read using these macros even if the node is disabled,
as long as it has a matching binding.

.. doxygengroup:: devicetree-generic-prop
   :project: Zephyr

.. _devicetree-reg-property:

``reg`` property
================

Use these APIs instead of :ref:`devicetree-property-access` to access the
``reg`` property. Because this property's semantics are defined by the
devicetree specification, these macros can be used even for nodes without
matching bindings.

.. doxygengroup:: devicetree-reg-prop
   :project: Zephyr

.. _devicetree-interrupts-property:

``interrupts`` property
=======================

Use these APIs instead of :ref:`devicetree-property-access` to access the
``interrupts`` property.

Because this property's semantics are defined by the devicetree specification,
some of these macros can be used even for nodes without matching bindings. This
does not apply to macros which take cell names as arguments.

.. doxygengroup:: devicetree-interrupts-prop
   :project: Zephyr

For-each macros
===============

There is currently only one "generic" for-each macro,
:c:func:`DT_FOREACH_CHILD`, which allows iterating over the children of a
devicetree node.

There are special-purpose for-each macros, like
:c:func:`DT_INST_FOREACH_STATUS_OKAY`, but these require ``DT_DRV_COMPAT`` to
be defined before use.

.. doxygengroup:: devicetree-generic-foreach
   :project: Zephyr

Existence checks
================

This section documents miscellaneous macros that can be used to test if a node
exists, how many nodes of a certain type exist, whether a node has certain
properties, etc. Some macros used for special purposes (such as
:c:func:`DT_IRQ_HAS_IDX` and all macros which require ``DT_DRV_COMPAT``) are
documented elsewhere on this page.

.. doxygengroup:: devicetree-generic-exist
   :project: Zephyr

Bus helpers
===========

Zephyr's devicetree bindings language supports a ``bus:`` key which allows
bindings to declare that nodes with a given compatible describe system buses.
In this case, child nodes are considered to be on a bus of the given type, and
the following APIs may be used.

.. doxygengroup:: devicetree-generic-bus
   :project: Zephyr

.. _devicetree-inst-apis:

Instance-based APIs
*******************

These are recommended for use within device drivers. To use them, define
``DT_DRV_COMPAT`` to the lowercase-and-underscores compatible the device driver
implements support for. Here is an example devicetree fragment:

.. code-block:: DTS

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
   :project: Zephyr

.. _devicetree-hw-api:

Hardware specific APIs
**********************

The following APIs can also be used by including ``<devicetree.h>``;
no additional include is needed.

ADC
===

These are commonly used by sensor device drivers which need to use an ADC
channel for conversion.

.. doxygengroup:: devicetree-adc
   :project: Zephyr

Clocks
======

These conveniences may be used for nodes which describe clock sources, and
properties related to them.

.. doxygengroup:: devicetree-clocks
   :project: Zephyr

DMA
===

These conveniences may be used for nodes which describe direct memory access
controllers or channels, and properties related to them.

.. doxygengroup:: devicetree-dmas
   :project: Zephyr

.. _devicetree-flash-api:

Fixed flash partitions
======================

These conveniences may be used for the special-purpose ``fixed-partitions``
compatible used to encode information about flash memory partitions in the
device tree.

.. doxygengroup:: devicetree-fixed-partition
   :project: Zephyr

.. _devicetree-gpio-api:

GPIO
====

These conveniences may be used for nodes which describe GPIO controllers/pins,
and properties related to them.

.. doxygengroup:: devicetree-gpio
   :project: Zephyr

PWM
===

These conveniences may be used for nodes which describe PWM controllers and
properties related to them.

.. doxygengroup:: devicetree-pwms
   :project: Zephyr

SPI
===

These conveniences may be used for nodes which describe either SPI controllers
or devices, depending on the case.

.. doxygengroup:: devicetree-spi
   :project: Zephyr

.. _devicetree-chosen-nodes:

Chosen nodes
************

The special ``/chosen`` node contains properties whose values describe
system-wide settings. The :c:func:`DT_CHOSEN()` macro can be used to get a node
identifier for a chosen node.

.. doxygengroup:: devicetree-generic-chosen
   :project: Zephyr

There are also conveniences for commonly used zephyr-specific properties of the
``/chosen`` node. (These may also be set in :file:`dts_fixup.h` files for now,
though this mechanism is deprecated.)

.. doxygengroup:: devicetree-zephyr
   :project: Zephyr

The following table documents some commonly used Zephyr-specific chosen nodes.

Often, a chosen node's label property will be used to set the default value of
a Kconfig option which in turn configures a hardware-specific subsystem
setting. This is usually for backwards compatibility in cases when the Kconfig
option predates devicetree support in Zephyr. In other cases, there is no
Kconfig option, and the devicetree node's label property is used directly in
the source code to specify a device name.

.. Documentation maintainers: please keep this sorted by property name

.. list-table:: Zephyr-specific chosen properties
   :header-rows: 1

   * - Property
     - Purpose
   * - zephyr,bt-c2h-uart
     - Sets default :option:`CONFIG_BT_CTLR_TO_HOST_UART_DEV_NAME`
   * - zephyr,bt-mon-uart
     - Sets default :option:`CONFIG_BT_MONITOR_ON_DEV_NAME`
   * - zephyr,bt-uart
     - Sets default :option:`CONFIG_BT_UART_ON_DEV_NAME`
   * - zephyr,can-primary
     - Sets the primary CAN controller
   * - zephyr,ccm
     - Core-Coupled Memory node on some STM32 SoCs
   * - zephyr,code-partition
     - Flash partition that the Zephyr image's text section should be linked
       into
   * - zephyr,console
     - Sets default :option:`CONFIG_UART_CONSOLE_ON_DEV_NAME`
   * - zephyr,dtcm
     - Data Tightly Coupled Memory node on some Arm SoCs
   * - zephyr,entropy
     - A device which can be used as a system-wide entropy source
   * - zephyr,flash
     - A node whose ``reg`` is sometimes used to set the defaults for
       :option:`CONFIG_FLASH_BASE_ADDRESS` and :option:`CONFIG_FLASH_SIZE`
   * - zephyr,flash-controller
     - The node corresponding to the flash controller device for
       the ``zephyr,flash`` node
   * - zephyr,ipc
     - Used by the OpenAMP subsystem to specify the inter-process communication
       (IPC) device
   * - zephyr,ipc_shm
     - A node whose ``reg`` is used by the OpenAMP subsystem to determine the
       base address and size of the shared memory (SHM) usable for
       interprocess-communication (IPC)
   * - zephyr,shell-uart
     - Sets default :option:`CONFIG_UART_SHELL_ON_DEV_NAME`
   * - zephyr,sram
     - A node whose ``reg`` sets the base address and size of SRAM memory
       available to the Zephyr image, used during linking
   * - zephyr,uart-mcumgr
     - UART used for :ref:`device_mgmt`
   * - zephyr,uart-pipe
     - Sets default :option:`CONFIG_UART_PIPE_ON_DEV_NAME`
   * - zephyr,usb-device
     - USB device node. If defined and has a ``vbus-gpios`` property, these
       will be used by the USB subsystem to enable/disable VBUS

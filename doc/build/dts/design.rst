.. _dt-design:

Design goals
############

Zephyr's use of devicetree has evolved significantly over time, and further
changes are expected. The following are the general design goals, along with
specific examples about how they impact Zephyr's source code, and areas where
more work remains to be done.

Single source for hardware information
**************************************

Zephyr's built-in device drivers and sample applications shall obtain
configurable hardware descriptions from devicetree.

Examples
========

- New device drivers shall use devicetree APIs to determine which :ref:`devices
  to create <dt-create-devices>`.

- In-tree sample applications shall use :ref:`aliases <dt-alias-chosen>` to
  determine which of multiple possible generic devices of a given type will be
  used in the current build. For example, the :zephyr:code-sample:`blinky` sample uses this to
  determine the LED to blink.

- Boot-time pin muxing and pin control for new SoCs shall be accomplished via a
  devicetree-based pinctrl driver

Example remaining work
======================

- Zephyr's :ref:`twister_script` currently use :file:`board.yaml` files to
  determine the hardware supported by a board. This should be obtained from
  devicetree instead.

- Legacy device drivers currently use Kconfig to determine which instances of a
  particular compatible are enabled. This can and should be done with devicetree
  overlays instead.

- Board-level documentation still contains tables of hardware support which are
  generated and maintained by hand. This can and should be obtained from the
  board level devicetree instead.

- Runtime determination of ``struct device`` relationships should be done using
  information obtained from devicetree, e.g. for device power management.

.. _dt-source-compatibility:

Source compatibility with other operating systems
*************************************************

Zephyr's devicetree tooling is based on a generic layer which is interoperable
with other devicetree users, such as the Linux kernel.

Zephyr's binding language *semantics* can support Zephyr-specific attributes,
but shall not express Zephyr-specific relationships.

Examples
========

- Zephyr's devicetree source parser, :ref:`dtlib.py <dt-scripts>`, is
  source-compatible with other tools like `dtc`_ in both directions:
  :file:`dtlib.py` can parse ``dtc`` output, and ``dtc`` can parse
  :file:`dtlib.py` output.

- Zephyr's "extended dtlib" library, :file:`edtlib.py`, shall not include
  Zephyr-specific features. Its purpose is to provide a higher-level view of the
  devicetree for common elements like interrupts and buses.

  Only the high-level :file:`gen_defines.py` script, which is built on top of
  :file:`edtlib.py`, contains Zephyr-specific knowledge and features.

.. _dtc: https://git.kernel.org/pub/scm/utils/dtc/dtc.git/about/

Example remaining work
======================

- Zephyr has a custom :ref:`dt-bindings` language *syntax*. While Linux's
  dtschema does not yet meet Zephyr's needs, we should try to follow what it is
  capable of representing in Zephyr's own bindings.

- Due to inflexibility in the bindings language, Zephyr cannot support the full
  set of bindings supported by Linux.

- Devicetree source sharing between Zephyr and Linux is not done.

.. _dt-multi-api-hardware:

Modeling hardware exposed through multiple Zephyr APIs
******************************************************

When a hardware block can be used through different Zephyr APIs, devicetree
should keep hardware identity separate from the way the system uses each
instance.

Guidelines
==========

- Keep ``compatible`` focused on hardware identity. Avoid changing it only to
  route software API selection. A different ``compatible`` is appropriate when
  the hardware interfaces are architecturally distinct, for example because
  they use separate register blocks.

- Use ``/chosen`` for singleton system-wide selection. If the system must
  select exactly one instance for a global function, such as the system timer,
  console, or entropy source, represent that choice with a vendor-agnostic
  chosen property, even if other identical instances are used by other APIs.
  See also :ref:`devicetree-zephyr-chosen-nodes`.

- Use a property for per-instance software role assignment when the assignment
  must be expressed independently for each device instance and no singleton
  system-wide selector applies. In that case, keep the same ``compatible``
  and use a per-node property to describe that assignment. Use this only when
  the DT-visible binding schema stays the same for all instances.

- Place the assignment at the right integration level. If it is a
  silicon-level fixed attribute, describe it in ``soc.dtsi``. If it varies by
  board, product, or application, describe it in the board devicetree or an
  overlay.

If different uses of the hardware device require different required
properties, child nodes, or bus semantics, then they do not share one binding
schema. In that case, use different ``compatible`` values or model the device
as a multifunction one.

Examples
========

**Singleton selection with** ``/chosen``

The system timer is a singleton function. The ``zephyr,system-timer``
chosen property selects which hardware timer instance provides it, while
other identical instances remain available for other APIs.

For example, two identical LPTMR timer instances can share the same
``compatible`` in the hardware description:

.. code-block:: devicetree

   lptmr1: timer@44300000 {
       compatible = "nxp,lptmr";
       reg = <0x44300000 0x1000>;
       /* ... */
   };

   lptmr2: timer@424d0000 {
       compatible = "nxp,lptmr";
       reg = <0x424d0000 0x1000>;
       /* ... */
   };

An integration layer can then select which one provides the singleton system
timer function:

.. code-block:: devicetree

   / {
       chosen {
           zephyr,system-timer = &lptmr1;
       };
   };

The system-timer driver binds to the ``/chosen`` node. The counter
driver skips that instance and uses the remaining one.

**Per-instance software role with a property**

When no singleton system-wide selector applies, identical instances
can keep the same ``compatible`` and use a per-node property to
distinguish which software role each instance provides.

Consider a SoC with three identical timer instances, where different boards or
applications may assign timer and counter roles independently while keeping
the same binding schema for each instance:

.. code-block:: devicetree

   /* hardware description */
   timer0: timer@40000000 {
     compatible = "vendor,multitimer";
     reg = <0x40000000 0x1000>;
     /* ... */
   };

   timer1: timer@40001000 {
     compatible = "vendor,multitimer";
     reg = <0x40001000 0x1000>;
     /* ... */
   };

   timer2: timer@40002000 {
     compatible = "vendor,multitimer";
     reg = <0x40002000 0x1000>;
     /* ... */
   };

.. code-block:: devicetree

   /* integration-specific role assignment */
   &timer0 {
       zephyr,role = "counter";
   };

   &timer1 {
       zephyr,role = "timer";
   };

   &timer2 {
       zephyr,role = "counter";
   };

Each driver filters instances by the ``zephyr,role`` value it
handles. In this example, the node properties and child-node structure stay
the same regardless of the selected role.

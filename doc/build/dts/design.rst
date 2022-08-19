.. _dt-design:

Design goals
############

Zephyr's use of devicetree has evolved significantly over time, and further
changes are expected. The following are the general design goals, along with
specific examples about how they impact Zephyr's source code, and areas where
more work remains to be done.

Single source for all hardware information
******************************************

Zephyr shall obtain its hardware descriptions exclusively from devicetree.

Examples
========

- New device drivers shall use devicetree APIs to determine which :ref:`devices
  to create <dt-create-devices>` if possible.

- In-tree sample applications shall use :ref:`aliases <dt-alias-chosen>` to
  determine which of multiple possible generic devices of a given type will be
  used in the current build. For example, the :ref:`blinky-sample` uses this to
  determine the LED to blink.

- Boot-time pin muxing and pin control can be accomplished via devicetree.

Example remaining work
======================

- Zephyr's :ref:`twister_script` currently use :file:`board.yaml` files to
  determine the hardware supported by a board. This should be obtained from
  devicetree instead.

- Various device drivers currently use Kconfig to determine which instances of a
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
at a DTS syntax level with other devicetree users, such as the Linux kernel.

Zephyr's devicetree bindings should be compatible with Linux bindings as well
whenever that is practical. However, the needs of the two operating systems are
different, so Zephyr bindings can be incompatible with Linux provided there is
a strong enough justification to convince the devicetree maintainers.

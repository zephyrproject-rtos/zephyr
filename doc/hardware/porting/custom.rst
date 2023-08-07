.. _platform_cusomizations:

Hardware with Custom Interfaces
###############################

When the provided support in Zephyr does not cover certain hardware platforms,
some interfaces can be implemented directly by the platform or a custom
architecture interface can be used and resulting in more flexibility and
hardware support of differebt variants of the same hardware.

Below is a list of interfaces that can be implemented by the platform overriding
the default implementation available in Zephyr:

- Interrupt Controller
- System IO operations
- Busy Wait

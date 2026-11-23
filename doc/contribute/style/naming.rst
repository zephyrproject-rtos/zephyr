.. _naming_conventions:

Naming conventions
##################

This section describes the naming conventions adopted by the Zephyr
Project, for each individual programming language or tool used in it.

C Code naming conventions
*************************

The naming conventions in this section apply to C source and header files,
as stated in each individual sub-section.

Public symbol prefixes
======================

All :term:`public APIs <public API>` introduced to Zephyr must be prefixed according
to the area or subsystem they belong to. Examples of area or subsystem prefixes
are provided below for reference.

* ``k_`` for the kernel
* ``sys_`` for system-wide code and features
* ``net_`` for the networking subsystem
* ``bt_`` for the Bluetooth subsystem
* ``i2c_`` for the I2C controller subsystem

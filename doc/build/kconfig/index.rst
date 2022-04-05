.. _kconfig:

Configuration System (Kconfig)
*******************************

The Zephyr kernel and subsystems can be configured at build time to adapt them
for specific application and platform needs. Configuration is handled through
Kconfig, which is the same configuration system used by the Linux kernel. The
goal is to support configuration without having to change any source code.

Configuration options (often called *symbols*) are defined in :file:`Kconfig`
files, which also specify dependencies between symbols that determine what
configurations are valid. Symbols can be grouped into menus and sub-menus to
keep the interactive configuration interfaces organized.

The output from Kconfig is a header file :file:`autoconf.h` with macros that
can be tested at build time. Code for unused features can be compiled out to
save space.

The following sections explain how to set Kconfig configuration options, go
into detail on how Kconfig is used within the Zephyr project, and have some
tips and best practices for writing :file:`Kconfig` files.

.. toctree::
   :maxdepth: 1

   menuconfig.rst
   setting.rst
   tips.rst
   preprocessor-functions.rst
   extensions.rst

Users interested in optimizing their configuration for security should refer
to the Zephyr Security Guide's section on the :ref:`hardening`.

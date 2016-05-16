.. _kbuild:

Build System User Guide
#######################

The Zephyr Kernel's build system is based on the Kbuild system used in the
Linux kernel. This way the kernel embraces the recursive model used in Linux
and the configuration model implemented using Kconfig.

The build process is driven by applications, unlike the Linux Kbuild
system. Therefore, the build system requires an application to initiate building
the kernel source code. The build system compiles the kernel and the application
into a single image.

.. toctree::
   :maxdepth: 1

   kbuild_kconfig
   ../kconfig/index.rst
   kbuild_makefiles
   kbuild_project

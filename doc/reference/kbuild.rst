.. _kbuild:

Kbuild User Guide
#################

The |project| build system is based on the Kbuild system used in the
Linux kernel (version 3.19-rc7). This way the |codename| embraces
the recursive model proposed by Linux and the configuration model
proposed by Kconfig.

The main difference between the Linux Kbuild system and the |codename|
build system is that the build is project centered. The consequence
is the need to define a project or application. The project drives
the build of the operating system.
For that reason, the build structure follows the Kbuild
architecture but it requires a project directory. The project
directory hosts the project's definition and code. Kbuild build the
kernel around the project directory.

Scope
*****

This document provides the details of the Kbuild implementation. It is
intended for application developers wanting to use the |project| as a
development platform and for kernel developers looking to modify or
expand the kernel functionality.

.. toctree::
   :maxdepth: 2

   kbuild_kconfig
   kbuild_makefiles
   kbuild_project
   kbuild_targets
   kbuild_toolchains
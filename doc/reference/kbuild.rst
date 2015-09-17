.. _kbuild:

Kbuild User Guide
#################

The |codename|'s build system is based on the Kbuild system used in the
Linux kernel (version 3.19-rc7). As a result, the kernel embraces
the recursive model proposed by Linux and the configuration model
proposed by Kconfig.

The |codename|'s build system is project centered, which is the main difference with the Linux
Kbuild system. The consequence is the need to define a project or application. Depending on the
APIs included in the application, the build system will build the kernel. For that reason, the
build structure follows the Kbuild architecture but it requires an application directory. The
application directory hosts the project's definition and code.

Scope
*****

This document provides the details of the Kbuild implementation. It is
intended for application developers wanting to use the kernel as a
development platform and for kernel developers looking to modify or
expand the kernel functionality.

.. toctree::
   :maxdepth: 2

   kbuild_kconfig
   kbuild_makefiles
   kbuild_project
   kbuild_targets
   kbuild_toolchains
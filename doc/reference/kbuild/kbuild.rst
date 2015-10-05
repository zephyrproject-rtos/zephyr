.. _kbuild:

The Build System User Guide
###########################

The |codename|'s build system is based on the Kbuild system used in the
Linux kernel, version 3.19-rc7. This way the kernel embraces
the recursive model proposed by Linux and the configuration model
proposed by Kconfig.

The |codename|'s build system is application centered, unlike the Linux Kbuild system. Therefore,
the build system requires an application to build the kernel's image. For that reason, the build
structure follows the Kbuild architecture, yet requires an application directory. The
application's directory contains the application's definition and code files. The build system
compiles the kernel and the application into a single image.

Scope
*****

The following sections provide details about the build system implementation. The information is
intended for application developers wanting to use the kernel as a development platform and for
kernel developers looking to modify or expand the kernel's functionality.

.. toctree::
   :maxdepth: 1

   kbuild_kconfig
   kbuild_makefiles
   kbuild_project
   kbuild_targets
   kbuild_toolchains

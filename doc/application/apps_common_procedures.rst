.. _apps_common_procedures:

Common Procedures
#################

Instructions that are common to many procedures are provided here
and referred to from the procedures that require them.

Procedures
**********

* `set_environment_variables`_

Setting Environment Variables
=============================

Set environment variables every time you open a terminal console to work on
|codename|'s applications.

Steps
-----

1. In a shell terminal console, enter an :command:`export` command that is
  consistent with your toolchain.

   a) For a Yocto toolchain:

      .. code-block:: bash

         $ export ZEPHYR_GCC_VARIANT=yocto
         $ export YOCTO_SDK_INSTALL_DIR=<yocto-installation-path>

   Go to Step 2.

   b) For an Xtools toolchain:

      .. code-block:: bash

         $ export ZEPHYR_GCC_VARIANT=xtools
         $ export XTOOLS_TOOLCHAIN_PATH=/opt/otc_viper/x-tools/
         $ export QEMU_BIN_PATH=/usr/local/bin

2. To set the environment variable :envvar:`$(ZEPHYR_BASE)`, navigate to the
   kernel's installation directory and enter:

      .. code-block:: bash

         $ source zephyr-env.sh

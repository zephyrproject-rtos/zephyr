.. _apps_common_procedures:

Common Procedures
#################

Instructions that are common to many procedures are provided here
and referred to from the procedures that require them.

Procedures
**********

.. _set_environment_variables:

Setting Environment Variables
=============================

Set environment variables every time you open a terminal console to work on
applications.

Steps
-----

1. In a shell terminal console, enter an :command:`export` command that is
   consistent with your toolchain.

   For the Zephyr SDK:

     .. code-block:: console

      $ export ZEPHYR_GCC_VARIANT=zephyr
      $ export ZEPHYR_SDK_INSTALL_DIR=<yocto-installation-path>

2. To set the environment variable :envvar:`\$ZEPHYR_BASE`, navigate to the
   kernel's installation directory and enter:

      .. code-block:: console

         $ source zephyr-env.sh

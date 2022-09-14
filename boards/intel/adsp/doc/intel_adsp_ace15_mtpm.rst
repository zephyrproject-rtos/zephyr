.. _intel_adsp_ace15:

Intel ADSP ACE 1.5
##################

Overview
********

This board configuration is used to run Zephyr on the Intel ACE 1.5 Audio DSP.
This configuration is present, for example, on Intel Meteor Lake microprocessors.
Refer to :ref:`intel_adsp_generic` for more details on Intel ADSP ACE and CAVS.

System requirements
*******************

Xtensa Toolchain
----------------

If you choose to build with the Xtensa toolchain instead of the Zephyr SDK, set
the following environment variables specific to the board in addition to the
Xtensa toolchain environment variables listed in :ref:`intel_adsp_generic`.

.. code-block:: shell

   export ZEPHYR_TOOLCHAIN_VARIANT=xt-clang
   export TOOLCHAIN_VER=RI-2021.7-linux
   export XTENSA_CORE=ace10_LX7HiFi4

For older versions of the toolchain, set the toolchain variant to ``xcc``.

Programming and Debugging
*************************

Refer to :ref:`intel_adsp_generic` for generic instructions on programming and
debugging applicable to all CAVS and ACE platforms.

.. _toolchain_xc32:

MPLAB XC32
##########

#. Download and install `MPLAB XC32 Compiler`_ for your operating system.

#. :ref:`Set these environment variables <env_vars>`:

   - Set :envvar:`ZEPHYR_TOOLCHAIN_VARIANT` to ``xc32``.
   - Set :envvar:`XC32_TOOLCHAIN_PATH` to the toolchain installation
     directory.

#. To check that you have set these variables correctly in your current
   environment, follow these example shell sessions (the
   :envvar:`XC32_TOOLCHAIN_PATH` values may be different on your system):

   .. code-block:: console

      # Linux, macOS:
      $ echo $ZEPHYR_TOOLCHAIN_VARIANT
      xc32
      $ echo $XC32_TOOLCHAIN_PATH
      /opt/microchip/xc32/v5.00

      # Windows:
      > echo %ZEPHYR_TOOLCHAIN_VARIANT%
      xc32
      > echo %XC32_TOOLCHAIN_PATH%
      C:\Microchip\xc32\v5.00

#. For Microchip SoCs, set :envvar:`XC_PACK_DIR` to the root directory
   containing installed Microchip Device Family Packs (DFPs).

   .. note::

      Zephyr uses :envvar:`XC_PACK_DIR` to find the DFP matching the selected
      SoC and to pass the required ``-mdfp`` and ``-mprocessor`` flag to the XC32 toolchain.

   For example:

   .. code-block:: console

      # Linux, macOS:
      $ export XC_PACK_DIR=/home/path/.packs/microchip

      # Windows:
      > set XC_PACK_DIR=C:\Users\path\.mchp_packs\Microchip

.. _MPLAB XC32 Compiler: https://www.microchip.com/en-us/tools-resources/develop/mplab-xc-compilers

.. _toolchain_designware_arc_mwdt:

DesignWare ARC MetaWare Development Toolkit (MWDT)
##################################################

#. You need to have `ARC MWDT <https://www.synopsys.com/dw/ipdir.php?ds=sw_metaware>`_ installed on
   your host.

#. You need to have :ref:`Zephyr SDK <toolchain_zephyr_sdk>` installed on your host.

   .. note::
      A Zephyr SDK is used as a source of tools like device tree compiler (DTC), QEMU, etc...
      Even though ARC MWDT toolchain is used for Zephyr RTOS build, still the GNU preprocessor & GNU
      objcopy might be used for some steps like device tree preprocessing and ``.bin`` file
      generation. We used Zephyr SDK as a source of these ARC GNU tools as well.
      To setup ARC GNU toolchain please use SDK Bundle (Full or Minimal) instead of manual installation
      of separate tarballs. It installs and registers toolchain and host tools in the system,
      that allows you to avoid toolchain related issues while building Zephyr.

#. :ref:`Set these environment variables <env_vars>`:

   - Set :envvar:`ZEPHYR_TOOLCHAIN_VARIANT` to ``arcmwdt``.
   - Set :envvar:`ARCMWDT_TOOLCHAIN_PATH` to the toolchain installation directory. MWDT installation
     provides :envvar:`METAWARE_ROOT` so simply set :envvar:`ARCMWDT_TOOLCHAIN_PATH` to
     ``$METAWARE_ROOT/../`` (Linux) or ``%METAWARE_ROOT%\..\`` (Windows).

   .. tip::
      If you have only one ARC MWDT toolchain version installed on your machine you may skip setting
      :envvar:`ARCMWDT_TOOLCHAIN_PATH` - it would be detected automatically.

#. To check that you have set these variables correctly in your current
   environment, follow these example shell sessions (the
   :envvar:`ARCMWDT_TOOLCHAIN_PATH` values may be different on your system):

   .. code-block:: console

      # Linux:
      $ echo $ZEPHYR_TOOLCHAIN_VARIANT
      arcmwdt
      $ echo $ARCMWDT_TOOLCHAIN_PATH
      /home/you/ARC/MWDT_2023.03/

      # Windows:
      > echo %ZEPHYR_TOOLCHAIN_VARIANT%
      arcmwdt
      > echo %ARCMWDT_TOOLCHAIN_PATH%
      C:\ARC\MWDT_2023.03\

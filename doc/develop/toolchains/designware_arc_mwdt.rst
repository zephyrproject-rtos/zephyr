.. _toolchain_designware_arc_mwdt:

DesignWare ARC MetaWare Development Toolkit (MWDT)
##################################################

#. You need to have `ARC MWDT
   <https://www.synopsys.com/dw/ipdir.php?ds=sw_metaware>`_ installed on your
   host.

#. :ref:`Set these environment variables <env_vars>`:

   - Set :envvar:`ZEPHYR_TOOLCHAIN_VARIANT` to ``arcmwdt``.
   - Set :envvar:`ARCMWDT_TOOLCHAIN_PATH` to the toolchain installation
     directory. MWDT installation provides :envvar:`METAWARE_ROOT` so simply set
     :envvar:`ARCMWDT_TOOLCHAIN_PATH` to ``$METAWARE_ROOT/../`` (Linux)
     or ``%METAWARE_ROOT%\..\`` (Windows)

#. To check that you have set these variables correctly in your current
   environment, follow these example shell sessions (the
   :envvar:`ARCMWDT_TOOLCHAIN_PATH` values may be different on your system):

   .. code-block:: console

      # Linux:
      $ echo $ZEPHYR_TOOLCHAIN_VARIANT
      arcmwdt
      $ echo $ARCMWDT_TOOLCHAIN_PATH
      /home/you/ARC/MWDT_2019.12/

      # Windows:
      > echo %ZEPHYR_TOOLCHAIN_VARIANT%
      arcmwdt
      > echo %ARCMWDT_TOOLCHAIN_PATH%
      C:\ARC\MWDT_2019.12\

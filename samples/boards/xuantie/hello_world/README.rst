Overview
========

``hello_world`` is the simplest example in the XuanTie Zephyr SDK, ported from ``samples/hello_world``, and can run in a QEMU environment.

Building and Running on Linux
=============================

Building
--------

.. code-block:: bash

    west build -b xuantie_${PLATFORM}_${CPUTYPE}/riscv_${PLATFORM}/${FEATURE} samples/boards/xuantie/hello_world --pristine

For builds that do not involve ``${FEATURE}``, use:

.. code-block:: bash

    west build -b xuantie_${PLATFORM}_${CPUTYPE} samples/boards/xuantie/hello_world --pristine

- ``${CPUTYPE}``:
  e902 e902m e902t e902mt e906 e906f e906fd e906p e906fp e906fdp
  e907 e907f e907fd e907p e907fp e907fdp
  r908 r908fd r908fdv r908-cp r908fd-cp r908fdv-cp r910 r920
  c906 c906fd c906fdv c908 c908v c908i
  c910 c910v2 c910v3 c910v3-cp c920 c920v2 c920v3 c920v3-cp
  c907 c907fd c907fdv c907fdvm c907-rv32 c907fd-rv32 c907fdv-rv32 c907fdvm-rv32

- ``${PLATFORM}``:
  smartl xiaohui

- ``${FEATURE}``:
  clic

.. note::
   The ``clic`` feature is only supported for the r908 series.

Example: to build for XuanTie CPU type ``e907fdp``:

.. code-block:: bash

    west build -b xuantie_smartl_e907fdp samples/boards/xuantie/hello_world --pristine

To build for an r908-series CPU with CLIC support (e.g., ``r908fdv``):

.. code-block:: bash

    west build -b xuantie_xiaohui_r908fdv/riscv_xiaohui/clic samples/boards/xuantie/hello_world --pristine

Running
-------

This example can run on either the XuanTie QEMU emulator or FPGA platforms.

Running on XuanTie QEMU
~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   (xuantie qemu)
   qemu-system-riscv32 -machine smartl -cpu e907fdp -nographic -kernel build/zephyr/zephyr.elf

Exiting QEMU from the terminal
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1. Press ``Ctrl+A``
2. Release all keys, then immediately press ``x``

Running on FPGA Platforms
~~~~~~~~~~~~~~~~~~~~~~~

For instructions on running this example on FPGA platforms, please refer to the *XuanTie Zephyr SDK User Manual*.

Expected Output
~~~~~~~~~~~~~~~

When running successfully, the serial output should look similar to the following:

.. code-block:: text

    *** Booting Zephyr OS build 743af5c2045c ***
    Hello World Sample Begin
    Hello World! xiaohui_c908
    Hello World! xiaohui_c908, cnt = 10, cpu_id: 0
    Hello World! xiaohui_c908, cnt = 9, cpu_id: 0
    Hello World! xiaohui_c908, cnt = 8, cpu_id: 0
    Hello World! xiaohui_c908, cnt = 7, cpu_id: 3
    Hello World! xiaohui_c908, cnt = 6, cpu_id: 3
    Hello World! xiaohui_c908, cnt = 5, cpu_id: 0
    Hello World! xiaohui_c908, cnt = 4, cpu_id: 0
    Hello World! xiaohui_c908, cnt = 3, cpu_id: 3
    Hello World! xiaohui_c908, cnt = 2, cpu_id: 0
    Hello World! xiaohui_c908, cnt = 1, cpu_id: 3
    Hello World Sample End

Related Documentation and Tools
===============================

Please visit the official XuanTie website https://www.xrvm.cn to search for and download the following resources:

1. *XuanTie Zephyr SDK User Manual*
2. XuanTie QEMU Emulator Tool

Notes
=====

1. For setting up the basic development environment on Linux, please refer to the *XuanTie Zephyr SDK User Manual*.
2. XuanTie eXX-series CPUs only support the ``smartl`` platform, while cXX/rXX-series CPUs only support the ``xiaohui`` platform.
3. Currently, building is only supported via the Linux command line; IDE-based compilation is not supported.
4. Some examples depend on specific hardware features that may not be emulated in QEMU; such examples can only run on the corresponding FPGA hardware platforms.
5. When using processors with vector or matrix extensions, please correctly configure ``CONFIG_XUANTIE_RISCV_VLENB_LEN`` and ``CONFIG_XUANTIE_RISCV_RLENB_LEN`` as described in Section 4 of the *XuanTie Zephyr SDK User Manual*.

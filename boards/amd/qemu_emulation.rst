.. _amd-qemu-emulation:

QEMU emulation for AMD adaptive SoC boards
##########################################

The AMD Versal, Versal NET, Versal Gen 2 and Zynq UltraScale+ MPSoC boards under
``boards/amd`` can be run under QEMU. These SoCs are modeled by the Xilinx fork of
QEMU using its ``arm-generic-fdt`` machine, which instantiates the whole SoC from a
hardware device tree supplied on the command line with ``-hw-dtb``. This hardware
device tree is a QEMU input and is unrelated to the devicetree Zephyr builds for the
board; it is never parsed by Zephyr.

Rather than vendoring these large device trees in the Zephyr tree, they are built on
demand from the upstream `Xilinx/qemu-devicetrees`_ repository, consumed as an
optional Zephyr module.

.. _Xilinx/qemu-devicetrees: https://github.com/Xilinx/qemu-devicetrees

Requirements
************

* The Zephyr SDK, which ships the Xilinx QEMU fork as ``qemu-system-xilinx-aarch64``.
  No separate QEMU installation is needed. If the environment exports
  ``QEMU_BIN_PATH``, make sure it points at an installation that provides
  ``qemu-system-xilinx-aarch64``, otherwise unset it so the SDK's binary is used.

* The optional ``qemu-devicetrees`` module. It is part of the ``optional`` manifest
  group, which is not enabled by default. Enable it and update the workspace:

  .. code-block:: console

     west config manifest.group-filter +optional
     west update qemu-devicetrees

  The module also requires the host tools ``make``, ``cpp``/``gcc`` and ``dtc`` to
  build a device tree.

Building and running
********************

With the module present, QEMU emulation is enabled automatically for the board.
Build an application and run it as usual:

.. code-block:: console

   west build -b versal2_rpu samples/hello_world
   west build -t run

If the module is absent the board builds normally as a hardware target and QEMU
emulation is simply not offered; nothing else changes.

.. note::

   These boards are not declared as QEMU simulation targets for Twister, because the
   ``qemu-devicetrees`` module is optional and is not available in upstream CI. QEMU
   emulation is therefore a developer convenience driven by ``west build -t run``, not
   an automated test platform.

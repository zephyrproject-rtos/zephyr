.. zephyr:board:: litex_vexiiriscv

LiteX VexiiRiscv is an example of a system on a chip (SoC) that consists of
a `VexiiRiscv processor <https://github.com/SpinalHDL/VexiiRiscv>`_
and additional peripherals. This setup can be generated using
`Zephyr on LiteX VexiiRiscv (reference platform)
<https://github.com/litex-hub/zephyr-on-litex-vexiiriscv>`_
or `LiteX SoC Builder <https://github.com/enjoy-digital/litex>`_
and can be used on various FPGA chips.
The bitstream (FPGA configuration file) can be obtained using both
vendor-specific and open-source tools.

The ``litex_vexiiriscv`` board configuration in Zephyr is meant for the
LiteX VexiiRiscv SoC implementation generated for the
`Efinix Titanium Ti375 C529 Development Kit
<https://www.efinixinc.com/products-devkits-titaniumti375c529.html>`_.

LiteX is based on
`Migen <https://m-labs.hk/gateware/migen/>`_/`MiSoC SoC builder <https://github.com/m-labs/misoc>`_
and provides ready-made system components such as buses, streams, interconnects,
common cores, and CPU wrappers to create SoCs easily. The tool contains
mechanisms for integrating, simulating, and building various designs
that target multiple chips of different vendors.
More information about the LiteX project can be found on
`LiteX's website <https://github.com/enjoy-digital/litex>`_.

VexiiRiscv is a implementation of the RISC-V CPU architecture
written in the `SpinalHDL <https://spinalhdl.github.io/SpinalDoc-RTD/>`_.
The implementation is optimized for FPGA chips.
More information about the project can be found on
`VexiiRiscv's website <https://github.com/SpinalHDL/VexiiRiscv>`_.

To run the ZephyrOS on the VexiiRiscv CPU, it is necessary to prepare the
bitstream for the FPGA on a Efinix Titanium Ti375 C529 Development Kit. This can be achieved
using the
`Zephyr on LiteX VexiiRiscv <https://github.com/litex-hub/zephyr-on-litex-vexiiriscv>`_
reference platform. You can also use the official LiteX SoC Builder.

Supported Features
******************

.. zephyr:board-supported-hw::

Bitstream generation
********************

Zephyr on LiteX VexiiRiscv
==========================
Using this platform ensures that all registers addresses are in the proper place.
All drivers were tested using this platform.
In order to generate the bitstream,
proceed with the following instruction:

1. Clone the repository and update all submodules:

   .. code-block:: bash

      git clone https://github.com/litex-hub/zephyr-on-litex-vexiiriscv.git
      cd zephyr-on-litex-vexiiriscv
      git submodule update --init --recursive

#. Follow the instructions in the README of in
   `Zephyr on LiteX VexiiRiscv <https://github.com/litex-hub/zephyr-on-litex-vexiiriscv>`_


Official LiteX SoC builder
==========================
You can also generate the bitstream using the `official LiteX repository <https://github.com/enjoy-digital/litex>`_.
In that case you must also generate a dts overlay.

1. Create a virtual environment and activate it:

   .. code-block:: bash

      python3 -m venv .venv
      source .venv/bin/activate

#. Install Migen/LiteX and the LiteX's cores:

   .. code-block:: bash

      wget https://raw.githubusercontent.com/enjoy-digital/litex/master/litex_setup.py
      chmod +x litex_setup.py
      ./litex_setup.py --init --install --user (--user to install to user directory) --config=(minimal, standard, full)

#. Install the RISC-V toolchain:

   .. code-block:: bash

      pip3 install meson ninja
      ./litex_setup.py --gcc=riscv

#. Build the target:

   .. code-block:: bash

      ./litex-boards/litex_boards/targets/efinix_ti375_c529_dev_kit.py --cpu-type=vexiiriscv --build

#. Generate the dts and config overlay:

   .. code-block:: bash

      ./litex/litex/tools/litex_json2dts_zephyr.py --dts overlay.dts --config overlay.config csr.json

Programming and booting
*************************

Building
========

Applications for the ``litex_vexiiriscv`` board configuration can be built as usual
(see :ref:`build_an_application`).
In order to build the application for ``litex_vexiiriscv``, set the ``BOARD`` variable
to ``litex_vexiiriscv``.

If you were generating bitstream with the official LiteX SoC builder you need to pass an additional argument:

.. code-block:: bash

   west build -b litex_vexiiriscv path/to/app -DDTC_OVERLAY_FILE=path/to/overlay.dts

Booting
=======

To upload the bitstream to Digilent Arty A7-35 you can either use the
``--load`` option or the ``--flash`` option to program the SPI flash.

You can boot from a serial port using litex_term (replace ``ttyUSBX`` with your device) , e.g.:

.. code-block:: bash

   litex_term /dev/ttyUSBX --speed 115200 --kernel zephyr.bin

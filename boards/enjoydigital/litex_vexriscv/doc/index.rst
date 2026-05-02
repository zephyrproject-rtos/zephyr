.. zephyr:board:: litex_vexriscv

LiteX VexRiscv is an example of a system on a chip (SoC) that consists of
a `VexRiscv processor <https://github.com/SpinalHDL/VexRiscv>`_
and additional peripherals. This setup can be generated using
`Zephyr on LiteX VexRiscv (reference platform)
<https://github.com/litex-hub/zephyr-on-litex-vexriscv>`_
or `LiteX SoC Builder <https://github.com/enjoy-digital/litex>`_
and can be used on various FPGA chips.
The bitstream (FPGA configuration file) can be obtained using both
vendor-specific and open-source tools, including the
`F4PGA toolchain <https://f4pga.org/>`_.

The ``litex_vexriscv`` board configuration in Zephyr is meant for the
LiteX VexRiscv SoC implementation generated for the
`Digilent Arty A7-35T or A7-100T Development Boards
<https://store.digilentinc.com/arty-a7-artix-7-fpga-development-board-for-makers-and-hobbyists>`_
or `SDI-MIPI Video Converter <https://github.com/antmicro/sdi-mipi-video-converter>`_.

LiteX is based on
`Migen <https://m-labs.hk/gateware/migen/>`_/`MiSoC SoC builder <https://github.com/m-labs/misoc>`_
and provides ready-made system components such as buses, streams, interconnects,
common cores, and CPU wrappers to create SoCs easily. The tool contains
mechanisms for integrating, simulating, and building various designs
that target multiple chips of different vendors.
More information about the LiteX project can be found on
`LiteX's website <https://github.com/enjoy-digital/litex>`_.

VexRiscv is a 32-bit implementation of the RISC-V CPU architecture
written in the `SpinalHDL <https://spinalhdl.github.io/SpinalDoc-RTD/>`_.
The processor supports M, C, and A RISC-V instruction
set extensions, with numerous optimizations that include multistage
pipelines and data caching. The project provides many optional extensions
that can be used to customize the design (JTAG, MMU, MUL/DIV extensions).
The implementation is optimized for FPGA chips.
More information about the project can be found on
`VexRiscv's website <https://github.com/SpinalHDL/VexRiscv>`_.

To run the ZephyrOS on the VexRiscv CPU, it is necessary to prepare the
bitstream for the FPGA on a Digilent Arty A7-35 Board or SDI-MIPI Video Converter. This can be achieved
using the
`Zephyr on LiteX VexRiscv <https://github.com/litex-hub/zephyr-on-litex-vexriscv>`_
reference platform. You can also use the official LiteX SoC Builder.

Supported Features
******************

.. zephyr:board-supported-hw::

Bitstream generation
********************

Zephyr on LiteX VexRiscv
========================
Using this platform ensures that all registers addresses are in the proper place.
All drivers were tested using this platform.
In order to generate the bitstream,
proceed with the following instruction:

1. Clone the repository and update all submodules:

   .. code-block:: bash

      git clone https://github.com/litex-hub/zephyr-on-litex-vexriscv.git
      cd zephyr-on-litex-vexriscv
      git submodule update --init --recursive

   Generating the bitstream for the Digilent Arty A7-35 Board requires F4PGA toolchain installation. It can be done by following instructions in
   `this tutorial <https://f4pga-examples.readthedocs.io/en/latest/getting.html>`_.

   In order to generate the bitstream for the SDI-MIPI Video Converter, install
   oxide (yosys+nextpnr) toolchain by following
   `these instructions <https://github.com/gatecat/prjoxide#getting-started---complete-flow>`_.

#. Next, get all required packages and run the install script:

   .. code-block:: bash

      apt-get install build-essential bzip2 python3 python3-dev python3-pip
      ./install.sh

#. Add LiteX to path:

   .. code-block:: bash

      source ./init

#. Set up the F4PGA environment (for the Digilent Arty A7-35 Board):

   .. code-block:: bash

      export F4PGA_INSTALL_DIR=~/opt/f4pga
      export FPGA_FAM="xc7"
      export PATH="$F4PGA_INSTALL_DIR/$FPGA_FAM/install/bin:$PATH";
      source "$F4PGA_INSTALL_DIR/$FPGA_FAM/conda/etc/profile.d/conda.sh"
      conda activate $FPGA_FAM

#. Generate the bitstream for the Arty 35T:

   .. code-block:: bash

      ./make.py --board=arty --variant=a7-35 --build --toolchain=symbiflow

#. Generate the bitstream for the Arty 100T:

   .. code-block:: bash

      ./make.py --board=arty --variant=a7-100 --build --toolchain=symbiflow

#. Generate the bitstream for the SDI-MIPI Video Converter:

   .. code-block:: bash

      ./make.py --board=sdi_mipi_bridge --build --toolchain=oxide

Official LiteX SoC builder
==========================
You can also generate the bitstream using the `official LiteX repository <https://github.com/enjoy-digital/litex>`_.
In that case you must also generate a dts overlay.

1. Install Migen/LiteX and the LiteX's cores:

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

      ./litex-boards/litex_boards/targets/digilent_arty.py --build --timer-uptime --csr-json csr.json

#. Generate the dts and config overlay:

   .. code-block:: bash

      ./litex/litex/tools/litex_json2dts_zephyr.py --dts overlay.dts --config overlay.config csr.json

Programming and booting
*************************

Building
========

Applications for the ``litex_vexriscv`` board configuration can be built as usual
(see :ref:`build_an_application`).
In order to build the application for ``litex_vexriscv``, set the ``BOARD`` variable
to ``litex_vexriscv``.

If you were generating bitstream with the official LiteX SoC builder you need to pass an additional argument:

.. code-block:: bash

   west build -b litex_vexriscv path/to/app -DDTC_OVERLAY_FILE=path/to/overlay.dts

Booting
=======

To upload the bitstream to Digilent Arty A7-35 you can use `xc3sprog <https://github.com/matrix-io/xc3sprog>`_ or
`openFPGALoader <https://github.com/trabucayre/openFPGALoader>`_:

.. code-block:: bash

   xc3sprog -c nexys4 digilent_arty.bit

.. code-block:: bash

   openFPGALoader -b arty_a7_100t digilent_arty.bit

Use `ecpprog <https://github.com/gregdavill/ecpprog>`_ to upload the bitstream to SDI-MIPI Video Converter:

.. code-block:: bash

   ecpprog -S antmicro_sdi_mipi_video_converter.bit

You can boot from a serial port using litex_term (replace ``ttyUSBX`` with your device) , e.g.:

.. code-block:: bash

   litex_term /dev/ttyUSBX --speed 115200 --kernel zephyr.bin

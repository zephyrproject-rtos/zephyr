.. _embarc_mli_hello_world:

Hello_world Example
###################

Quick Start
--------------

Example supports building with [Zephyr Software Development Kit (SDK)](https://docs.zephyrproject.org/latest/getting_started/installation_linux.html#zephyr-sdk) and running with MetaWare Debuger on [nSim simulator](https://www.synopsys.com/dw/ipdir.php?ds=sim_nSIM).

Add embarc_mli module to Zephyr instruction
-------------------------------------------

1. Open command line and change working directory to './zephyrproject/zephyr'

2. Download embarc_mli version 1.1

        west update

Build with Zephyr SDK toolchain
-------------------------------

    Build requirements:
        - Zephyr SDK toolchain version 0.12.3 or higher
        - gmake

1. Open command line and change working directory to './zephyrproject/zephyr/samples/modules/embarc_mli/hello_world'

2. Build example

        west build -b nsim_em samples/modules/embarc_mli/hello_world

Run example
--------------

1. Run example

        west flash

    Result shall be:

.. code-block:: console

        ...

        ************************************

        2 4 6 8 10 12 14 16

        0 0 0 0 0 0 0 0

        ************************************

        Hello World! nsim

        ...

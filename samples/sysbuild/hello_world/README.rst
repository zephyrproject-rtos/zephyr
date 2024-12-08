.. zephyr:code-sample:: sysbuild_hello_world
   :name: Hello World for multiple board targets using Sysbuild

   Run a hello world sample on multiple board targets

Overview
********

The sample demonstrates how to build a Hello World application for two board
targets with :ref:`sysbuild`. This sample can be useful to test, for example,
SoCs with multiple cores as each core is exposed as a board target. Other
scenarios could include boards embedding multiple SoCs. When building with
Zephyr Sysbuild, the build system adds additional images based on the options
selected in the project's additional configuration and build files.

All images use the same :file:`main.c` that prints the board target on which the
application is programmed.

Building and Running
********************

This sample needs to be built with Sysbuild by using the ``--sysbuild`` option.
The remote board needs to be specified using ``SB_CONFIG_REMOTE_BOARD``. Some
additional settings may be required depending on the platform, for example,
to boot a remote core.

.. note::
   It is recommended to use sample setups from
   :zephyr_file:`samples/sysbuild/hello_world/sample.yaml` using the
   ``-T`` option.

Here's an example to build and flash the sample for the
:ref:`nrf54h20dk_nrf54h20`, using application and radio cores:

.. zephyr-app-commands::
   :zephyr-app: samples/sysbuild/hello_world
   :board: nrf54h20dk/nrf54h20/cpuapp
   :west-args: --sysbuild
   :gen-args: -DSB_CONFIG_REMOTE_BOARD='"nrf54h20dk/nrf54h20/cpurad"'
   :goals: build flash
   :compact:

The same can be achieved by using the
:zephyr_file:`samples/sysbuild/hello_world/sample.yaml` setup:

.. zephyr-app-commands::
   :zephyr-app: samples/sysbuild/hello_world
   :board: nrf54h20dk/nrf54h20/cpuapp
   :west-args: -T sample.sysbuild.hello_world.nrf54h20dk_cpuapp_cpurad
   :goals: build flash
   :compact:

After programming the sample to your board, you should observe a hello world
message in the Zephyr console configured on each target. For example, for the
sample above:

Application core

   .. code-block:: console

      *** Booting Zephyr OS build v3.6.0-274-g466084bd8c5d ***
      Hello world from nrf54h20dk/nrf54h20/cpuapp

Radio core

   .. code-block:: console

      *** Booting Zephyr OS build v3.6.0-274-g466084bd8c5d ***
      Hello world from nrf54h20dk/nrf54h20/cpurad

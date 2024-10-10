.. zephyr:code-sample:: llext-modules
   :name: Linkable loadable extensions "module" sample
   :relevant-api: llext_apis

    Call a function in a loadable extension module,
    either built-in or loaded at runtime.

Overview
********

This sample demonstrates the use of the :ref:`llext` subsystem in Zephyr. The
llext subsystem allows for the loading of relocatable ELF files at runtime;
their symbols can be accessed and functions called.

Specifically, this shows how to call a simple "hello world" function,
implemented in :zephyr_file:`samples/subsys/llext/modules/src/hello_world_ext.c`.
This is achieved in two different ways, depending on the value of the Kconfig
symbol ``CONFIG_HELLO_WORLD_MODE``:

- if it is ``y``, the function is directly compiled and called by the Zephyr
  application. The caller code used in this case is in
  :zephyr_file:`samples/subsys/llext/modules/src/main_builtin.c`.

- if it is ``m``, the function is compiled as an llext and it is included in
  the application as a binary blob. At runtime, the llext subsystem is used to
  load the extension and call the function. The caller code is in
  :zephyr_file:`samples/subsys/llext/modules/src/main_module.c`.

Requirements
************

A board with a supported llext architecture and console. This can also be
executed in QEMU emulation on the :ref:`qemu_xtensa <qemu_xtensa>` or
:ref:`qemu_cortex_r5 <qemu_cortex_r5>` virtual boards.

Building and running
********************

- By default, the sample will compile the function along with the rest of
  Zephyr in the same binary. This can be verified via the following commands:

  .. zephyr-app-commands::
     :zephyr-app: samples/subsys/llext/modules
     :board: qemu_xtensa/dc233c
     :goals: build run
     :compact:

- The following commands build and run the sample so that the extension code is
  compiled separately and included in the Zephyr image as a binary blob:

  .. zephyr-app-commands::
     :zephyr-app: samples/subsys/llext/modules
     :board: qemu_xtensa/dc233c
     :goals: build run
     :west-args: -T sample.llext.modules.module_build
     :compact:

  .. important::
     Take a look at :zephyr_file:`samples/subsys/llext/modules/sample.yaml` for
     the additional architecture-specific configurations required in this case.

To build for a different board, replace ``qemu_xtensa`` in the commands above
with the desired board name.

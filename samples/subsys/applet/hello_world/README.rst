.. zephyr:code-sample:: applet-hello-world
   :name: Applet "Hello World" sample
   :relevant-api: applet_apis

   Show the lifecycle of an applet that is either
   native or backed by LLEXT.

Overview
********

The sample demonstrates the use of the :ref:`applet` subsystem,
which allows for the grouping of threads into subapplications ("applets")
and simplified memory domain management for those applets. Users can
choose between a native applet, which is compiled into the Zephyr image,
or a LLEXT-backed applet, which is compiled as an ELF and loaded at runtime.

Specifically, this shows the lifecycle of a simple "hello world" applet,
implemented in
:zephyr_file:`samples/subsys/applet/hello_world/src/hello_world_applet.c`.
Depending on whether :kconfig:option:`CONFIG_LLEXT` is enabled, the applet
is built as a LLEXT and loaded at runtime or either compiled into the Zephyr image
("native").

Requirements
************

If using LLEXT, a supported llext architecture and console. This can also be
executed in QEMU emulation.

Building and running
********************

- By default, the sample will use a native applet, which is compiled into the
  Zephyr image.

  .. zephyr-app-commands::
     :zephyr-app: samples/subsys/applet/hello_world
     :board: mps2/an385
     :goals: build run
     :compact:

- The following commands build and run the sample so that the applet is built
   as a LLEXT.

  .. zephyr-app-commands::
     :zephyr-app: samples/subsys/applet/hello_world
     :board: mps2/an385
     :goals: build run
     :west-args: -T sample.applet.hello_world.llext
     :compact:

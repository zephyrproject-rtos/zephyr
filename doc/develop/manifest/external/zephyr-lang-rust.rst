.. _external_module_rust:

Rust Language Support
#####################

Introduction
************

The `zephyr-lang-rust`_ module adds support for writing Zephyr applications in Rust. It provides
the ``zephyr`` crate, which exposes Zephyr kernel services, devices and logging to Rust code, the
build support that compiles an application's Cargo crate and links it into the Zephyr image, and
code generation from the devicetree.

The module is licensed under the Apache-2.0 license.

Usage with Zephyr
*****************

To pull in the module as a Zephyr :ref:`module <modules>`, either add it as a West project in the
``west.yaml`` file or pull it in by adding a submanifest (e.g.
``zephyr/submanifests/zephyr-lang-rust.yaml``) file with the following content and run
``west update``:

.. code-block:: yaml

   manifest:
     projects:
       - name: zephyr-lang-rust
         url: https://github.com/zephyrproject-rtos/zephyr-lang-rust
         revision: main
         path: modules/lang/rust # adjust the path as needed

Building Rust applications also requires a Rust toolchain with the targets of the boards in use,
installed for example with ``rustup``. See :ref:`language_rust` for how to set up an application.

The module contains samples under ``samples`` and tests under ``tests``. Build and run the hello
world sample on :zephyr:board:`qemu_cortex_m3` with:

.. code-block:: console

   west build -b qemu_cortex_m3 modules/lang/rust/samples/hello_world -t run

Reference
*********

.. target-notes::

.. _zephyr-lang-rust:
   https://github.com/zephyrproject-rtos/zephyr-lang-rust

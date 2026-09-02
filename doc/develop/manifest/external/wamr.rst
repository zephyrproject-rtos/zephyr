.. _external_module_wamr:

WebAssembly Micro Runtime (WAMR)
################################

Introduction
************

`WebAssembly Micro Runtime`_ (WAMR) is a lightweight standalone WebAssembly runtime designed for
embedded and resource constrained devices. It lets an application load and execute WebAssembly
(WASM) modules at runtime, so functionality can be updated or extended without reflashing the
firmware, and untrusted code can run in the sandbox the WASM specification defines. See the
`WAMR documentation`_ for the runtime APIs, the execution modes and the ``wamrc`` AOT compiler.

WAMR supports Zephyr through a dedicated platform layer, and ships the Zephyr module glue
(``zephyr/module.yml``, ``zephyr/Kconfig`` and ``zephyr/CMakeLists.txt``) in its own repository,
so an application only has to select a few Kconfig options to get the runtime linked into its
image.

WAMR is licensed under the Apache License 2.0 with LLVM exceptions.

Usage with Zephyr
*****************

To use WAMR as a Zephyr module, add the following entry:

.. code-block:: yaml

   manifest:
     projects:
       - name: wasm-micro-runtime
         url: https://github.com/wasm-micro-runtime/wasm-micro-runtime
         revision: main
         path: modules/wasm-micro-runtime # adjust the path as needed

to a Zephyr submanifest (e.g. ``zephyr/submanifests/wamr.yaml``) and run ``west update``, or add
it as a West project in your project's ``west.yml`` manifest.

Configuring the runtime
=======================

The runtime is disabled by default. Enable it and select its features with the ``CONFIG_WAMR_*``
options in your application's ``prj.conf``:

.. code-block:: cfg

   CONFIG_WAMR=y
   CONFIG_WAMR_INTERP=y
   CONFIG_WAMR_AOT=y
   CONFIG_WAMR_LIBC_BUILTIN=y
   CONFIG_WAMR_GLOBAL_HEAP_POOL=y
   CONFIG_WAMR_GLOBAL_HEAP_SIZE=131072

Each option maps onto the corresponding ``WAMR_BUILD_*`` CMake variable used by the regular WAMR
build scripts, and can still be overridden on the CMake command line
(e.g. ``-DWAMR_BUILD_AOT=0``) for one-off builds.

``WAMR_BUILD_TARGET`` is derived from the board architecture, so it normally does not have to be
passed.

WAMR ships a set of `Zephyr samples`_; see their ``README.md`` for how to build and run them.

Reference
*********

.. target-notes::

.. _WebAssembly Micro Runtime:
   https://github.com/wasm-micro-runtime/wasm-micro-runtime

.. _WAMR documentation:
   https://github.com/wasm-micro-runtime/wasm-micro-runtime/tree/main/doc

.. _Zephyr samples:
   https://github.com/wasm-micro-runtime/wasm-micro-runtime/tree/main/product-mini/platforms/zephyr

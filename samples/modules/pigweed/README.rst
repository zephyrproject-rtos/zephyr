Pigweed
************

Pigweed is a broad open source collection of embedded-targeted libraries.
These libraries are called modules inside Pigweed. Pigweed is intergrated into Zephyr as a single Zephyr module.
Individual Pigweed modules can be enabled with Kconfig flags (e.g. `CONFIG_PIGWEED_ASSERT`).
Not every module can be expected to work with Zephyr out of the box.
Only pigweed modules with Zephyr samples are expected to work reliably.

For detailed documentation on individual Pigweed modules, refer to `Pigweed.dev <https://pigweed.dev/module_guides.html>`_.

Requirements
************

* The pigweed python module (included in `requirements-extra.txt`)
* The `pw_rpc` sample depends on the :ref:`nanopb<nanopb_sample>` module.

Building
************

To build the samples use the following west command:

.. zephyr-app-commands::
   :zephyr-app: samples/modules/pigweed/<pigweed_module_sample>
   :board: native_posix
   :goals: build

Maintenance
************

Bug fixes and feature changes should be sent to the upstream `Pigweed repository <https://pigweed.googlesource.com/pigweed/pigweed/>`_.
Pigweed APIs are subject to change, users should pin to specific revision in west.yml to ensure compatibility.

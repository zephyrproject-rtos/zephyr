.. _west-sign:

Signing Binaries
################

This page documents the ``west sign`` :ref:`extension <west-extensions>`
command included in the zephyr repository. It is used to sign a Zephyr
application binary for consumption by a bootloader using an external tool.

Currently, it supports signing binaries for use with the `MCUboot`_ bootloader,
using the `imgtool`_ program provided by its developers. Using ``west sign`` as
a wrapper around ``imgtool`` for Zephyr binaries is more convenient than using
``imgtool`` directly, because ``west sign`` knows how to read numeric values
needed by ``imgtool`` out of an application build directory. These values
differ depending on your :ref:`board <boards>`, so using ``west sign`` means
both shorter command lines and not having to learn or memorize
hardware-specific details.

To produce signed ``.bin`` and ``.hex`` files for a Zephyr application, make
sure ``imgtool`` is installed (e.g. with ``pip3 install imgtool`` on macOS and
Windows, and ``pip3 install --user imgtool`` on Linux), then run:

.. code-block:: console

   west sign -t imgtool -d YOUR_BUILD_DIR -- --key YOUR_SIGNING_KEY.pem

Above, :file:`YOUR_BUILD_DIR` is a Zephyr build directory containing an
application compiled for MCUboot (in practice, this means
:option:`CONFIG_BOOTLOADER_MCUBOOT` is ``y`` in the application's Kconfig).

Some additional notes follow. See ``west sign -h`` for detailed help.

- The default ``-d`` value is ``build``, which is the default output directory
  created by :ref:`west build <west-building>`.
- If you don't have your own signing key and have a default MCUboot build, use
  ``--key path/to/mcuboot/root-rsa-2048.pem``.
- By default, the output files produced by ``west sign`` are named
  :file:`zephyr.signed.bin` and :file:`zephyr.signed.hex` and are created in the
  build directory next to the unsigned :file:`zephyr.bin` and :file:`zephyr.hex`
  versions.

  You can control this using the ``-B`` and ``-H`` options, e.g. this would
  create :file:`my-signed.bin` and :file:`my-signed.hex` in the current working
  directory instead:

  .. code-block:: console

     west sign -t imgtool -B my-signed.bin -H my-signed.hex [...]

Example build flow
******************

For reference, here is an example showing how to build :ref:`hello_world` for
MCUboot using ``west``:

.. code-block:: console

   west build -b YOUR_BOARD samples/hello_world -- -DCONFIG_BOOTLOADER_MCUBOOT=y
   west sign -t imgtool -- --key YOUR_SIGNING_KEY.pem
   west flash --hex-file build/zephyr/zephyr.signed.hex

The availability of a hex file, and whether ``west flash`` uses it to flash,
depends on your board and build configuration. At least the west flash runners
using ``nrfjprog`` and ``pyocd`` work with the above flow.

.. _MCUboot:
   https://mcuboot.com/

.. _imgtool:
   https://pypi.org/project/imgtool/

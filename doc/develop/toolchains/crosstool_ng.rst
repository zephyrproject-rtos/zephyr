.. _toolchain_xtools:

Crosstool-NG (Deprecated)
#########################

.. warning::

   ``xtools`` toolchain variant is deprecated. The
   :ref:`cross-compile toolchain variant <other_x_compilers>` should be used
   when using a custom toolchain built with Crosstool-NG.

You can build toolchains from source code using crosstool-NG.

#. Follow the steps on the crosstool-NG website to `prepare your host
   <http://crosstool-ng.github.io/docs/os-setup/>`_.

#. Follow the `Zephyr SDK with Crosstool NG instructions
   <https://github.com/zephyrproject-rtos/sdk-ng/blob/master/README.md>`_ to
   build your toolchain. Repeat as necessary to build toolchains for multiple
   target architectures.

   You will need to clone the ``sdk-ng`` repo and run the following command:

   .. code-block:: console

      ./go.sh <arch>

   .. note::

      Currently, only i586 and Arm toolchain builds are verified.

#. :ref:`Set these environment variables <env_vars>`:

   - Set :envvar:`ZEPHYR_TOOLCHAIN_VARIANT` to ``xtools``.
   - Set :envvar:`XTOOLS_TOOLCHAIN_PATH` to the toolchain build directory.

#. To check that you have set these variables correctly in your current
   environment, follow these example shell sessions (the
   :envvar:`XTOOLS_TOOLCHAIN_PATH` values may be different on your system):

   .. code-block:: console

      # Linux, macOS:
      $ echo $ZEPHYR_TOOLCHAIN_VARIANT
      xtools
      $ echo $XTOOLS_TOOLCHAIN_PATH
      /Volumes/CrossToolNGNew/build/output/

.. _crosstool-ng site: http://crosstool-ng.org

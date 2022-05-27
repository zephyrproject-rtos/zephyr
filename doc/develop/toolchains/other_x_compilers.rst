.. _other_x_compilers:

Other Cross Compilers
######################

This toolchain variant is borrowed from the Linux kernel build system's
mechanism of using a :envvar:`CROSS_COMPILE` environment variable to set up a
GNU-based cross toolchain.

Examples of such "other cross compilers" are cross toolchains that your Linux
distribution packaged, that you compiled on your own, or that you downloaded
from the net. Unlike toolchains specifically listed in
:ref:`third_party_x_compilers`, the Zephyr build system may not have been
tested with them, and doesn't officially support them. (Nonetheless, the
toolchain set-up mechanism itself is supported.)

Follow these steps to use one of these toolchains.

#. Install a cross compiler suitable for your host and target systems.

   For example, you might install the ``gcc-arm-none-eabi`` package on
   Debian-based Linux systems, or ``arm-none-eabi-newlib`` on Fedora or Red
   Hat:

   .. code-block:: console

      # On Debian or Ubuntu
      sudo apt-get install gcc-arm-none-eabi
      # On Fedora or Red Hat
      sudo dnf install arm-none-eabi-newlib

#. :ref:`Set these environment variables <env_vars>`:

   - Set :envvar:`ZEPHYR_TOOLCHAIN_VARIANT` to ``cross-compile``.
   - Set :envvar:`CROSS_COMPILE` to the common path prefix which your
     toolchain's binaries have, e.g. the path to the directory containing the
     compiler binaries plus the target triplet and trailing dash.

#. To check that you have set these variables correctly in your current
   environment, follow these example shell sessions (the
   :envvar:`CROSS_COMPILE` value may be different on your system):

   .. code-block:: console

      # Linux, macOS:
      $ echo $ZEPHYR_TOOLCHAIN_VARIANT
      cross-compile
      $ echo $CROSS_COMPILE
      /usr/bin/arm-none-eabi-

   You can also set ``CROSS_COMPILE`` as a CMake variable.

When using this option, all of your toolchain binaries must reside in the same
directory and have a common file name prefix.  The ``CROSS_COMPILE`` variable
is set to the directory concatenated with the file name prefix. In the Debian
example above, the ``gcc-arm-none-eabi`` package installs binaries such as
``arm-none-eabi-gcc`` and ``arm-none-eabi-ld`` in directory ``/usr/bin/``, so
the common prefix is ``/usr/bin/arm-none-eabi-`` (including the trailing dash,
``-``).  If your toolchain is installed in ``/opt/mytoolchain/bin`` with binary
names based on target triplet ``myarch-none-elf``, ``CROSS_COMPILE`` would be
set to ``/opt/mytoolchain/bin/myarch-none-elf-``.

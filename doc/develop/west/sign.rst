.. _west-sign:

Signing Binaries
################

The ``west sign`` :ref:`extension <west-extensions>` command can be used to
sign a Zephyr application binary for consumption by a bootloader using an
external tool. In some configurations, ``west sign`` is also used to invoke
an external, post-processing tool that "stitches" the final components of
the image together. Run ``west sign -h`` for command line help.

rimage
******

rimage configuration uses an approach that does not rely on Kconfig or CMake
but on :ref:`west config<west-config>`, similar to
:ref:`west-building-cmake-config`.

Signing involves a number of "wrapper" scripts stacked on top of each other: ``west
flash`` invokes ``west build`` which invokes ``cmake`` and ``ninja`` which invokes
``west sign`` which invokes ``imgtool`` or `rimage`_. As long as the signing
parameters desired are the default ones and fairly static, these indirections are
not a problem. On the other hand, passing ``imgtool`` or ``rimage`` options through
all these layers can causes issues typical when the layers don't abstract
anything. First, this usually requires boilerplate code in each layer. Quoting
whitespace or other special characters through all the wrappers can be
difficult. Reproducing a lower ``west sign`` command to debug some build-time issue
can be very time-consuming: it requires at least enabling and searching verbose
build logs to find which exact options were used. Copying these options from the
build logs can be unreliable: it may produce different results because of subtle
environment differences. Last and worst: new signing feature and options are
impossible to use until more boilerplate code has been added in each layer.

To avoid these issues, ``rimage`` parameters can bet set in ``west config``.
Here's a ``workspace/.west/config`` example:

.. code-block:: ini

   [sign]
   # Not needed when invoked from CMake
   tool = rimage

   [rimage]
   # Quoting is optional and works like in Unix shells
   # Not needed when rimage can be found in the default PATH
   path = "/home/me/zworkspace/build-rimage/rimage"

   # Not needed when using the default development key
   extra-args = -i 4 -k 'keys/key argument with space.pem'

In order to support quoting, values are parsed by Python's ``shlex.split()`` like in
:ref:`west-building-cmake-args`.

The ``extra-args`` are passed directly to the ``rimage`` command. The example
above has the same effect as appending them on command line after ``--`` like this:
``west sign --tool rimage -- -i 4 -k 'keys/key argument with space.pem'``. In case
both are used, the command-line arguments go last.

.. _rimage:
   https://github.com/thesofproject/rimage

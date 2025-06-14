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

silabs_commander
***************

Signing involves a number of "wrapper" scripts stacked on top of each other: ``west
flash`` invokes ``west build`` which invokes ``cmake`` and ``ninja`` which invokes
``west sign`` which invokes ``imgtool`` or ``rimage`` or `silabs_commander`_.

The ``silabs_commander`` tool is used to apply sign or MIC or encrypt binaries for Silicon
Labs devices. It is invoked by ``west sign`` or ``west build`` when the ``sign.tool``
configuration is set to ``silabs_commander``. The tool requires a configuration file
that specifies the signing parameters, such as the key files. The configuration file
can be specified in the ``project config`` file and defined in the ``soc Kconfig``
file. Here is an example of a ``samples/example/prj.conf`` file for signing with 
``silabs_commander``:
.. code-block:: ini

   # Configuration for enabling MIC with silabs_commander
   CONFIG_SIWX91X_SILABS_OTA_MIC_ENABLE = n or y

   # Configuration for enabling encryption with silabs_commander
   CONFIG_SIWX91X_SILABS_OTA_ENCRYPT_ENABLE = n or y

   # Configuration for enabling signing with silabs_commander
   CONFIG_SIWX91X_SILABS_OTA_SIGN_ENABLE = n or y

   # Configuration for OTA encryption key with silabs_commander
   CONFIG_SIWX91X_OTA_ENCRYPT_KEY = "ota_encryption_key in hex format"

   # Configuration for OTA signing key with silabs_commander
   CONFIG_SIWX91X_OTA_SIGN_KEY = "ota_signing_key file path"

The ``silabs_commander`` tool will read the configuration file and use the specified
parameters to sign or MIC or encrypt the binary. The secured binary will be saved to the
specified output file. When using ``west sign``, the tool will automatically read the 
configuration file and use the specified parameters to sign or MIC or encrypt the binary.
The command can be run as follows:
.. code-block:: console

   west sign --tool silabs_commander or
   west build -b <board> example/path --pristine

This will invoke the ``silabs_commander`` tool with the specified configuration file and
sign or MIC or encrypt the binary according to the parameters defined in the configuration file.
.. note::
   The ``silabs_commander`` tool is specific to Silicon Labs devices and may not be
   applicable for other platforms. Ensure that you are using the correct signing tool
   for your target device.
.. note::
   The ``silabs_commander`` tool supports various options for security, such as MIC,
   encryption, and signing. You can specify these options in the configuration file
   to customize the signing process according to your requirements. Refer to the
   ``silabs_commander`` documentation for more details on the available options and
   how to use them effectively.
   "https://www.silabs.com/documents/public/user-guides/ug574-siwx917-soc-manufacturing-utility-user-guide.pdf"
.. _no-west:

Using Zephyr without west
#########################

There are many valid usecases to avoid using a meta-tool such as west which has
been custom-designed for a particular project and its requirements.
This section applies to Zephyr users who fall into one or more of these
categories:

- Already use a multi-repository management system (for example Google repo)

- Do not need additional functionality beyond compiling and linking

In order to obtain the Zephyr source code and build it without the help of
west you will need to manually clone the repositories listed in the
`manifest file`_ one by one, at the path indicated in it.

.. code-block:: console

   mkdir zephyrproject
   cd zephyrproject
   git clone https://github.com/zephyrproject-rtos/zephyr
   # clone additional repositories listed in the manifest file,
   # and check out the specified revisions

If you want to manage Zephyr's repositories without west but still need to
use west's additional functionality (flashing, debugging, etc.) it is possible
to do so by manually creating an installation:

.. code-block:: console

   # cd into zephyrproject if not already there
   git clone https://github.com/zephyrproject-rtos/west.git .west/west

Then create a file :file:`.west/config` with the following contents:

.. code-block:: none

   [manifest]
   path = zephyr

After that, and in order for ``ninja`` to be able to invoke ``west``
to flash and debug, you must specify the west directory. This can be
done by setting the environment variable ``WEST_DIR`` to point to
:file:`zephyrproject/.west/west` before running CMake to set up a
build directory.

.. _manifest file:
   https://github.com/zephyrproject-rtos/zephyr/blob/master/west.yml

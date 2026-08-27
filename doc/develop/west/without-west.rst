.. _no-west:

Using Zephyr without west
#########################

This page provides information on using Zephyr without west. This is
not recommended for beginners due to the extra effort involved. In
particular, you will have to do work "by hand" to replace these
features:

- cloning the additional source code repositories used by Zephyr in
  addition to the main zephyr repository, and keeping them up to date
- specifying the locations of these repositories to the Zephyr build
  system
- flashing and debugging without understanding detailed usage of the
  relevant host tools

.. note::

   If you have previously installed west and want to stop using it,
   uninstall it first:

   .. code-block:: console

      pip3 uninstall west

   Otherwise, Zephyr's build system will find it and may try to use
   it.

Getting the Source
------------------

In addition to downloading the zephyr source code repository itself,
you will need to manually clone the additional projects listed in the
:term:`west manifest` file inside that repository.

.. code-block:: console

   mkdir zephyrproject
   cd zephyrproject
   git clone https://github.com/zephyrproject-rtos/zephyr
   # clone additional repositories listed in zephyr/west.yml,
   # and check out the specified revisions as well.

As you pull changes in the zephyr repository, you will also need to
maintain those additional repositories, adding new ones as necessary
and keeping existing ones up to date at the latest revisions.

Building applications
---------------------

You can build a Zephyr application using CMake and Ninja (or make) directly
without west installed if you specify any modules manually.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :tool: cmake
   :goals: build
   :gen-args: -DZEPHYR_MODULES=module1;module2;...
   :compact:

When building with west installed, the Zephyr build system will use it to set
:ref:`ZEPHYR_MODULES <important-build-vars>`.

If you don't have west installed and your application does not need any of
these repositories, the build will still work.

If you don't have west installed and your application *does* need one
of these repositories, you must set :makevar:`ZEPHYR_MODULES`
yourself as shown above.

See :ref:`modules` for more details.

Similarly, if your application requires binary blobs and you are not using
west, you will need to download and place those blobs in the right places
instead of using ``west blobs``. See :ref:`bin-blobs` for more details.

Flashing and Debugging
----------------------

Running build system targets like ``ninja flash``, ``ninja debug``,
etc. is just a call to the corresponding :ref:`west command
<west-build-flash-debug>`. For example, ``ninja flash`` calls ``west
flash``\ [#wbninja]_. If you don't have west installed on your system, running
those targets will fail. You can of course still flash and debug using
any :ref:`flash-debug-host-tools` which work for your board (and which those
west commands wrap).

If you want to use these build system targets but do not want to
install west on your system using ``pip``, it is possible to do so
by manually creating a :term:`west workspace`:

.. code-block:: console

   # cd into zephyrproject if not already there
   git clone https://github.com/zephyrproject-rtos/west.git .west/west

Then create a file :file:`.west/config` with the following contents:

.. code-block:: none

   [manifest]
   path = zephyr

   [zephyr]
   base = zephyr

After that, and in order for ``ninja`` to be able to invoke ``west``
to flash and debug, you must specify the west directory. This can be
done by setting the environment variable ``WEST_DIR`` to point to
:file:`zephyrproject/.west/west` before running CMake to set up a
build directory.

.. rubric:: Footnotes

.. [#wbninja]

   Note that ``west build`` invokes ``ninja``, among other
   tools. There's no recursive invocation of either ``west`` or
   ``ninja`` involved by default, however, as ``west build`` does not
   invoke ``ninja flash``, ``debug``, etc. The one exception is if you
   specifically run one of these build system targets with a command
   line like ``west build -t flash``. In that case, west is run twice:
   once for ``west build``, and in a subprocess, again for ``west
   flash``. Even in this case, ``ninja`` is only run once, as ``ninja
   flash``. This is because these build system targets depend on an
   up to date build of the Zephyr application, so it's compiled before
   ``west flash`` is run.

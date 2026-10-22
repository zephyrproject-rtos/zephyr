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

Flashing and debugging are done with the ``west flash``, ``west debug``,
``west debugserver``, ``west attach`` and ``west rtt`` commands, which are
documented in :ref:`west-build-flash-debug`. These commands require west, so
they are not available if you are using Zephyr without it.

Without west, you can still flash and debug using any of the
:ref:`flash-debug-host-tools` which work for your board (and which those west
commands wrap), but you will have to invoke them yourself, with the right
options for your board and application.

.. _using-snippets:

Using Snippets
##############

.. tip::

   See :ref:`built-in-snippets` for a list of snippets that are provided by
   Zephyr.

Snippets have names. You use snippets by giving their names to the build
system.

With west build
***************

To use a snippet named ``foo`` when building an application ``app``:

.. code-block:: console

   west build -S foo app

To use multiple snippets:

.. code-block:: console

   west build -S snippet1 -S snippet2 [...] app

With cmake
**********

If you are running CMake directly instead of using ``west build``, use the
``SNIPPET`` variable. This is a whitespace- or semicolon-separated list of
snippet names you want to use. For example:

.. code-block:: console

   cmake -Sapp -Bbuild -DSNIPPET="snippet1;snippet2" [...]
   cmake --build build

Application required snippets
*****************************

If an application should always be compiled with a given snippet, it
can be added to that application's ``CMakeLists.txt`` file. For example:

.. code-block:: cmake

   if(NOT snippet1 IN_LIST SNIPPET)
     set(SNIPPET snippet1 ${SNIPPET} CACHE STRING "" FORCE)
   endif()

   find_package(Zephyr ....)

.. _application-local-snippets:

Application-local snippets
**************************

You can define snippets directly in your application's source directory by
creating a :file:`snippets/` subdirectory. These application-local snippets
are used the same way as any other snippet:

.. code-block:: console

   west build -S my-app-snippet app

Or with CMake:

.. code-block:: console

   cmake -Sapp -Bbuild -DSNIPPET="my-app-snippet" [...]

With sysbuild
=============

When using :ref:`sysbuild <sysbuild>` to build multiple images, application-local
snippets require the image-specific snippet variable. This is because app-local
snippets are only visible to the application that defines them, not to other
images in the build (such as a bootloader).

Use the image name as a prefix for the ``SNIPPET`` variable using west:

.. code-block:: console

   west build -b <board> --sysbuild app -- -Dapp_SNIPPET=my-app-snippet

Or with CMake:

.. code-block:: console

   cmake -Sapp -Bbuild -Dapp_SNIPPET="my-app-snippet" [...]

For example, if your application directory is ``my_app`` and you have a snippet
called ``debug-console`` in :file:`my_app/snippets/debug-console/snippet.yml`:

.. code-block:: console

   west build -b nrf52840dk/nrf52840 --sysbuild my_app -- -Dmy_app_SNIPPET=debug-console

.. note::

   Using the global ``-S`` or ``-DSNIPPET=`` syntax with application-local
   snippets in a sysbuild configuration will result in a build error, as the
   build system will attempt to apply the snippet to all images.

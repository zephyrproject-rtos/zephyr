.. _snippets:

Snippets
########

Snippets are a way to save build system settings in one place, and then use
those settings when you build a Zephyr application. This lets you save common
configuration separately when it applies to multiple different applications.

Some example use cases for snippets are:

- changing your board's console backend from a "real" UART to a USB CDC-ACM UART
- enabling frequently-used debugging options
- applying interrelated configuration settings to your "main" CPU and a
  co-processor core on an AMP SoC

Application-local snippets
**************************

In addition to snippets defined in Zephyr, modules, and directories specified
by :makevar:`SNIPPET_ROOT`, you can also define snippets in your application's
source directory. When using :ref:`sysbuild <sysbuild>` to build multiple images application-local
snippets require special handling. Application-local snippets are only available to the
application that defines them. To use application-local snippets with sysbuild, you must use the
image-specific snippet variable by prefixing the variable with the image name.
See :ref:`application-local-snippets` for more details.

The following pages document this feature.

.. toctree::
   :maxdepth: 1

   using.rst
   /snippets/index.rst
   writing.rst
   design.rst

.. _snippets:

Snippets
########

Snippets are a way to save build system settings in one place, and then use
those settings when you build any Zephyr application. This lets you save common
configuration separately when it applies to multiple different applications.

Some example use cases for snippets are:

- changing your board's console backend from a "real" UART to a USB CDC-ACM UART
- enabling frequently-used debugging options
- applying interrelated configuration settings to your "main" CPU and a
  co-processor core on an AMP SoC

The following pages document this feature.

.. toctree::
   :maxdepth: 1

   using.rst
   /snippets/index.rst
   writing.rst
   design.rst

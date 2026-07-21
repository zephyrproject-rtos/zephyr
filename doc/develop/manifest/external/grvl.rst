.. _external_module_grvl:

grvl
####

Introduction
************

`Graphics Rendering Visual Library`_ (grvl) is a lightweight GUI library for Zephyr-based MCUs,
providing a portable solution that is both suitable for resource-constrained devices and offers
a modern, responsive user experience.

Internally, grvl links to a standard XML library called `tinyxml`_ that is used for parsing the GUI
config instead of code. The UI interaction can be scripted with JavaScript using the integrated
`Duktape`_ JavaScript engine.

grvl features:

* Native support for PNG and JPEG graphics
* Simple DirectMedia Layer compatibility
* POSIX compliance and Zephyr support
* A collection of built-in components

  * Popups
  * Fonts
  * Labels
  * Buttons
  * Sliders

* User-defined prefabs that can be used to instantiate complex structures at runtime
* XML-based layout and a JavaScript engine

grvl is licensed under the Apache License 2.0.
tinyxml is licensed under the Zlib license.
Duktape is licensed under the MIT License.

Usage with Zephyr
*****************

To use grvl as a Zephyr :ref:`module <modules>`, add the following entry:

.. code-block:: yaml

   manifest:
     projects:
       - name: grvl
         url: https://github.com/antmicro/grvl
         revision: main
         path: modules/grvl # adjust the path as needed

to a Zephyr submanifest (e.g. ``zephyr/submanifests/grvl.yaml``) and run ``west update``, or add it
as a West project in your project's ``west.yaml`` manifest.

For more information, see the `grvl documentation`_ or the `grvl blog article`_.

You can also try an interactive `Zephyr calendar demo`_.

Reference
*********

.. target-notes::

.. _Graphics Rendering Visual Library:
   https://github.com/antmicro/grvl

.. _tinyxml:
   https://github.com/leethomason/tinyxml2

.. _Duktape:
   https://github.com/svaarala/duktape

.. _grvl documentation:
   https://antmicro.github.io/grvl/

.. _Zephyr calendar demo:
   https://github.com/antmicro/grvl-zephyr-calendar-demo

.. _grvl blog article:
  https://antmicro.com/blog/2025/12/grvl-a-lightweight-gui-library-for-zephyr-based-mcus

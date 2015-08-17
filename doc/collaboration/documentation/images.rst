.. _images:

Images
******

Images grab the reader's attention and convey information that
sometimes is difficult to explain using words alone. Well-planned
graphics can also reduce the amount of text required to explain
information. Readers who read in a second language rely more heavily
on graphics than those reading in their primary language because the
understanding they gain from the graphics helps them understand the
text.

Follow these guidelines when creating graphics for the |project|:

* Captions. Include a caption to explain or describe what the graphic
  illustrates or to use as a navigational tool when referring to the
  graphic from another location. All graphics should have a caption.

* Use cross references. Refer to your graphics in the main text flow.
  Create a label using the filename of the image. Use``:ref:`` to place
  the cross reference, see :ref:`internal_cross` for more details. Place
  the graphic immediately after its reference in the text flow or as
  close as possible.

* Keep graphics simple. They should only contain the information the
  reader needs.

* Use graphics judiciously. Don't use superfluous graphics and don't
  use graphics as mere decorations; they must have purpose. You don't
  need to show a screenshot of every single step or window in a software
  installation procedure, for example.

* Avoid volatility. Don't incorporate information into a graphic that
  might change with each release, for example, product versions or
  codename abbreviations.

* Use only approved formats. Use either PNG or JPEG bitmap files for
  screenshots and SVG files for vector graphics. Graphics that do not
  constitute photographs or screenshots must be provided as vector
  graphics to ensure that they can be changed.

* Provide the source files. Save the source artwork files with the
  documentation in a separate :file:`figures` folder. The filename must
  follow the naming conventions of the project, for example: :file:`
  fibers.svg` or :file:`nanokernel_fiber-1.png`.

Examples
========

These examples follow the guidelines and can be used used as reference.

The fiber context is represented in the diagram either as a box
containing different objects or a :ref:`symbol <fibers.svg>`.

.. _fibers.svg:

.. figure:: figures/fibers.svg
   :scale: 75 %
   :alt: Fibers Execution Context Symbol

   The graphic representation of the fibers execution context.

   This symbol is used to illustrate the actions performed by the
   abstract fibers execution context.

The :ref:`screenshot <nanokernel_fiber-1.png>` shows a comparison of
two file containing code. Observe how the differences are highlighted
and linked to make identifying them easier.

.. _nanokernel_fiber-1.png:

.. figure:: figures/nanokernel_fiber-1.png
   :scale: 75 %
   :alt: Nanokernel Code Comparison 2

   This screenshot shows a code comparison.

Templates
=========

Use this template to add a figure to your documentation according to
these guidelines.

.. code-block:: rst

    .. _file_name.ext:

    .. figure:: figures/file_name.ext
    :scale: 75%
    :alt: Alternative text.

    Brief caption detailing the contents of the image.

    Any additional explanation, description or actions depicted in the
   image.
    It can encompass multiple lines.
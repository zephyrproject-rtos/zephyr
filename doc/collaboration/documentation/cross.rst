.. _cross:

Cross References
****************

Sphinx and ReST provide different methods to create both internal and
external cross references. Use only the following methods to increase the
consistency of the documents.

.. _internal_cross:

Internal Cross References
=========================

An internal cross reference is a reference to a location within the Zephyr Project's
documentation. Use explicit markup labels and the ``:ref:`` markup to
create cross references to headings, figures, and code examples as needed.
All modules must have a label before their titles in order to be able to
cross reference them, see :ref:`modular` for more information regarding
modules.

The naming conventions for the labels are:

* Use only full words.

* Use \_ to link multiple words.

* Use only as many words as necessary to ensure that the label is unique.

These are some examples of proper labels:

.. code-block:: rst

   .. _getting_started:

   .. _gerrit_access:

   .. _building_zephyr_galileo:

Do not use labels like these:

.. code-block:: rst

   .. _QuickStart:

   .. _How to Gain Access to Gerrit:

   .. _building:

This is an internal reference to the beginning of :ref:`restructured`.

Observe that the ``:ref:`` markup is replaced with the title's text.
Similarly, it will be replaced with the figure's caption. If a different
text is needed the ``:ref:`` markup can still be used.

This is an internal reference to the beginning of
:ref:`this module <restructured>`.

Use the following templates to insert internal cross references properly.

.. code-block:: rst

   .. _label_of_target:

   This Is a Heading
   -----------------

   This creates a link to the :ref:`label_of_target` using the text of the
   heading.

   This creates a link to the :ref:`target <label_of_target>` using the word
   'target' instead of the heading.

The template renders as:

.. _label_of_target:

This Is a Heading
-----------------

This creates a link to the :ref:`label_of_target` using the text of the
heading.

This creates a link to the :ref:`target <label_of_target>` using the word
'target' instead of the heading.

.. important::

   This type of internal cross reference works across multiple files, is
   independent of changes in the text of the headings and works on all
   Sphinx builders that support cross references.

Referencing In-code Documentation
=================================

We have integrated in-code documentation using Sphinx and :program:`Breath`.
This integration allows us to cross reference functions, variables, macros
and types in any document. Use the following templates to insert a cross
reference to a documented code element.

.. code-block:: rst

   :c:func:`function_name()`

   :c:data:`varible`

   :c:macro:`macro_name`

   :c:type:`type_name`

.. caution::
   References to in-code documentation only work if the element has been
   documented in the code following the :ref:`contributing_code`.

External References
===================

External references or hyperlinks can be added easily with ReST. The allowed
methods are explicit hyperlinks and hyperlinks with a separated target
definition.

Explicit hyperlinks consist of writing the whole URL, for example:
http://sphinx-doc.org/rest.html#hyperlinks. Sphinx will recognize the URL
and create the link using the URL as label.

Hyperlinks with a separated target definition allow to replace the URL with
another label. They are easier to update and independent of the text, for
example:

`Gitg`_ is a great tool to visualize a GIT tree.

.. _Gitg: https://wiki.gnome.org/Apps/Gitg/

While both methods are accpeted, hyperlinks with a separated target
definition are preferred. Follow these guidelines when inserting hyperlinks:

* The labels for hyperlinks must be grammatically correct and unique within
  the module.

* Do not create labels for hyperlinks using: link, here, this, there, etc.

* Add all target definitions at the end of the section containing the
  hyperlinks.

Use this template to add a hyperlink with a separated definition:

.. code-block:: rst

   The state of `Oregon`_ offers a wide range of recreational activities.

   .. _Oregon: http://traveloregon.com/

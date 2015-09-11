.. _in-code:

In-Code Documentation
#####################

The in-code documentation is automatically extracted from the code. Doxygen
generates a huge XML tree in :file:`../../../xml` that the :program:`Breathe`
extension imports into Sphinx.

The Doxygen pass is independent of the Sphinx pass. Using Breathe to link them together, we can
reference the code in the documentation and vice-versa. For example, :cpp:type:`K_COMM` and
:cpp:class:`k_timer` have been modified to have additional information.

.. _doxygen_guides:

In-Code Documentation Guides
****************************

Follow these guides to document your code using comments. The |project|
follows the Javadoc Doxygen comments style. We provide examples on how to
comment different parts of the code. Files, functions, defines, structures,
variables and type definitions must be documented.

We have grouped the guides according to the object that is being
documented. Read sections carefully, as each object provides further
details as to how to document the code as a whole.

These simple rules apply to all the code that you wish to include in
the documentation:

#. Start and end a comment block with :literal:`/**` and :literal:`*/`

#. Start each line of the comment with :literal:` * `

#. Use \@ for all Doxygen special commands.

#. Files, functions, defines, structures, variables and type
   definitions must have a brief description.

#. All comments must start with a capital letter and end in a period.
   Even if the comment is a sentence fragment.

.. note::

   Always use :literal:`/**` This is a comment. :literal:`*/` if that
   comment should become part of the documentation.
   Use :literal:`/*` This comment won't appear in the documentation :
   literal:`*/` for comments that you want in the code but not in the
   documentation.

.. toctree::
   :maxdepth: 2


   files.rst
   functions.rst
   variables.rst
   defines.rst
   structs.rst
   typedefs.rst
   groups.rst
.. _in-code:

In-Code Documentation
#####################

Doxygen extracts the in-code documentation automatically from the code. Doxygen
generates XML files that the :program:`Breathe` extension imports into Sphinx.

The Doxygen pass is independent from the Sphinx pass. Using Breathe to link
them together, we can reference the code in the documentation and vice-versa.

.. _doxygen_guides:

In-Code Documentation Guides
****************************

Follow these guides to document your code using comments. The |project|
follows the Javadoc / Doxygen commenting style. We provide examples on how to
comment different parts of the code. Files, functions, defines, structures,
variables and type definitions must be documented.

We have grouped the guides according to the object that is being
documented. Read sections carefully, as each object provides further
details as to how to document the code as a whole.

These simple rules apply to all the code that you wish to include in
the documentation:

#. Start and end a comment block with :literal:`/**` and :literal:`*/`

#. Start each line of the comment with :literal:`*`

#. Use \@ for all Doxygen special commands.

#. Files, functions, defines, structures, variables and type
   definitions must have a brief description.

#. All comments must be in sentence-form: begin with a capital letter and
   end in a period, even when the comment is a sentence fragment.

.. note::
   Always use this syntax when your intention is to have that comment
   become part of the documentation.
   :literal:`/** This comment style will show up in docs. */`
   Alternatively, use the single-asterisk syntax when your intention
   is for that comment to appear in the code, but not in the documentation.
   :literal:`/* This style of comment won't appear in the docs */`

.. _Javadoc / Doxygen:
   http://www.stack.nl/~dimitri/doxygen/manual/docblocks.html

.. toctree::
   :maxdepth: 1

   files
   functions
   variables
   defines
   structs
   typedefs
   groups
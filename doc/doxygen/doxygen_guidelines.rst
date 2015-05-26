.. _In-Code Documentation Guidelines:

In-Code Documentation Guidelines
################################

Follow these guidelines to document your code using comments. We provide
examples on how to comment different parts of the code. Files,
functions, defines, structures, variables and type definitions must be
documented.

We have grouped the guidelines according to the object that is being
documented. Read the information regarding all elements carefully since
each type of object provides further details as to how to document the
code as a whole.

These simple rules apply to all the code that you wish to include in the
documentation:

#. Start and end a comment block with :literal:`/*!` and :literal:`*/`

#. Use \@ for all Doxygen special commands.

#. Files, functions, defines, structures, variables and type
   definitions must have a brief description.

#. All comments must start with a capital letter and end in a period.
   Even if the comment is a sentence fragment.

.. important::

   Always use :literal:`/*!` This is a comment. :literal:`*/` if that comment should
   become part of the documentation.
   Use :literal:`/*` This comment won't appear in the documentation :literal:`*/` for
   comments that you want in the code but not in the documentation.

.. toctree::
   :maxdepth: 2
   
   doxygen_guidelines_files.rst
   doxygen_guidelines_functions.rst
   doxygen_guidelines_variables.rst
   doxygen_guidelines_defines.rst
   doxygen_guidelines_structs.rst
   doxygen_guidelines_typedefs.rst
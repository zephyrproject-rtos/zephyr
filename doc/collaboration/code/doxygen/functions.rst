.. _functions:

Function Documentation
######################

Doxygen recognizes a wide variety of syntaxes and structures for the
function comments. The syntax described here is one of many that are possible.
If your development requires an option that is not described here, use it.
However, you may use the following syntax for everything else.

.. note::

   When linking functions within a ReST file, two possible markups are:
   ``:cpp:`` or ``:c:``. Use ``:cpp:`` for functions defined using an
   :code:`extern`. Use ``:c:`` for functions defined using a
   :code:`#define`.

Function Comment Templates
**************************

The full template shows the best practices for documenting a function.
The simplified version shows the minimal acceptable amount of
documentation for a function. Use the simplified template only if you
are familiar with Doxygen and how it uses blank lines to recognize the
parts of the comment.

Full template:

.. code-block:: c

   /**
    * @brief Short description of my_function().
    *
    * @details Longer multi-paragraph description.
    * Use this longer description to provide details about the
    * function's purpose, operation, limitations, etc.
    *
    * @param a This is the first parameter.
    * @param b This is the second parameter.
    *
    * @return Information about the return value.
    *
    * @error
    * Details about the possible error.
    *
    * @warning
    * This would be a warning.
    */
   my_function(int a, int b){}

Simplified template:

.. code-block:: c

   /**
    * Short description of my_function().
    *
    * Longer multi-paragraph description.
    * Use this longer description to provide details about the
    * function's purpose, operation, limitations, etc.
    *
    * @param a This is the first parameter.
    * @param b This is the second parameter.
    *
    * @return Information about the return value.
    */
   my_function(int a, int b){}

.. important::
   Ensure that you have **no** blank lines between the comment block
   and the function's signature. This ensures that Doxygen can link the comment
   to the function.

Function Documentation Examples
*******************************

Example 1
=========

Take the very simple function :c:func:`taskA()`:

.. literalinclude:: hello_wrong.c
   :language: c
   :lines: 1-12
   :emphasize-lines: 4-6, 10, 12
   :linenos:

The highlighted lines contain comments that will be missing from the generated
documentation. To avoid this, pay careful attention to your commenting syntax.
This example shows good commenting syntax.
:c:func:`taskA()` is:

.. literalinclude:: hello_commented.c
   :language: c
   :lines: 83-96
   :emphasize-lines: 5-7, 11, 13
   :linenos:

The highlighted lines show how to reference the code from within a
comment block. The direct reference is optional and the comments on
lines 11 and 13 are not added to the documentation. This method allows
for easy maintenance of the code blocks and easy addition of further
details. It also helps maintain the 72 character line length.

Example 2
=========
Take the more complex function hello_loop():

.. literalinclude:: hello_wrong.c
   :language: c
   :lines: 14-31
   :emphasize-lines: 1, 3-5, 7-9,13-16
   :linenos:

The function parameters have been documented using the correct Doxygen
command, yet notice line 1. The comment block was not started with
:literal:`/**` so, Doxygen will not parse it correctly.

The parameters have been documented using the \\param command. This is
equivalent to using @param but incorrect according to these guidelines.
Restructured Text uses the \\ as the escape for special characters.
To avoid possible conflicts, the \@ symbol must be used instead.

Notice that there is no blank line between the comment and the
function's signature, lines 7 and 8. This allows Doxygen to correctly
link the comment to the function.

Lines 13-16 contain comments that will not be included by Doxygen
in the documentation. To include that information, use the brief description
or the detailed description inside the comment block.
Remember that variables must be documented separately. See
:ref:`variables` for more details.

.. literalinclude:: hello_commented.c
   :language: c
   :lines: 58-81
   :emphasize-lines: 2, 4-7, 9-11, 19, 21
   :linenos:

Comment blocks must have the following structure:

#. Brief description. See line 2.

#. Detailed description. See lines 4-7.

#. Parameter information. See lines 9-11.

#. Return information. Return information is optional for void functions.

#. Other special commands. There is no specific order for any further special commands.

The description of the actions referenced in lines 19 and 21 is part of
the detailed description in the comment block. The references shown in
lines 19 and 21 are optional.

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

Workarounds
***********

When adding qualifiers to a function declaration, like *__deprecated*
(or *ALWAYS_INLINE*, or any others), for example:

.. code-block:: c

  /**
   * @brief Register a task IRQ object.
   *
   * This routine connects a task IRQ object to a system interrupt based
   * upon the specified IRQ and priority values.
   *
   * IRQ allocation is done via the microkernel server fiber, making simultaneous
   * allocation requests single-threaded.
   *
   * @param irq_obj IRQ object identifier.
   * @param irq Request IRQ.
   * @param priority Requested interrupt priority.
   * @param flags IRQ flags.
   *
   * @return assigned interrupt vector if successful, INVALID_VECTOR if not
   */
  uint32_t __deprecated task_irq_alloc(kirq_t irq_obj, uint32_t irq,
				     uint32_t priority, uint32_t flags);

the *Sphinx* parser can get confused with errors such as::

  doc/api/microkernel_api.rst:35: WARNING: Error when parsing function declaration.
  If the function has no return type:
    Error in declarator or parameters and qualifiers
    Invalid definition: Expecting "(" in parameters_and_qualifiers. [error at 9]
      uint32_t __deprecated task_irq_alloc(kirq_t irq_obj, uint32_t irq, uint32_t priority, uint32_t flags)
      ---------^
  If the function has a return type:
    Error in declarator or parameters and qualifiers
    If pointer to member declarator:
      Invalid definition: Expected '::' in pointer to member (function). [error at 22]
        uint32_t __deprecated task_irq_alloc(kirq_t irq_obj, uint32_t irq, uint32_t priority, uint32_t flags)
        ----------------------^
    If declarator-id:
      Invalid definition: Expecting "(" in parameters_and_qualifiers. [error at 22]
        uint32_t __deprecated task_irq_alloc(kirq_t irq_obj, uint32_t irq, uint32_t priority, uint32_t flags)
        ----------------------^
  ... <etc etc>...

a workaround is to name with *@fn* the function:

.. code-block:: c

  /**
   * @fn uint32_t task_irq_alloc(kirq_t irq_obj, uint32_t irq,
   *                             uint32_t priority, uint32_t flags)
   * @brief Register a task IRQ object.
   *
   * This routine connects a task IRQ object to a system interrupt based
   * upon the specified IRQ and priority values.
   *
   * IRQ allocation is done via the microkernel server fiber, making simultaneous
   * allocation requests single-threaded.
   *
   * @param irq_obj IRQ object identifier.
   * @param irq Request IRQ.
   * @param priority Requested interrupt priority.
   * @param flags IRQ flags.
   *
   * @return assigned interrupt vector if successful, INVALID_VECTOR if not
   */

This has been reported to the Sphinx developers
(https://github.com/sphinx-doc/sphinx/issues/2682).

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

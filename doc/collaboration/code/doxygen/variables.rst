.. _variable_documentation:

Variable Documentation
######################

Variables are the smallest element that requires documentation. As such
only a brief description is required detailing the purpose of the
variable. Only significant variables have to be documented. A
significant variable is a variable that adds functionality to a
component. Review the `Variable Documentation Examples`_ to understand
what constitutes a significant variable.

Variable Comment Template
*************************

.. code-block:: c

   /** Brief description of singificant_variable's purpose. */
   int significant_variable;

Variable Documentation Examples
*******************************

Example 1
=========

This example shows a function that has been fully documented following
the best practices.

.. literalinclude:: phil_fiber_commented.c
   :language: c
   :lines: 110-168
   :emphasize-lines: 15, 18, 21-23, 25, 31
   :linenos:

Lines 15 and 18 show the documentation for two variables.
The blank line 17 is necessary to increase the clarity
of the code and also so Doxygen can determine properly where
the comment belongs.

Lines 21-23 show another acceptable way to document two variables with a
similar function. Notice that only the first variable is documented.
The argument can be made that **kmutex_t f2** is no
longer a significant variable because it does not add any functionality
that has not been described for **kmutex_t f1**.

Lines 25 and 31 show us a different situation. Although both variables
are of the same type and very similar, they have different purposes.
Therefore, both must be documented and the difference between them
must be noted.

Example 2
=========
Variables outside of functions must be documented as well.

.. literalinclude:: hello_commented.c
   :language: c
   :lines: 133-140
   :emphasize-lines: 1, 4, 7
   :linenos:

As you can see, the syntax of the comment does not change. Always start
the comment with :literal:`/**` and end it with :literal:`*/`. Remember
to begin with a capital letter and end with a period, even if the
comment is only a sentence fragment.

Notice that the variable comments also apply for more complex types like
structs. The comments on lines 4 and 7 apply only to the specific
variable and not to the whole struct. Complex types must be documented
wherever they are defined. See :ref:`structure_documentation` and
:ref:`type_definition_documentation` for further details.
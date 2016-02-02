.. _defines:

Define Documentation
####################

Defines and functions are documented similarly. Some noteworthy
differences are:

* The best practice for defines requires the use of the **@def**
  special command.

* Just as with functions, we provide a full and a simplified template.
  The syntax used in the simplified template should only be used if you are familiar with
  Doxygen. Use the full template if you are having problems
  generating the documentation properly.

Define Comment Templates
************************

Full template:

.. code-block:: c

   /** @def name_of_define
     *
     * @brief Brief description of the define.
     *
     * @details Multiple lines describing in detail the
     * purpose of the define and what it does.
   */

   #define name_of_define

Simplified template:

.. code-block:: c

   /**
     * Brief description of the define.
     *
     * Multiple lines describing in detail the
     * purpose of the define and what it does.
   */
   #define name_of_define

Define Documentation Example
****************************

This simple example shows how to document a define with the least amount
of effort while still following best practices.

Correct:

.. literalinclude:: phil_commented.h
   :language: c
   :lines: 46-54
   :emphasize-lines: 2, 3, 5
   :linenos:

Observe how each piece of information is clearly marked. The @def on line 2
ensures that the comment is appropriately linked to the code.

Incorrect:

.. literalinclude::
   ../../../../samples/microkernel/apps/philosophers/src/phil.h

   :language: c
   :lines: 24-28
   :emphasize-lines: 2, 5
   :linenos:

Observe that the comment does not start with
:literal:`/**` and therefore Doxygen will ignore it.

The comment is ambiguous; it could apply to either the define or the #if.
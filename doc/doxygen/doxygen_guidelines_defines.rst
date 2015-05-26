.. _Define Documentation:

Define Documentation
####################

Defines are documented similarly as functions. There are some note
worthy differences:

* The best practice for defines requires the use of the **@def**
  special command.

* Just as with functions we provide a full and a simplified template.
  The simplified template is also accepted. The syntax used in the
  simplified template should only be used if you are familiar with
  Doxygen. If you are having problems getting the documentation to
  generate properly use the full template.

Define Comment Templates
************************

Full template:

.. code-block:: c

   /*! @def name_of_define

       @brief Brief description of the define.

       @details Multiple lines describing in detail what is the
       purpose of the define and what it does.
   */

   #define name_of_define

Simplified template:

.. code-block:: c

   /*! 
       Brief description of the define.

       Multiple lines describing in detail what is the
       purpose of the define and what it does.
   */
   #define name_of_define

Define Documentation Example
****************************

This simple example shows how to document a define with the least amount
of effort while still following the best practices.

Correct:

.. literalinclude:: phil_commented.h
   :language: c
   :lines: 46-54
   :emphasize-lines: 2, 3, 5
   :linenos:

Observe how each piece of information is clearly marked. There is no
confusion regarding to what part of the code the comment belongs thanks
to the @def on line 2.

Incorrect:

.. literalinclude:: ../../samples/microkernel/apps/philosophers/src/phil.h
   :language: c
   :lines: 42-47
   :emphasize-lines: 2, 5
   :linenos:

Observe that the comment does not start with
:literal:`/*!` and therefore Doxygen will ignore it.

The comment is ambiguous; it could apply to either the define or the #if.
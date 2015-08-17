.. _type_definition_documentation:

Type Definition Documentation
#############################

The documentation of type definitions, typedefs for short, is a simple
but tricky matter. Typedefs are aliases for other types and as such
need to be well documented. Always document typedefs even if the
complex type it uses is documented already.

Type Definition Comment Template.
*********************************

Typedefs require a simple comment explaining why they are being used and
what type they are referencing.

.. code-block:: c

   /** Brief description with the type that is being used and why. */
   typedef int t_ie;

No further explanation is needed regarding the type even if it is
complex. Each complex type must be documented where ever it was
defined. Doxygen connects the typedef and the complex type it uses
automatically.

Type Definition Documentation Example
*************************************

Correct:

.. literalinclude:: irq_test_common_commented.h
   :language: c
   :lines: 69-73
   :emphasize-lines: 1, 3, 4
   :linenos:

Lines 1 and 4 name the type that is being used and with what purpose.
Even if the purpose is the same, since the types are different, two
comments are needed. Leaving line 3 blank not only increases
readability but it also helps Doxygen link the comments to the typedefs
appropriately.

Incorrect:

.. literalinclude:: ../../../../samples/include/irq_test_common.h
   :language: c
   :lines: 67-72
   :emphasize-lines: 3, 4
   :linenos:

The comments on lines 3 and 4 offer little insight into the code's
behavior. Furthermore, they do not start with
:literal:`/**` and end with :literal:`*/`. Doxygen won't add the
information to the documentation nor link it properly to the complex
type documentation.
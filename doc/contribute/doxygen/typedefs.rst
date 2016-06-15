.. _typedefs:

Type Definition Documentation
#############################

The documentation of type definitions, typedefs for short, is a simple
yet tricky matter. Typedefs are aliases for other types and, as such,
need to be well documented. Always document typedefs even if the
complex type it uses is documented already.

Type Definition Comment Template
********************************

Typedefs require a simple comment explaining why they are being used and
what type they are referencing.

.. code-block:: c

   /** Brief description with the type that is being used and why. */
   typedef int t_ie;

No further explanation is needed regarding the type even if it is complex.
Each complex type must be documented whereever it was defined.
Doxygen connects the typedef and the complex type it uses automatically.

Type Definition Documentation Example
*************************************

Correct:

.. code-block:: c

   /** Declares a void-void function pointer to test the ISR. */
   typedef void (*vvfn)(void);

   /** Declares a void-void_pointer function pointer to test the ISR. */
   typedef void (*vvpfn)(void *);

Lines 1 and 4 name the type that is being used and with what purpose.
Even if the purpose is the same, since the types are different, two
comments are needed. Leaving line 3 blank not only increases
readability but it also helps Doxygen link the comments to the typedefs
appropriately.

Incorrect:

.. code-block:: c

   typedef void (*vvfn)(void);		/* void-void function pointer */
   typedef void (*vvpfn)(void *);	/* void-void_pointer function pointer */

The comments offer little insight into the code's behavior.
Furthermore, they do not start with :literal:`/**` and end with
:literal:`*/`. Doxygen won't add the information to the documentation
nor link it properly to the complex type documentation.

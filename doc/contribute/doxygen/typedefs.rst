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

Workarounds for function definitions
************************************

Some *typedefs* confuse the *Sphynx* / *breathe* parser; the construct:

.. code-block:: c

   /**
    * @brief Callback API upon firing of a trigger
    *
    * @param "struct device *dev" Pointer to the sensor device
    * @param "struct sensor_trigger *trigger" The trigger
    */
   typedef void (*sensor_trigger_handler_t)(struct device *dev,
                                            struct sensor_trigger *trigger);

might generate a warning such as::

  /home/e/inaky/z/kernel-oss.git/doc/api/io_interfaces.rst:70: WARNING: Error in type declaration.
  If typedef-like declaration:
    Type must be either just a name or a typedef-like declaration.
    If just a name:
      Error in declarator or parameters and qualifiers
      Invalid definition: Expected identifier in nested name, got keyword: int [error at 3]
        int(* sensor_trigger_set_t) (struct device *dev, const struct sensor_trigger *trig, sensor_trigger_handler_t handler)
        ---^
    If typedef-like declaration:
  ...

A workaround is to force the name of the typedef with the *@typedef*
construct:

.. code-block:: c

   /**
    * @typedef sensor_trigger_handler_t
    * @brief Callback API upon firing of a trigger
    *
    * @param "struct device *dev" Pointer to the sensor device
    * @param "struct sensor_trigger *trigger" The trigger
    */
   typedef void (*sensor_trigger_handler_t)(struct device *dev,
                                            struct sensor_trigger *trigger);

This issue has been reported and is awaiting a final fix
(https://github.com/michaeljones/breathe/issues/267).

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

.. _structure_documentation:

Structure Documentation
#######################

Structures, or structs for short, require very little documentation.
Structs must be documented wherever they are defined. Basically,
structs only require a brief description detailing why they are needed.
Each variable that composes a struct must be commented. A fully
simplified syntax is therefore appropriate.

.. note::

   Follow the same rules as struct when documenting an enum.
   Use the simplified syntax to add the brief description.

Structure Comments Template
***************************
Structs only have a simplified template:

.. literalinclude:: ex_struct_pre.c
   :language: c
   :lines: 1-11
   :emphasize-lines: 8
   :linenos:

Doxygen does not require any commands to recognize the different comments.
It does, however, require that line 8 be left blank.

Structure Documentation Example
*******************************

Correct:

.. literalinclude:: irq_test_common_commented.h
   :language: c
   :lines: 117-125
   :emphasize-lines: 6
   :linenos:

Make sure to start every comment with
:literal:`/**` and end it with :literal:`*/`. Every comment must start
with a capital letter and end with a period.

Doxygen requires that line 6 is left blank. Ensure a blank line
separates each group of variable and comment.

Incorrect:

.. literalinclude:: ../../../../samples/include/irq_test_common.h
   :language: c
   :lines: 112-115
   :emphasize-lines: 1
   :linenos:

The struct has no documentation. Developers that want to expand its
functionality have no way of understanding why the struct is defined
this way nor what its components are.
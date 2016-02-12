.. _structs:

Structure Documentation
#######################

Structures, or structs for short, require very little documentation,
and it's best to document them wherever they are defined.
Structs require only a brief description detailing why they are needed.
Each variable that composes a struct needs a comment contained within
the correct syntax. A fully simplified syntax is therefore appropriate.

.. note::

   Follow the same rules as struct when documenting an enum.
   Use the simplified syntax to add the brief description.

Structure Comments Template
***************************
Structs have a simplified template:

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
   :lines: 102-110
   :emphasize-lines: 6
   :linenos:

Make sure to start every comment with
:literal:`/**` and end it with :literal:`*/`. Every comment must start
with a capital letter and end with a period.

Doxygen requires that line 6 is left blank. Ensure a blank line
separates each group of variable and comment.

Incorrect:


.. literalinclude:: irq_test_common_wrong.h
   :language: c
   :lines: 1-7
   :emphasize-lines: 2
   :linenos:

The struct has no documentation. Developers that want to expand its
functionality have no way of understanding why the struct is defined
this way, nor what its components are.
.. _File Header Documentation:

File Header Documentation
#########################

Every .c, .h and .s file must contain a file header comment at the
beginning of the file. The file header must contain:

#. The filename. Use **@file** for Doxygen to auto-complete the
   filename.

#. The brief description: A single line summarizing the contents of
   the file. Use **@brief** to clearly mark the brief description.

#. The detailed description: One or multiple lines describing the
   purpose of the file, how it works and any other pertinent
   information such as copyrights, authors, etc.

.. note:: 

   Doxygen has special commands for copyrights (@copyright), authors
   (@author), and other important information. Please refer to the
   `Doxygen documentation`_ for further details.
   
.. _Doxygen documentation: http://www.stack.nl/~dimitri/doxygen/manual/index.html

Examples
********

Correct:

* A file header with a single line description.



.. literalinclude:: hello_commented.c
   :language: c
   :lines: 1-5
   :emphasize-lines: 1,2,4
   :linenos:

* A file header with a larger description.

.. literalinclude:: phil_task_commented.c
   :language: c
   :lines: 1-10
   :emphasize-lines: 5-8
   :linenos:

Incorrect:

A file header without a detailed description.

.. literalinclude:: phil_fiber_commented.c
   :language: c
   :lines: 1-3
   :emphasize-lines: 4
   :linenos:

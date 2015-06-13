The In-Code Documentation Generation Process
############################################

This is the documentation automatically extracted from the code. Doxygen
generates a huge XML tree in :file:`../xml` that the :program:`Breathe`
extension imports into Sphinx.

It is so huge though, that, when the full thing is put in, it can take
ten minutes and plenty of memory to run. For now, it only includes the
files in :file:`include/microkernel` and :file:`kernel/common`; other
folders can be added as needed.

The Doxygen pass is independent of the Sphinx pass, but the interesting
part of linking them toghether is that using Breathe, we can reference
the code in the documentation and viceversa.

For example, :cpp:type:`K_COMM`. :cpp:class:`k_timer` has been modified
to have more stuff, visit it's documentation.

This would be an example of referencing function :c:func:`__k_memcpy_s()`.

The guidelines for in-code documentation can be found in the collaboration
guidelines available here: :ref:`In-Code Documentation Guidelines`.

.. toctree::
   :maxdepth: 2

   doxygen_output.rst

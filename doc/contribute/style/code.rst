.. _general_code_style:

C Code and General Style Guidelines
###################################

Coding style is enforced on any new or modified code, but contributors are
not expected to correct the style on existing code that they are not
modifying.

For style aspects where the guidelines don't offer explicit guidance or
permit multiple valid ways to express something, contributors should follow
the style of existing code in the tree, with higher importance given to
"nearby" code (first look at the function, then the same file, then
subsystem, etc).

In general, follow the `Linux kernel coding style`_, with the following
exceptions and clarifications:

* Tabs are 8 characters.
* Use `snake case`_ for code and variables.
* The line length is 100 columns or fewer. In the documentation, longer lines
  for URL references are an allowed exception.
* Add braces to every ``if``, ``else``, ``do``, ``while``, ``for`` and
  ``switch`` body, even for single-line code blocks.
* Use spaces instead of tabs to align comments after declarations, as needed.
* Use C89-style single line comments, ``/*  */``. The C99-style single line
  comment, ``//``, is not allowed.
* Use ``/**  */`` for doxygen comments that need to appear in the documentation.
* Avoid using binary literals (constants starting with ``0b``).
* Avoid using non-ASCII symbols in code, unless it significantly improves
  clarity, avoid emojis in any case.
* Use proper capitalization of nouns in code comments (e.g. ``UART`` and not
  ``uart``, ``CMake`` and not ``cmake``).

.. _Linux kernel coding style:
   https://kernel.org/doc/html/latest/process/coding-style.html

.. _snake case:
   https://en.wikipedia.org/wiki/Snake_case

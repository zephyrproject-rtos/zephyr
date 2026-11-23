.. _python_style:

Python Style Guidelines
#######################

Python should be formatted in a `PEP 8`_ compliant manner. Zephyr uses the `ruff formatter`_
to achieve this. This opinionated formatter aims for consistency, generality, readability and
reducing git diffs.

To apply the formatter, run:

.. code-block:: shell

   ruff check --select I --fix <file> # Sort imports
   ruff format <file>

Ruff configuration
******************

A small set of options is applied on top of the defaults:

* Line length of 100 columns or fewer.
* Both single ``'`` and double ``"`` quote styles are allowed.
* Line endings will be converted to ``\n``. The default line ending on Unix.

Excluded files
**************

The formatter is enforced in CI, but only for newly added Python files, because the project already
had a large Python codebase when this was introduced.
The :zephyr_file:`.ruff-excludes.toml` file has a ``[format]`` section where all the files that are
currently excluded are listed. It is encouraged for contributors, when changing an excluded file,
to remove it from the list and format it in a separate commit.

.. _PEP 8:
   https://peps.python.org/pep-0008/

.. _ruff formatter:
   https://docs.astral.sh/ruff/formatter/

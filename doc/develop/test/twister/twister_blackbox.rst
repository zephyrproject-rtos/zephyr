.. _twister_blackbox:

Twister blackbox tests
######################

This guide aims to explain the structure of a test file so the reader will be able
to understand existing files and create their own. All developers should fix any tests
they break and create new ones when introducing new features, so this knowledge is
important for any Twister developer.

Basics
******

Twister blackbox tests are written in python, using the ``pytest`` library.
Read up on it :ref:`here <integration_with_pytest>` .
Auxiliary test data follows whichever format it was in originally.
Tests and data are wholly contained in the :zephyr_file:`scripts/tests/twister_blackbox`
directory and prepended with ``test_``.

Blackbox tests should not be aware of the internal twister code. Instead, they should
call twister as user would and check the results.

Sample test file
****************

.. literalinclude:: ./sample_blackbox_test.py
   :language: python
   :linenos:

Comparison with CLI
*******************

Test above runs the command

.. code-block:: console

    twister -i --outdir $OUTDIR -T $TEST_DATA/tests -y --level $LEVEL
    --test-config $TEST_DATA/test_config.yaml -p qemu_x86 -p frdm_k64f

It presumes a CLI with the ``zephyr-env.sh`` or ``zephyr-env.cmd`` already run.

Such a test provides us with all the outputs we typically expect of a Twister run thanks to
``importlib`` 's ``exec_module()`` [#f1]_ .
We can easily set up all flags that we expect from a Twister call via ``args`` variable [#f2]_ .
We can check the standard output or stderr in ``out`` and ``err`` variables.

Beside the standard outputs, we can also investigate the file outputs, normally placed in
``twister-out`` directories. Most of the time, we will use the ``out_path`` fixture in conjunction
with ``--outdir`` flag (L52) to keep test-generated files in temporary directories.
Typical files read in blackbox tests are ``testplan.json`` , ``twister.xml`` and ``twister.log`` .

Other functionalities
*********************

Decorators
==========

* ``@pytest.mark.usefixtures('clear_log')``
    - allows us to use ``clear_log`` fixture from ``conftest.py`` .
      The fixture is to become ``autouse`` in the future.
      After that, this decorator can be removed.
* ``@pytest.mark.parametrize('level, expected_tests', TESTDATA_X, ids=['smoke', 'acceptance'])``
    - this is an example of ``pytest`` 's test parametrization.
      Read up on it `here <https://docs.pytest.org/en/7.1.x/example/parametrize.html#different-options-for-test-ids>`__.
      TESTDATAs are most often declared as class fields.
* ``@mock.patch.object(TestPlan, 'TESTSUITE_FILENAME', suite_filename_mock)``
    - this decorator allows us to use only tests defined in the ``test_data`` and
      ignore the Zephyr testcases in the ``tests`` directory. **Note that all ``test_data``
      tests use** ``test_data.yaml`` **as a filename, not** ``testcase.yaml`` **!**
      Read up on the ``mock`` library
      `here <https://docs.python.org/3/library/unittest.mock.html>`__.

Fixtures
========

Blackbox tests use ``pytest`` 's fixtures, further reading on which is available
`here <https://docs.pytest.org/en/6.2.x/fixture.html>`__.

If you would like to add your own fixtures,
consider whether they will be used in just one test file, or in many.

* If in many, create such a fixture in the
  :zephyr_file:`scripts/tests/twister_blackbox/conftest.py` file.

    - :zephyr_file:`scripts/tests/twister_blackbox/conftest.py` already contains some fixtures -
      take a look there for an example.
* If in just one, declare it in that file.

    - Consider using class fields instead - look at TESTDATAs for an example.

How do I...
***********

Call Twister multiple times in one test?
========================================

Sometimes we want to test something that requires prior Twister use. ``--test-only``
flag would be a typical example, as it is to be coupled with previous ``--build-only``
Twister call. How should we approach that?

If we just call the ``importlib`` 's ``exec_module`` two times, we will experience log
duplication. ``twister.log`` will duplicate every line (triplicate if we call it three times, etc.)
instead of overwriting the log or appending to the end of it.

It is caused by the use of logger module variables in the Twister files.
Thus us executing the module again causes the loggers to have multiple handles.

To overcome this, between the calls you ought to use

.. code:: python

    capfd.readouterr()   # To remove output from the buffer
                         # Note that if you want output from all runs after each other,
                         # skip this line.
    clear_log_in_test()  # To remove log duplication


------

.. rubric:: Footnotes

.. [#f1] Take note of the ``setup_class()`` class function, which allows us to run
         ``twister`` python file as if it were called directly
         (bypassing the ``__name__ == '__main__'`` check).

.. [#f2] We advise you to keep the first section of ``args`` definition intact in almost all
         of your tests, as it is used for the common test setup.

.. _pluginFilterCreation:

Filter Creation
###############

Filter Interface
****************

The Plugin Filter Interface class offers a clear representation of what a filter should look like.
It implements four abstract methods, out of which one (either exclude or include) should always
be implemented. Although ``FilterInterface`` is effectively empty, since it doesn't contain
any logic, it should always be the superclass of each Filter instance. A filter could still 
function without throwing exceptions even if it doesn't extend Filter Interface, as long as it
implements every method of that class. This is exactly the reason why it would be better if
all Filters extended the interface: all the methods of the Interface are called on each Filter, so
having a blank implementation in the superclass allows the user to ignore the nonessential ones.

.. code-block:: python

    class FilterInterface:
        def setup(self, *args, **kwargs):
            """
            Initialises the Filter object. The arguments and keyword arguments given in the command
            will be passed to the Filters by this method.
            """

        def exclude(self, suite) -> bool:
            """
            Examines a given Test Suite and decides if it must be excluded from the test plan.

            Returns:
                bool: True, if the suite should be skipped
            """

        def include(self, suite) -> bool:
            """
            Examines a given Test Suite and decides if it must be included in the test plan.

            Returns:
                bool: True, if the suite should not be skipped
            """

        def teardown(self):
            """
            Simple hook to execute some code after the filter has been used
            """

Filter Class
************

Each Filter should be contained in its own .py file. As soon as this has been created,
it is essential to define a class named **Filter** that inherits from **FilterInterface**.
Each filter file must contain exactly one class with this specific name.
The requirement exists because the system's dynamic import mechanism looks precisely
for this label when seeking a desired Filter implementation.

This design choice, although strict and very limiting, simplifies the commands.
Having to specify the name of each Filter class would've made the
:ref:`plugin filter <_pluginFilterCommandsPG>` and
:ref:`plugin filter JSON String <_pluginFilterCommandsPGJS>` calls longer,
while only :ref:`plugin filter JSON Path <_pluginFilterCommandsPGJP>`
would have been unaffected.

It would have also been possible to implement a method that searched for a class
extending **FilterInterface**, but such an approach could have caused conflicts
in files where multiple classes inherited the interface.

.. note::

    If desired, the actual filter's name can still be changed by using a wrapper.
    This strategy would involve an empty class with the Filter name that inherits from the
    actual implementation, therefore allowing the original instance to have any desired name.

    Example:

    .. code-block:: python

        from plugin_filters.filter_interface import FilterInterface

        class MyFilterWithACustomName(FilterInterface):
            # Actual filter implementation
            ...

        class Filter(MyFilterWithACustomName):
            pass

Setup Method
************

After creating the Filter class, the next step should be the definition of ``setup(...)``.
This method will be fed the filter parameters entered in the twister call and is therefore
a crucial part of a filter's implementation. The method's parameters can be extended both with
positional and keyword arguments without any restrictions, although it is suggested to keep the
refecences to both ``*args`` and ``**kwargs``. Having these two elements will prevent exceptions
from being thrown on simple argument mistakes. More severe errors (for example forgetting a
positional argument) will still throw exceptions, although those will be caught quickly and
handled accordingly.

The method should mainly be used to assign values to the filter object (as seen in the
:ref:`implementation example <_pluginFilterExampleRegex>`), or also to execute some code
before the filtering process commences.

Exclude / Include Methods
*************************

The ``exclude(...)`` and ``include(...)`` are simple but important parts of the interface.
These two methods shall decide whether a filter will be executed or not.

The implementation depends entirely on the filter's goal, with the only constant
being its parameters: all methods overriding ``exclude(...)`` or ``include(...)`` will be given
exactly two: a reference to self and the TestSuite, whose exclusion / inclusion must be decided.

``exclude(...)`` and ``include(...)`` must always return a boolean. Their result will tell
the filter handler whether a filter must be excluded / included (return True) or left as it is
(return False).

.. note::

    Exclude will always be called before include is. This means that include, which was built to
    override decisions made by filters with lower priority, can override decisons taken by its
    own exclude method as well.

Teardown Method
***************

Teardown is a simple hook that allows some code to be executed once the filtering process has
finished running. Possible usages for this method go from a simple logging statement to closing
file handlers, network connections, etc. It receives only one parameter, a reference to self.

.. _pluginFilterExampleRegex:

Implementation Example
######################

The regex filter file contains the following code:

.. code-block:: python

    import re

    from plugin_filters.filter_interface import FilterInterface
    from twisterlib.testplan import TestSuite

    class Filter(FilterInterface):
        """
        The Filter ID Regex excludes Suites based on their ID.
        """
        regex_pattern: str = "*"

        # pylint: disable=arguments-differ
        def setup(self, regex_pattern: str, *args, **kwargs):
            self.regex_pattern = regex_pattern

        def exclude(self, suite: TestSuite):
            # Example: the regex_pattern "sample\\.bluetooth\\..*" would skip all but those test cases
            # from testcase.yaml or sample.yaml resp. the testsuites already collected in the testplan
            # that do not start with sample.bluetooth.
            return not bool(re.search(self.regex_pattern, suite.id))

As described in the previous sections, the class is called Filter and inherits FilterInterface.
It implements a setup method with a regex_pattern parameter, which is later assigned to the
object itself, making it accessible to the exclude message as well.

This filter also contains some short documentation and a few comments, since they can help other
users by explaining the inner workings of the class in detail.

The current implementation of the regex filter does not contain the include or teardown methods.
The teardown method is not necessary here, since there is nothing to clean up. On the other hand,
the include method could be a future extension of the Regex Filter, which could prevent lower
filters from excluding Suites with a specific ID. This feature is not yet implemented, however.

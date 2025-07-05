.. _pluginFilterCreation:

Filter Creation
###############

Filter Interface
****************

The filter interface class offers a clear representation of what a filter should look like.
It implements four abstract methods, out of which one (either exclude or include) should always
be implemented. The interface should always be the superclass of each Filter instance, even if it
contains a minimal amount of actual logic.  In fact, a filter can still function without throwing
exceptions even if it doesn't extend the Filter Interface, or at least it could, as long as it
implements every method of said class. This is exactly the reason why it would be better if
all Filters extended the interface: all the methods of the Interface are called on each Filter, so
having a blank implementation in the superclass allows the user to ignore the nonessential ones.

.. literalinclude:: ../../../../scripts/pylib/twister/plugin_filters/filter_interface.py
    :language: python
    :linenos:
    :caption: Filter Interface

Filter Class
************

Each Filter should be contained in its own .py file. As soon as this has been created,
it is essential to define a class named **Filter** that inherits from **FilterInterface**.
Each filter file must contain exactly one class with this specific name.
The requirement exists because the system's dynamic import mechanism looks precisely
for this label when seeking a desired Filter implementation.

This design choice, although strict and very limiting, simplifies the commands.
Having to specify the name of each Filter class would've made the
:ref:`plugin filter <pluginFilterCommandsPG>` and
:ref:`plugin filter JSON String <pluginFilterCommandsPGJS>` calls longer,
while only :ref:`plugin filter JSON Path <pluginFilterCommandsPGJP>`
would have been unaffected.

It would have also been possible to implement a method that searched for a class
extending **FilterInterface**, but such an approach could have caused conflicts
in files where multiple classes inherited the interface.

.. note::

    If desired, the actual filter's name can still be changed by using a wrapper.
    This strategy would involve an empty class with the Filter name inheriting the
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
:ref:`implementation example <pluginFilterExampleRegex>`), or also to execute some code
before the filtering process commences.

Setup is also the only method from the Filter Interface (currently) where the super class
implements some actual logic. Calling ``super().setup(*args, **kwargs)`` is *highly suggested*,
since it will initialize ``exclude_reason`` and ``include_reason``, two important variables
used in the logging process. Although their absence will not break the execution, these two
variables help users understand why a specific Test Suite was skipped. Therefore, it is
preferable not to leave them undefined.

Exclude / Include Methods
*************************

The ``exclude(...)`` and ``include(...)`` are simple but important parts of the interface.
These two methods shall decide whether a filter has to be executed or not.

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

The regex filter is a very simple implementation of a filter. Examining it can help understanding
the inner workings of a custom filter.

.. literalinclude:: ../../../../scripts/pylib/twister/plugin_filters/filters/filter_id_regex.py
    :language: python
    :linenos:
    :caption: Example class implementation

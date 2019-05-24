.. _code-documentation:

Code Documentation
###################

API Documentation
******************

Well documented APIs enhance the experience for developers and are an essential
requirement for defining an API's success. Doxygen is a general purpose
documentation tool that the zephyr project uses for documenting APIs. It
generates either an on-line documentation browser (in HTML) and/or provides
input for other tools that is used to generate a reference manual from
documented source files. In particular, doxygen's XML output is used as an input
when producing the Zephyr project's online documentation.

Reference to Requirements
**************************

APIs for the most part document the implementation of requirements or advertised
features and can be traced back to features. We use the API documentation as the
main interface to trace implementation back to documented features. This is done
using custom _doxygen_ tags that reference requirements maintained somewhere
else in a requirement catalogue.

Test Documentation
*******************

To help understand what each test does and which functionality it tests we also
document all test code using the same tools and in the same context and generate
documentation for all unit and integration tests maintained in the same
environment. Tests are documented using references to the APIs or functionality
they validate by creating a link back to the APIs and by adding a reference to
the original requirements.


Documentation Guidelines
*************************

Test Code
**********

The Zephyr project uses several test methodologies, the most common being the
:ref:`Ztest framework <test-framework>`. Test documentation should only be done
on the entry test functions (usually prefixed with test\_) and those that are
called directly by the Ztest framework. Those tests are going to appear in test
reports and using their name and identifier is the best way to identify them and
trace back to them to requirements.

Test documentation should not interfere with the actual API documentation and
needs to follow a new structure to avoid confusion. Using a consistent naming
scheme and following a well-defined structure we will be able to group this
documentation in its own module and identify it uniquely when parsing test data
for traceability reports. Here are a few guidelines to be followed:

- All test code documentation should be grouped under the ``all_tests`` doxygen
  group
- All test documentation should be under doxygen groups that are prefixed
  with tests\_

The doxygen ``@see`` directive is used to link features and APIs under test,
like so::


    /**
    * @brief Tests for the Semaphore kernel object
    * @defgroup kernel_semaphore_tests Semaphore
    * @ingroup all_tests
    * @{
    */

    ...
    /**
    * @brief A brief description of the tests
    * Some details about the test
    * more details
    *
    * @see k_sem_init(), #K_SEM_DEFINE(x)
    */
    void test_sema_thread2thread(void)
    {
    ...
    }
    ...

    /**
    * @}
    */

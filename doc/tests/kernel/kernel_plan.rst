.. _kernel_plan:

Zephyr Kernel Verification Plan
###############################

Zephyr Test Overview
********************

To ensure robustness of the Zephyr Kernel, the Zephyr source code incorporates
test cases using the Ztest framework. Zephyr verification mainly focuses on
API correctness and feature completeness. Zephyr kernel testing uses different
verification levels depending on need, as described below.

* **API verification**

    * Verify kernel objects initialization in all supported ways:

        * K\_\*\_DEFINE (example: K\_THREAD\_STACK\_DEFINE, K\_STACK\_DEFINE)
        * K\_\*\_INITIALIZER (example: \_K\_WORK\_INITIALIZER)
        * k\_\*\_init (example: k\_sem\_init, k\_mutex\_init)

    * Verify usages mentioned in Zephyr kernel API documentation:
      :ref:`kernel_apis`

    * Verify every API’s functionality with its typical parameters defined in
      zephyr/kernel/include.

    * Verify API invocation from each callable context (ISR, cooperative
      thread and preemptive thread).

    * Verify possible API calling sequence and/or normal state transition.

    * API sequence test is to verify a kernel feature by calling its APIs
      in various sequences. As an example of LIFO test:

        * Init, put, put, get, get
        * Init, get, get, put, put

    * State transition test is to verify the kernel feature by calling its
      APIs that triggering internal state change.
      As an example of a thread’s life cycle:

        * Spawn, delayed start, suspend, resume, and abort

    * The test will cover only expected sequence and normal transition.
      The test does not take priority to traverse kernel APIs with
      unexpected sequence or abnormal transition.

* **Concept verification**

    * Verify testable statements mentioned in kernel documentation
      ("Concept" section, example: :ref:`lifecycle_v2`)

    * Usually these are behavioral description about kernel service or
      feature.

* **Configure Option verification**

    * Validate Kconfig and device tree configuration’s runtime effect.

* **Threadsafe verification**

    * Verify thread safety of kobject’s implementation in multi-threaded
      environment. Example: Usage of a memory slab by multiple threads.

Kernel Test Structure
*********************

Test cases are located at zephyr/tests/kernel and are organized
in two levels:

* Test Suite
    Corresponds to one single kernel service or feature. Test suite is named
    after the service name or the feature name. A test suite contains multiple
    test cases. Test suite is run through Ztest. Example: mpool_api.

* Test Case
    Corresponds to unit tests which are subsets of test suite.
    Example: test_mpool_alloc_free_thread.

Each test suite follows a common way to organize and name
test cases, with self-indication of test coverage. In the descriptions
below, the <component> refers to single kernel service or feature,
such as mutex, semaphore, alert, or thread.

* <component>_api

   * Verify kernel objects initialization in all means.

   * Verify usage mentioned in kernel documentation.

   * Verify every API in header file, functioning with its parameters.

   * Verify API invocation from each callable context.
     Example: mempool_api, memslab_api

* <component>_concept

   * Verify testable statements mentioned in kernel documentation.

   * Verify testable scenarios derived from RFC.
     Example: mheap_concept, tickless_concept

* <component>

   * Validate the functionality of kernel object or service in general.
     Example: xip, pend

* <component>_threadsafe

   * Verify API thread safe in multi-thread environment.
     Example: mslab_threadsafe


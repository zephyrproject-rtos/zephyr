.. _pigweed_cpp_support:

Enabling Additional C++ Features with Pigweed
#############################################

Additional C++ features that are currently not natively supported by Zephyr are
provided through a 3rd party module: `Pigweed`_. Pigweed provides some standard
library functionality like:

.. list-table:: Pigweed support
   :header-rows: 1

   * - Pigweed module
     - Description
   * - `pw::thread`_
     - Provides C++ features to :c:struct:`k_thread` and functionality
       provided by :ref:`threads_v2`
   * - `pw::sync`_
     - Provides C++ features to :c:struct:`k_mutex` and :c:struct:`k_sem`
       along with functionality provided by :ref:`mutexes_v2` and
       :ref:`semaphores_v2` respectively. Features such as support for
       ``std::lock_guard`` (:ref:`pigweed_example_lock_guard`).
   * - `pw::chrono`_
     - Provides C++ features to :ref:`kernel_timing` and :ref:`timers_v2` along
       with some STL chrono features such as the ability to cast, operate, or
       convert different time units with fewer errors.

Navigating Pigweed
******************

Pigweed uses a different definition for what a ``module`` is when compared to
Zephyr. When reviewing the documentation on `Pigweed`_'s page, a ``module`` can
be viewed as a collection of related libraries.

.. _pigweed_example_lock_guard:

Example: lock guard
*******************

.. code-block:: C++

   #include <mutex>

   #include <pw_sync/mutex.h>

   pw::sync::Mutex mutex;

   void ThreadSafeCriticalSection() {
     std::lock_guard lock(mutex);
     NotThreadSafeCriticalSection();
   }

Getting started
###############

Step 1: Add Pigweed to your Zephyr manifest
*******************************************

.. code-block:: yaml

   remotes:
     - name: pigweed
       url-base: https://pigweed.googlesource.com/pigweed

   projects:
     - name: pigweed
       remote: pigweed
       revision: main

Step 2: Bootstrap the Pigweed environment
*****************************************

.. warning::

    If you've already set up a virtual environment in
    :ref:`install_py_requirements` you'll need to exit and delete it first.
    Pigweed will manage the virtual environment for you. Trying to bootstrap
    from inside a virtual environment will lead to undefined behavior.

Bootstrapping the Pigweed environment is effectively like Zephyr's installation
of the ``requirements.txt`` file. After you bootstrap Pigweed, there will be a
virtual environment set up with all the Pigweed tools. Then, you install the
Zephyr requirements on top.

.. code-block:: shell

   cd ~/zephyrproject
   source modules/lib/pigweed/bootstrap.sh
   pip install zephyr/scripts/requirements.txt

Once installed, the environment is set up. In future work sessions, run the
following command to enter your virtual environment:

.. code-block:: shell

   source modules/lib/pigweed/activate.sh

Updating
=========

You should re-run the bootstrapping process following any ``west update`` that
modifies either the Zephyr main tree or the Pigweed module.

Step 3: Use Pigweed
*******************

Pigweed modules can be enabled via Kconfigs and automatically link into your
application. The configurations and modules can be seen at
`Pigweed Zephyr Kconfig reference`_.

.. _`Pigweed`: https://pigweed.dev/
.. _`pw::thread`: https://pigweed.dev/pw_thread
.. _`pw::sync`: https://pigweed.dev/pw_sync
.. _`pw::chrono`: https://pigweed.dev/pw_chrono
.. _`Pigweed Zephyr Kconfig reference`: https://pigweed.dev/docs/os/zephyr/kconfig.html

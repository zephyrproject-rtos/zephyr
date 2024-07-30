.. _sparse:

Sparse support
##############

`Sparse <https://www.kernel.org/doc/html/latest/dev-tools/sparse.html>`__
is a static code analysis tool.
Apart from performing common code analysis tasks it also supports an
``address_space`` attribute, which allows introduction of distinct address
spaces in C code with subsequent verification that pointers to different
address spaces do not get confused. Additionally it supports a ``force``
attribute which should be used to cast pointers between different address
spaces. At the moment Zephyr introduces a single custom address space
``__cache`` used to identify pointers from the cached address range on the
Xtensa architecture. This helps identify cases where cached and uncached
addresses are confused.

Running with sparse
*******************

To run a sparse verification build :ref:`west build <west-building>` should be
called with a ``-DZEPHYR_SCA_VARIANT=sparse`` parameter, e.g.

.. code-block:: shell

    west build -d hello -b intel_adsp/cavs25 zephyr/samples/hello_world -- -DZEPHYR_SCA_VARIANT=sparse

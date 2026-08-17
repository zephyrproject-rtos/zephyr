.. _requirements_catalog:

Requirements Catalog
####################

The Zephyr requirements are maintained in the dedicated ``reqmgmt`` repository
using `StrictDoc <https://github.com/strictdoc-project/strictdoc>`__. When that
repository is present in the workspace (it is pulled in as a west project), its
requirements are exported and rendered directly into this documentation below.

.. only:: reqmgmt

   .. toctree::
      :maxdepth: 2

      /build/requirements/index

.. only:: not reqmgmt

   The requirements are not included in this build because the ``reqmgmt``
   module is not present in the workspace. See the
   `Zephyr Project Requirements <https://zephyrproject-rtos.github.io/reqmgmt/>`__
   for the published version.

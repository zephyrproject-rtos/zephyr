..
    Zephyr Project documentation main file

.. _zephyr-home:

Zephyr Project Documentation
############################

.. raw:: html

   <script>
     function openVersionSelector() {
       // Open the mobile menu if visible
       var mobileMenu = document.querySelector('[data-toggle="wy-nav-top"]');
       if (mobileMenu && mobileMenu.offsetParent !== null) {
         mobileMenu.click();
       }
       // Open the version selector
       var versionSelector = document.querySelector('[data-toggle="rst-current-version"]');
       if (versionSelector) {
         versionSelector.click();
       }
     }
   </script>

.. only:: release

   .. admonition:: Welcome to Zephyr Project Documentation for version |version|.
      :class: welcome

      .. raw:: html

         <p>
           Use the <a href="#" onclick="openVersionSelector(); return false;">version selector</a>
           for the documentation of other Zephyr versions.
         </p>

.. only:: development

   .. admonition:: Welcome to Zephyr Project Documentation for the ``main`` tree (|version|).
      :class: welcome

      .. raw:: html

         <p>
           Use the <a href="#" onclick="openVersionSelector(); return false;">version selector</a>
           for the documentation of previously released versions.
         </p>

.. raw:: html
   :file: index.html

.. toctree::
   :maxdepth: 1
   :hidden:

   introduction/index.rst
   develop/index.rst
   kernel/index.rst
   services/index.rst
   build/index.rst
   connectivity/index.rst
   hardware/index.rst
   contribute/index.rst
   project/index.rst
   security/index.rst
   safety/index.rst
   samples/index.rst
   boards/index.rst
   releases/index.rst

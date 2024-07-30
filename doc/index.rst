..
    Zephyr Project documentation main file

.. _zephyr-home:

Zephyr Project Documentation
############################

.. only:: release

   Welcome to the Zephyr Project's documentation for version |version|.

   Documentation for the latest (main) development branch of Zephyr
   can be found at https://docs.zephyrproject.org/

.. only:: (development or daily)

   **Welcome to the Zephyr Project's documentation
   for the main tree under development** (version |version|).

Use the version selection menu on the left to view
documentation for a specific version of Zephyr.

For information about the changes and additions for releases, please
consult the published :ref:`zephyr_release_notes` documentation.

The Zephyr OS is provided under the `Apache 2.0 license`_ (as found in
the LICENSE file in the project's `GitHub repo`_).  The Zephyr OS also
imports or reuses packages, scripts, and other files that use other
licensing, as described in :ref:`Zephyr_Licensing`.


.. raw:: html

   <ul class="grid">
       <li class="grid-item">
	   <a href="introduction/index.html">
	       <img alt="" src="_static/images/kite.png"/>
	       <h2>Introduction</h2>
	   </a>
	   <p>Introducing the Zephyr Project: overview, architecture, features, and licensing</p>
       </li>
       <li class="grid-item">
	   <a href="develop/getting_started/index.html">
               <span class="grid-icon fa fa-map-signs"></span>
	       <h2>Getting Started Guide</h2>
	   </a>
	   <p>Follow this guide to set up a Zephyr development environment on your
	       system, and then build and run a sample application.</p>
       </li>
       <li class="grid-item">
	   <a href="contribute/index.html">
               <span class="grid-icon fa fa-github"></span>
	       <h2>Contribution Guidelines</h2>
	   </a>
	   <p>As an open-source project, we welcome and encourage the community
           to submit patches directly to the project.</p>
       </li>
       <li class="grid-item">
	   <a href="samples/index.html">
               <span class="grid-icon fa fa-cogs"></span>
	       <h2>Samples and Demos</h2>
	   </a>
	   <p>A list of samples and demos that can run on a variety of boards supported
	       by Zephyr</p>
       </li>
       <li class="grid-item">
	   <a href="hardware/index.html">
               <span class="grid-icon fa fa-sign-in"></span>
	       <h2>Hardware Support</h2>
	   </a>
	   <p>Information about supported architectures, supported hardware and porting guides</p>
       </li>
       <li class="grid-item">
	   <a href="security/index.html">
               <span class="grid-icon fa fa-lock"></span>
	       <h2>Security</h2>
	   </a>
	   <p>Requirements, processes, and developer guidelines for ensuring security is addressed within the Zephyr project.</p>
       </li>
       <li class="grid-item">
	   <a href="boards/index.html">
               <span class="grid-icon fa fa-object-group"></span>
	       <h2>Supported Boards</h2>
	   </a>
	   <p>List of supported boards and platforms.</p>
       </li>
       <li class="grid-item">
	   <a href="services/index.html">
               <span class="grid-icon fa fa-puzzle-piece"></span>
	       <h2>OS Services</h2>
	   </a>
	   <p>OS Services and guides how to use them with Zephyr</p>
       </li>
   </ul>

Sections
********

.. toctree::
   :maxdepth: 1
   :caption: Contents

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

Indices and Tables
******************

* :ref:`glossary`
* :ref:`genindex`

.. _Apache 2.0 license:
   https://github.com/zephyrproject-rtos/zephyr/blob/main/LICENSE

.. _GitHub repo: https://github.com/zephyrproject-rtos/zephyr

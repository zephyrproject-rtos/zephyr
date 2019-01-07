..
    Zephyr Project documentation master file

Zephyr Project Documentation
############################

.. only:: release

   Welcome to the Zephyr Project's documentation version |version|!

   Documentation for the development branch of Zephyr can be found at
   https://docs.zephyrproject.org/

.. only:: (development or daily)

   Welcome to the Zephyr Project's documentation. This is the documentation of the
   master tree under development (version |version|).

For information about the changes and additions for releases, please
consult the published :ref:`zephyr_release_notes` documentation.

The Zephyr OS is provided under the `Apache 2.0 license`_ (as found in
the LICENSE file in the project's `GitHub repo`_).  The Zephyr OS also
imports or reuses packages, scripts, and other files that use other
licensing, as described in :ref:`Zephyr_Licensing`.

.. _Apache 2.0 license:
   https://github.com/zephyrproject-rtos/zephyr/blob/master/LICENSE

.. _GitHub repo: https://github.com/zephyrproject-rtos/zephyr


.. raw:: html

   <ul class="grid">
       <li class="grid-item">
	   <a href="introduction/introducing_zephyr.html">
	       <img alt="" src="_static/images/kite.png"/>
	       <h2>Introduction</h2>
	   </a>
	   <p>Introducing the Zephyr Project: the overview, architecture, features and licensing</p>
       </li>
       <li class="grid-item">
	   <a href="getting_started/getting_started.html">
	       <img alt="" src=""/>
	       <h2>Getting Started Guide</h2>
	   </a>
	   <p>Follow this guide to set up a Zephyr development environment on your
	       system, and then build and run a sample application.</p>
       </li>
       <li class="grid-item">
	   <a href="contribute/index.html">
	       <img alt="" src=""/>
	       <h2>Contribution Guidelines</h2>
	   </a>
	   <p>As an open-source project, we welcome and encourage the community
           to submit patches directly to the project.</p>
       </li>
       <li class="grid-item">
	   <a href="samples/samples.html">
	       <img alt="" src=""/>
	       <h2>Samples and Demos</h2>
	   </a>
	   <p>A list of samples and demos that can run on a variety of boards supported
	       by Zephyr</p>
       </li>
       <li class="grid-item">
	   <a href="kernel/kernel.html">
	       <img alt="" src=""/>
	       <h2>Kernel Services</h2>
	   </a>
	   <p>General introduction of the Zephyr kernel’s key capabilities and services.</p>
       </li>
       <li class="grid-item">
	   <a href="security/security.html">
	       <img alt="" src=""/>
	       <h2>Security</h2>
	   </a>
	   <p>Requirements, processes, and developer guidelines for ensuring security is addressed within the Zephyr project.</p>
       </li>
       <li class="grid-item">
	   <a href="boards/boards.html">
	       <img alt="" src=""/>
	       <h2>Supported Boards</h2>
	   </a>
	   <p>List if supported boards and platforms.</p>
       </li>
       <li class="grid-item">
	   <a href="tools/index.html">
	       <img alt="" src=""/>
	       <h2>Tools</h2>
	   </a>
	   <p>List of Tools used for development.</p>
       </li>
   </ul>

.. only:: html

   Sections
   ********

.. toctree::
   :maxdepth: 1

   introduction/introducing_zephyr.rst
   getting_started/getting_started.rst
   contribute/index.rst
   application/application.rst
   kernel/kernel.rst
   security/security.rst
   subsystems/subsystems.rst
   devices/index.rst
   tools/index.rst
   porting/index.rst
   documentation/index.rst
   samples/samples.rst
   api/index.rst
   boards/boards.rst
   reference/index.rst

.. only:: html

   Indices and Tables
   ******************

* :ref:`glossary`

* :ref:`genindex`

.. _Zephyr 1.13.0: https://docs.zephyrproject.org/1.13.0/
.. _Zephyr 1.12.0: https://docs.zephyrproject.org/1.12.0/
.. _Zephyr 1.11.0: https://docs.zephyrproject.org/1.11.0/
.. _Zephyr 1.10.0: https://docs.zephyrproject.org/1.10.0/
.. _Zephyr 1.9.2: https://docs.zephyrproject.org/1.9.0/

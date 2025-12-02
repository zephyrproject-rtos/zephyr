.. _getting_started_with_safety_requirements:

Getting started with Requirements Management
############################################

Zephyr's requirements management is powered by `StrictDoc <https://github.com/strictdoc-project/strictdoc>`_, a lightweight tool for writing, organizing, and publishing structured requirements.
This section explains how to set up and use the toolchain provided in the `reqmgmt <https://github.com/zephyrproject-rtos/reqmgmt>`_ repository.

Overview
*********

The ``reqmgmt`` repository is Zephyr's official requirements workspace. It supports:

- Authoring system and software requirements
- Linking requirements to verification artifacts
- Generating browsable HTML documentation
- Running a local web interface for editing and review

Installation
************

StrictDoc requires Python 3.7 or newer. To install (you might need to use pip3 instead of pip):

.. code-block:: bash

   pip install strictdoc

Clone the Zephyr requirements repository:

.. code-block:: bash

   git clone https://github.com/zephyrproject-rtos/reqmgmt
   cd reqmgmt

Usage
******

To export the requirements to HTML:

.. code-block:: bash

   strictdoc export .

This generates a static site in the :file:`output/` directory.

To launch the local web interface:

.. code-block:: bash

   strictdoc server .

You should see something like

.. code-block:: bash

   Uvicorn running on http://127.0.0.1:5111 (Press CTRL+C to quit)


Bring up your local browser and paste in the address above to access the browser-based editor
for navigating and editing requirements.


Structure
*********

The repository contains:

- :file:`docs/` — Requirements documents organized by subsystem
- :file:`strictdoc.toml` — Configuration file for StrictDoc
- :file:`tasks.py` — Automation helpers for building and validating requirements

Live Documentation
******************

You can view the published requirements online at:

`https://zephyrproject-rtos.github.io/reqmgmt <https://zephyrproject-rtos.github.io/reqmgmt>`_

Next Steps
**********

- Explore the :file:`docs/` folder to see example requirement files
- Use the web interface to create or edit your own
- Link requirements to test cases or source modules for traceability

``StrictDoc`` helps Zephyr move toward safety-critical readiness by formalizing the connection between system goals and software implementation.

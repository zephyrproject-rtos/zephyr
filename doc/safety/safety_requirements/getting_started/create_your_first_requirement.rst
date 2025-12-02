.. _create_first_safety_requirements:

Create your first Zephyr RTOS requirement
*****************************************
Repository Overview
-------------------

The Zephyr requirements are managed in the ``reqmgmt`` repository using [StrictDoc](https://github.com/strictdoc-project/strictdoc). The repo includes:

- :file:`docs/`: Contains requirement documents in StrictDoc format
- :file:`tools/`: Utility scripts and configuration
- :file:`strictdoc.toml`: Project configuration for StrictDoc
- :file:`tasks.py`: Task automation using Invoke


Step 1: Create or Edit a Requirement File
-----------------------------------------

Navigate to the :file:`docs/` folder in the repository.

Folder Overview
~~~~~~~~~~~~~~~

The :file:`docs/` folder contains all requirement documents written in StrictDoc format.

The :file:`docs/` folder contains two subfolders. These are organized as follows:

- :file:`system_requirements/`
  Contains high-level system requirements that describe the overall goals, constraints, and expected behavior of the Zephyr RTOS.

  - :file:`system_requirements.sgra`
    Contains the GRAMMAR, that defines the formal structure of a requirement
  - :file:`index.sdoc` that contains the actual requirements statements

- :file:`software_requirements/`
  Contains component-level requirements. Each file in this folder corresponds to a specific subsystem or module.
  - :file:`software_requirements.sgra` contains the GRAMMAR for all software requirements
  - a list of :file:`.sdoc` snippet files that compile together to a big software requirements document ,e.g.

    - :file:`interrupts.sdoc`
    - :file:`queues.sdoc`
    - :file:`semaphore.sdoc`
    - :file:`threads.sdoc`

These files are modular and allow contributors to work independently on specific areas of the system.
You can either edit an existing :file:`.sdoc` file or create a new one. Here's a basic example of a requirement block:

.. code-block:: text

   [REQUIREMENT]
   UID: ZEP-SRS-17-1
   STATUS: Draft
   TYPE: Functional
   COMPONENT: File System
   TITLE: Create file
   STATEMENT: >>>
   Zephyr shall provide file create capabilities for files on the file system.
   <<<

If you set the UID to ``TBD`` so StrictDoc can generate it automatically.

Step 2: Save the File
---------------------

Save your file in the systems or software requirements subfolder in :file:`docs/` folder.
If you create a new file give it a meaningful name, usually referring to the component its content is
meant for or the functionality.

Step 3: Assign a UID Automatically
----------------------------------

From the root of the repository, run the following command:

.. code-block:: bash

   strictdoc manage auto-uid .

This command will:
- Traverse the project directory
- Find requirements with ``UID: TBD``
- Assign a unique identifier to each one

Step 4: Verify the UID Assignment
---------------------------------

Open the file again. You should now see something like:

.. code-block:: text

   UID: ZEP-SRS-17-2
   STATUS: Draft
   TYPE: Functional
   COMPONENT: File System
   TITLE: Create file
   STATEMENT: >>>
   Zephyr shall provide file create capabilities for files on the file system.
   <<<

Note: The UID format can be configured in :file:`strictdoc.toml`.

Optional: Generate HTML Documentation
-------------------------------------

To build the HTML version of the requirements documentation:

.. code-block:: bash

   strictdoc export .

This will generate browsable documentation in the :file:`output/` folder.

Optional: Launch the Web Editor
-------------------------------

To interactively browse and edit requirements:

.. code-block:: bash

   strictdoc server .

This launches a local web interface for editing and reviewing requirements.

Summary
-------

- Create or edit a requirement in :file:`docs/` with ``UID: TBD``
- Run ``strictdoc manage auto-uid .`` to assign UIDs
- Optionally generate HTML docs or launch the web editor

StrictDoc helps Zephyr maintain structured, traceable, and editable requirements across platforms.

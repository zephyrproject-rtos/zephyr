===============================
Display capture Twister harness
===============================

Configuration example
---------------------

.. code-block:: console

   case_config:
     device_id: 0  # Try different camera indices
     res_x: 1280   # x resolution
     res_y: 720    # y resolution
     fps: 30       # analysis frame pre-second
     run_time: 20  # Run for 20 seconds
   tests:
     timeout: 30   # second wait for prompt string
     prompt: "screen starts" # prompt to show the test start
     expect: ["tests.drivers.display.check.shield"]
   plugins:
     - name: signature
       module: plugins.signature_plugin
       class: VideoSignaturePlugin
       status: "enable"
       config:
         operations: "compare" # operation ('generate', 'compare')
         metadata:
           name: "tests.drivers.display.check.shield" # finger-print stored metadata
           platform: "frdm_mcxn947"
         directory: "./fingerprints" # fingerprints directory to compare with, not used in generate mode
         duration: 100 # number of frames to check
         method: "combined" #Signature method ('phash', 'dhash', 'histogram', 'combined')
         threshold: 0.65
         phash_weight: 0.35
         dhash_weight: 0.25
         histogram_weight: 0.2
         edge_ratio_weight: 0.1
         gradient_hist_weight: 0.1

Installation Guide
------------------

.. code-block:: powershell

   # Create virtual environment
   python -m venv .venv

.. code-block:: powershell

   # Activate environment
   .venv\Scripts\activate

.. code-block:: powershell

   # Install dependencies
   uv pip install -r requirements.txt

.. code-block:: bash

   # add video to user group
   sudo usermod -a -G video $USER

.. code-block:: bash

   # need log out and login to effective or run
   newgrp video

If you are in Ubuntu 24.04 with XWayland do the following:

.. code-block:: bash

   export DISPLAY=:0
   cp /run/user/1000/.mutter-Xwaylandauth.AIT2A3 ~/.Xauthority

or

.. code-block:: bash

   export QT_QPA_PLATFORM=xcb

If your server does not have display please do the following:

.. code-block:: bash

   pip uninstall opencv-python

.. code-block:: bash

   pip install opencv-python-headless

.. code-block:: bash

   export QT_QPA_PLATFORM=offscreen

example zephyr display tests
----------------------------

1. Setup camera to capture display content

   - UVC compatible camera with at least 2 megapixels (such as 1080p)
   - A light-blocking black curtain
   - A PC host where camera connect to
   - DUT connected to the same PC host for flashing and serial console

2. Generate video fingerprints

   - build and flash the known-to-work display app to DUT e.g.

     ::

        west build -b frdm_mcxn947/mcxn947/cpu0 tests/drivers/display/display_check
        west flash

   - clone code

     ::

        git clone https://github.com/hakehuang/camera_shield

   - follow the instructions in the repo's README.

   - set the signature capture mode as below in config.yaml

     ::

        - name: signature
          module: .plugins.signature_plugin
          class: VideoSignaturePlugin
          status: "enable"
          config:
            operations: "generate" # operation ('generate', 'compare')
            metadata:
              name: "tests.drivers.display.check.shield" # finger-print stored metadata
              platform: "frdm_mcxn947"
            directory: "./fingerprints" # fingerprints directory to compare with not used in generate mode

   - Run generate fingerprints program outside the camera_shield folder

     ::

        python -m camera_shield.main --config camera_shield/config.yaml

     video fingerprint for captured screenshots will be recorded in directory './fingerprints' by default

   - set environment variable to "DISPLAY_TEST_DIR"

     ::

        DISPLAY_TEST_DIR=~/camera_shield/

3. Run test

   ::

      # export the fingerprints path
      export DISPLAY_TEST_DIR=<path to "fingerprints" parent-folder>
      # Twister hardware map file settings:
      # Ensure your map file has the required fixture
      # in the example below, you need to have "fixture_display"
      # Ensure you have installed the required Python packages for tests in scripts/requirements-run-test.txt
      # Run detection program
      scripts/twister --device-testing --hardware-map map.yml -T tests/drivers/display/display_check/

Notes
-----

1. When generating the fingerprints, they will be stored in folder "name" as defined in "metadata" from ``config.yaml``.
2. The DUT testcase name shall match the value in the metadata 'name' field of the captured fingerprint's config.
3. You can put multiple fingerprints in one folder, it will increase compare time, but will help to check other defects.

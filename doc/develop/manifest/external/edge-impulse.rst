.. _external_module_edge_impulse:

Edge Impulse SDK for Zephyr
############################

Introduction
************

The Edge Impulse SDK for Zephyr is an external module that enables seamless integration of trained machine learning models into Zephyr applications. It provides native integration with Zephyr's build system through the module architecture, eliminating manual SDK setup and enabling deployment across 850+ Zephyr-supported hardware targets.

The module includes custom West extension commands (`west ei-build` and `west ei-deploy`) that integrate directly with Edge Impulse Studio APIs for streamlined model deployment workflows. This approach differs from manual C++ library integration by providing automatic updates, cleaner integration, and native Zephyr build support.

Usage with Zephyr
*****************

**Prerequisites:**
- Trained Edge Impulse model
- Zephyr SDK or Nordic NCS installed
- West meta-tool: `pip install -U west`
- Edge Impulse API key and project ID

**Integration Methods:**

1. **Standalone Project (Quick Start):**
   ```bash
   west init -m https://github.com/edgeimpulse/example-standalone-inferencing-zephyr-module
   cd example-standalone-inferencing-zephyr-module
   west update
   ```

2. **Existing Project Integration:**
   Add to your `west.yml`:
   ```yaml
   - name: edge-impulse-sdk-zephyr
     path: modules/edge-impulse-sdk-zephyr
     revision: v1.75.4
     url: https://github.com/edgeimpulse/edge-impulse-sdk-zephyr
     west-commands: scripts/west-commands.yml
   ```

**Model Deployment Workflow:**

1. Build model in Edge Impulse Studio or use West commands:
   ```bash
   west ei-build -k <API_KEY> -p <PROJECT_ID>
   west ei-deploy -k <API_KEY> -p <PROJECT_ID>
   unzip ei_model.zip -d ./model
   ```

2. Add model path to `CMakeLists.txt`:
   ```cmake
   list(APPEND ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_SOURCE_DIR}/model)
   ```

3. Configure target board in `.west/config`:
   ```ini
   [build]
   board = <your_board>
   ```

4. Build and flash:
   ```bash
   west build --pristine
   west flash
   ```

**West Extension Commands:**

- `west ei-build`: Triggers Studio build with optional parameters (`-e tflite-eon`, `-t int8`, `-i 1`)
- `west ei-deploy`: Downloads pre-built deployment artifacts
- Both require `-k` (API key) and `-p` (project ID) flags

**Note:** Commands must be run from the manifest repository directory and require `west-commands` registration in `west.yml`.

Reference
*********

- Zephyr Modules Documentation: https://docs.zephyrproject.org/latest/develop/modules.html
- Edge Impulse SDK Repository: https://github.com/edgeimpulse/edge-impulse-sdk-zephyr
- Example Project: https://github.com/edgeimpulse/example-standalone-inferencing-zephyr-module
- Build API: https://docs.edgeimpulse.com/apis/studio/jobs/build-on-device-model
- Download API: https://docs.edgeimpulse.com/apis/studio/deployment/download
- Zephyr Getting Started: https://docs.zephyrproject.org/latest/develop/getting_started/index.html
- West Tool Documentation: https://docs.zephyrproject.org/latest/develop/west/index.html
- Supported Boards: https://docs.zephyrproject.org/latest/boards/index.html

==============
Display capture Twister harness
==============


Configuration example
---------------------

```yaml: config.yaml
case_config:
  device_id: 0  # Try different camera indices
  res_x: 1280   # x reslution
  res_y: 720    # y resolution
  fps: 30       # analysis frame pre-second
  run_time: 20  # Run for 20 seconds
tests:
  timeout: 30   # second wait for prompt string
  prompt: "screen starts" # prompt to show the test start
  expect: ["sample.display.shield"]
plugins:
  - name: signature
    module: plugins.signature_plugin
    class: VideoSignaturePlugin
    status: "enable"
    config:
      operations: "compare" # operation ('generate', 'compare')
      metadata:
        name: "sample.display.shield" # finger-print stored metadata
        platform: "frdm_mcxn947"
      directory: "./fingerprints" # fingerprints directory to compare with not used in generate mode
      duration: 100 # number of frames to check
      method: "combined" #Signature method ('phash', 'dhash', 'histogram', 'combined')
      threshold: 0.65
      phash_weight: 0.35
      dhash_weight: 0.25
      histogram_weight: 0.2
      edge_ratio_weight: 0.1
      gradient_hist_weight: 0.1
```

example zephyr display tests
----------------------------

1. Setup camera to capture display content

 - UVC compatible camera with at least 2 megapixels (such as 1080p)
 - a light-blocking black curtain
 - a PC host where camera connect to
 - DUT connect to same PC host for flashing and console uart

2. Generate video fingerprints

 - build and flash the known to work firmware to DUT
    e.g.
```
west build -b frdm_mcxn947/mcxn947/cpu0 tests/drivers/display/display_check
west flash
```

 - clone code
```bash
git clone https://github.com/hakehuang/camera_shield
```

  - set the signature capture mode as below in config.yaml
```yaml
  - name: signature
    module: .plugins.signature_plugin
    class: VideoSignaturePlugin
    status: "enable"
    config:
      operations: "generate" # operation ('generate', 'compare')
      metadata:
        name: "test.display.shield" # finger-print stored metadata
        platform: "frdm_mcxn947"
      directory: "./fingerprints" # fingerprints directory to compare with not used in generate mode
```

  - Run generate fingerprints program
```bash
python -m camera_shield.main --config camera_shield/config.yaml
```

video fingerprint for given screen shots will be recorded in directory './fingerprints' by default

   - set environment variable to "DISPLAY_TEST_DIR"

```bash
DISPLAY_TEST_DIR=~/camera_shield/
```

3. Run test
```bash
# export the fingerprints path
export DISPLAY_TEST_DIR=<your path with fingerprints subfolder inside>

# map file settings
# ensure your map file has the required fixture
# in below example you need have "fixture_display"

# Run detection program
scripts/twister --device-testing --hardware-map ~/frdm_mcxn947/map.yaml -T tests/drivers/display/display_check/

```

Notes
-----

1. when generating the fingerprints, they will stored in "name" as defined in "metadata" from ``config.yaml`` .
2. the DUT case will match the name with captured one.
3. you can put mutliply fingerprints in one folder, it will increase compare time,
   but will help to check other defects.

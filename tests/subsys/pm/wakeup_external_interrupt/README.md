**TESTS**

**pm.wakeup_external_interrupt:** Verify the system can wake up from sleep modes using an external interrupt.
steps:
```
Zephyr App:
-> Setup Wakeup Source (PF4 (INT4))
-> Go sleep
-> Waiting for External interrupt

Pytest:
-> Init Power Monitor
-> Start measure
-> Connect to the Controller's shell
-> Init PC6 as output (Controller side)
-> Set GPIO High to wake up the Target (Controller side)
-> Stop Measure
-> Analyze data
```

**SETUP**
![image](https://github.com/user-attachments/assets/493adb86-cf78-4577-a10e-5f3b24c6fef7)

![image](https://github.com/user-attachments/assets/2627f4ce-81a2-4e61-8a58-9deeaf6aa6d9)

**To Reproduce**
1. Configure the boards according to above diagram.
2. Connect all 3 usb to the host.
3. Copy the path of the `Power Monitor`, `Controller` and update the `<path_to_power_monitor>`,`<power_to_controller>` in the `tests/subsys/pm/wakeup_external_interrupt/target/testcase.yaml` file
    under the pytest_args section as follows:
   
`pytest_args: [--power-monitor=<path_to_power_monitor>,--controller=<path_to_controller>]`

4. Run the test `pm.wakeup_external_interrupt.controller` to build and flash application to the controller.
  ```
  twister --device-testing --device-serial <path_to_controller> -p stm32l562e_dk -s pm.wakeup_external_interrupt.controller --west-flash="--dev-id=<controller_serial_number>" --west-runner=pyocd
  ```
 
  To get the `<controller_serial_number>` run:
  `pyocd list`
  Output:
  ```
    #   Probe/Board     Unique ID                  Target            
-------------------------------------------------------------------
  0   STLINK-V3       0048004##########3333639   ✔︎ stm32l562qeixq  
      STM32L562E-DK                                                
                                                                   
  1   STLINK-V3       004E003##########3333639   ✔︎ stm32l562qeixq  
      STM32L562E-DK                                      
  ```

5. Run the test `pm.wakeup_external_interrupt.target`.
   
   ```
   twister --device-testing --device-serial <path_to_controller> -p stm32l562e_dk -s pm.wakeup_external_interrupt.target --west-flash="--dev-id=<controller_serial_number>" --west-runner=pyocd
   ```

Twister log:
```
DEBUG   - PYTEST: ============================= test session starts ==============================
DEBUG   - PYTEST: platform linux -- Python 3.10.12, pytest-8.1.0, pluggy-1.4.0 -- /usr/bin/python3
DEBUG   - PYTEST: cachedir: .pytest_cache
DEBUG   - PYTEST: rootdir: <path_to>/zephyr
DEBUG   - PYTEST: collecting ... collected 1 item
DEBUG   - PYTEST: tests/subsys/pm/wakeup_external_interrupt/target/pytest/test_wakeup_external_interrupt.py::test_wakeup_external_interrupt
DEBUG   - PYTEST: -------------------------------- live log setup --------------------------------
DEBUG   - PYTEST: DEBUG: Get device type "hardware"
DEBUG   - PYTEST: DEBUG: Opening serial connection for /dev/ttyACM1
DEBUG   - PYTEST: DEBUG: Flashing command: <path_to>/.local/bin/west flash --skip-rebuild --build-dir <path_to>/zephyr/twister-out/stm32l562e_dk/tests/subsys/pm/wakeup_external_interrupt/target/pm.wakeup_external_interrupt.target --runner pyocd -- --dev-id=0048004############33639
DEBUG   - PYTEST: DEBUG: Flashing finished
DEBUG   - PYTEST: [CMD] htc
DEBUG   - PYTEST: PowerShield > ack htc
DEBUG   - PYTEST: [CMD] format bin_hexa
DEBUG   - PYTEST: PowerShield > ack format bin_hexa
DEBUG   - PYTEST: [CMD] freq 1k
DEBUG   - PYTEST: PowerShield > ack freq 1k
DEBUG   - PYTEST: [CMD] acqtime 0
DEBUG   - PYTEST: PowerShield > ack acqtime 0
DEBUG   - PYTEST: [CMD] volt 3300m
DEBUG   - PYTEST: PowerShield > ack volt 3300m
DEBUG   - PYTEST: [CMD] funcmode high
DEBUG   - PYTEST: PowerShield > ack funcmode high
DEBUG   - PYTEST: [CMD] start
DEBUG   - PYTEST: setting gpioc 6 as output
DEBUG   - PYTEST: setting gpioc 6 to 1
DEBUG   - PYTEST: setting gpioc 6 to 0
DEBUG   - PYTEST: [CMD] stop
DEBUG   - PYTEST: stop 2:
DEBUG   - PYTEST: Expected RMS: 0.34 mA
DEBUG   - PYTEST: Tolerance: 0.03 mA
DEBUG   - PYTEST: Current RMS: 0.31 mA
DEBUG   - PYTEST: active:
DEBUG   - PYTEST: Expected RMS: 95.00 mA
DEBUG   - PYTEST: Tolerance: 9.50 mA
DEBUG   - PYTEST: Current RMS: 95.32 mA
DEBUG   - PYTEST: PASSED
DEBUG   - PYTEST: ------------------------------ live log teardown -------------------------------
DEBUG   - PYTEST: DEBUG: Closed serial connection for /dev/ttyACM1
DEBUG   - PYTEST: - generated xml file: <path_to>twister-out/stm32l562e_dk/tests/subsys/pm/wakeup_external_interrupt/target/pm.wakeup_external_interrupt.target/report.xml -
DEBUG   - PYTEST: ============================== 1 passed in 15.35s ==============================
DEBUG   - run status: stm32l562e_dk/tests/subsys/pm/wakeup_external_interrupt/target/pm.wakeup_external_interrupt.target passed
INFO    - 1/1 stm32l562e_dk             tests/subsys/pm/wakeup_external_interrupt/target/pm.wakeup_external_interrupt.target PASSED (device 15.371s)
```

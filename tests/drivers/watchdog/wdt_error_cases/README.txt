This test suite contains negative test cases for the Watchdog driver.
Test scenarios validate that invalid use of the watchdog driver
returns error code as described in the API documentation
(or Assertion Fail as explained below).

Ideally, the driver shall be fully compliant with the documentation.
However, it may happen that invalid function call results in
Assertion Fail instead of error code.
Since, main goal is to detect (and report) invalid use of the driver,
both error code and assertion fail set test result as passed.
See for example test_02_wdt_setup_before_setting_timeouts where
ztest_set_assert_valid(true);
is used to catch assertion fail.

These tests were written to increase test coverage for the Watchdog driver.
Since, coverage data is stored in the RAM, it is lost when watchdog fires.
Therefore, in all test cases watchdog shall NOT expire.
Use other sample to verify positive scenario for the watchdog driver.

These tests were prepared on a target that had a bug in the wdt_disable()
implementation. As a temporary remedy, order of tests was imposed.
Since, tests are executed alphabetically, order was set by starting
each test name with f.e. 'test_08b_...'.


Tests are based on the watchdog API documentation available here:
https://docs.zephyrproject.org/latest/hardware/peripherals/watchdog.html


Follow these guidelines when enabling test execution on a new target.

1. Although, code defines WDT_DISABLE_SUPPORTED but it was tested on a
   target that supports disabling the watchdog.
   Multiple tests call wdt_setup() which starts the watchdog.
   Every test assumes that watchdog is disabled at startup.
   As a result, executing this test suite on a target that doesn't
   support wdt_disable() may require changes to the code.
   When target supports wdt_disable() then extend WDT_TEST_FLAGS with:
   #define WDT_TEST_FLAGS (... | WDT_DISABLE_SUPPORTED)

2. There are three watchdog flags that can be passed to wdt_install_timeout():
    - WDT_FLAG_RESET_NONE - when watchdog expires, it's callback is serviced but
      reset doesn't occur.
    - WDT_FLAG_RESET_CPU_CORE - when watchdog expires, only one core is reset, while
      other cores are not affected.
    - WDT_FLAG_RESET_SOC - when watchdog expires, target as "a whole" is reset.
   Support for these flags varies between vendors and products.
   a) List all supported flags in
      #define WDT_TEST_FLAGS (... | WDT_FLAG_RESET_NONE_SUPPORTED |
                              WDT_FLAG_RESET_CPU_CORE_SUPPORTED |
                              WDT_FLAG_RESET_SOC_SUPPORTED)
   b) Set supported flag in
      #define DEFAULT_FLAGS (WDT_FLAG_RESET_SOC)
      This define will be used in wdt_install_timeout() "correct" test step.

3. These tests assume that watchdog driver supports multiple timeouts. Set
   #define MAX_INSTALLABLE_TIMEOUTS (8)

4. When all watchdog timeouts must have exactly the same watchdog timeout value
   then extend WDT_TEST_FLAGS with:
   #define WDT_TEST_FLAGS (... | WDT_FLAG_ONLY_ONE_TIMEOUT_VALUE_SUPPORTED)

5. Set maximal allowed watchdog timeout value, f.e.:
   #define WDT_WINDOW_MAX_ALLOWED (0xFFFFFFFFU)
   Also, test assumes that minimal allowed watchdog timeout value must be zero.

6. There are two watchdog options that can be passed to wdt_setup():
    - WDT_OPT_PAUSE_IN_SLEEP;
    - WDT_OPT_PAUSE_HALTED_BY_DBG.
   Support for these options varies between vendors and products.
   a) List all supported options in
      #define WDT_TEST_FLAGS (... | WDT_OPT_PAUSE_IN_SLEEP_SUPPORTED |
                              WDT_OPT_PAUSE_HALTED_BY_DBG_SUPPORTED)
   b) Set supported option(s) in
      #define DEFAULT_OPTIONS (WDT_OPT_PAUSE_IN_SLEEP | WDT_OPT_PAUSE_HALTED_BY_DBG)
      This define will be used in wdt_setup() "correct" test step.

7. When wdt_feed() can stall, extend WDT_TEST_FLAGS with:
   #define WDT_TEST_FLAGS (... | WDT_FEED_CAN_STALL)

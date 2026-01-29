# Ticker Module Unit Tests

## Overview

This directory contains comprehensive unit tests for the ticker module used in the Zephyr Bluetooth Low Energy Controller. The ticker module provides high-precision time-based scheduling functionality critical for managing timing-sensitive BLE operations.

## Test Coverage

### Core Functionality
- **Initialization and Deinitialization**: Tests `ticker_init()`, `ticker_deinit()`, and `ticker_is_initialized()`
- **Start and Stop Operations**: Tests `ticker_start()`, `ticker_stop()`, and `ticker_stop_abs()`
- **Update Operations**: Tests `ticker_update()` for drift and slot adjustments
- **Callback Execution**: Validates timeout and operation completion callbacks

### Advanced Features
- **Multiple Concurrent Tickers**: Tests multiple ticker nodes operating simultaneously
- **Lazy Timeout Handling**: Tests peripheral latency support with lazy parameter
- **Must Expire Flag**: Tests `TICKER_LAZY_MUST_EXPIRE` for forced callbacks
- **Sub-microsecond Precision**: Tests `ticker_start_us()` with remainder support
- **Extended Features**: Tests `ticker_start_ext()` and `ticker_update_ext()` (when `CONFIG_BT_TICKER_EXT` is enabled)

### Utility Functions
- **Tick Difference Calculation**: Tests `ticker_ticks_diff_get()` including wraparound handling
- **Next Slot Query**: Tests `ticker_next_slot_get()` for querying next expiring ticker
- **Idle Status**: Tests `ticker_job_idle_get()` for job status queries

### Edge Cases and Error Handling
- **Invalid Instance**: Tests operations on uninitialized instances
- **Boundary Conditions**: Tests maximum ticker node allocation
- **Wraparound Handling**: Tests tick counter wraparound scenarios
- **Return Code Validation**: Validates all return codes (SUCCESS, BUSY, FAILURE)

## Architecture

### Test Structure
```
tests/bluetooth/controller/ticker/
├── CMakeLists.txt      # Build configuration
├── testcase.yaml       # Test metadata
├── prj.conf           # Kconfig configuration
├── README.md          # This file
└── src/
    ├── main.c         # Test implementations
    └── hal_mock.c     # HAL interface mocks
```

### Mock Implementation

The tests use mock implementations for hardware abstraction layer (HAL) interfaces:
- `cntr_cnt_get()`: Returns simulated counter values
- `cntr_cmp_set()`: No-op for compare value setting
- `cpu_dmb()`: No-op for memory barrier

These mocks allow deterministic testing without hardware dependencies.

## Test Configuration

### Kconfig Options Enabled
- `CONFIG_BT_TICKER_EXT`: Extended ticker features
- `CONFIG_BT_TICKER_REMAINDER_SUPPORT`: Sub-microsecond precision
- `CONFIG_BT_TICKER_EXT_EXPIRE_INFO`: Multiple ticker expire tracking
- `CONFIG_BT_TICKER_PRIORITY_SET`: Priority-based scheduling

### Test Parameters
- **Ticker Nodes**: 8 nodes configured
- **Ticker Users**: 2 users configured
- **User Operations**: 8 concurrent operations supported

## Building and Running

### Build the tests:
```bash
west build -b native_sim tests/bluetooth/controller/ticker
```

### Run the tests:
```bash
west build -t run
```

### Run specific test:
```bash
west build -b native_sim tests/bluetooth/controller/ticker -- -DCONF_FILE=prj.conf
```

## Test Scenarios

### 1. Basic Ticker Lifecycle
Tests the fundamental lifecycle: init → start → expire → stop → deinit

### 2. Periodic Ticker Operation
Tests periodic tickers with multiple expirations

### 3. One-shot Ticker
Tests single-expiration (non-periodic) tickers

### 4. Multiple Concurrent Tickers
Tests 4+ simultaneous tickers with different timing

### 5. Drift Correction
Tests `ticker_update()` for timing adjustments

### 6. Lazy Timeout with Peripheral Latency
Tests skipping callbacks for power optimization

### 7. Must Expire Priority
Tests critical tickers that cannot be skipped

### 8. Sub-tick Precision
Tests remainder accumulation for high-precision timing

### 9. Extended Features (Conditional)
Tests collision avoidance and rescheduling when enabled

## Expected Results

All tests should pass with status `PASS`. Example output:
```
START - ticker_tests
 PASS - test_ticker_init_deinit (0.001s)
 PASS - test_ticker_start_stop (0.002s)
 PASS - test_ticker_timeout_callback (0.001s)
 PASS - test_multiple_ticker_nodes (0.003s)
 PASS - test_ticker_update (0.002s)
 ...
TESTSUITE ticker_tests succeeded
```

## Limitations and Assumptions

### Current Limitations
1. **Hardware Timing**: Tests use mocked time, not real hardware timing
2. **Interrupt Context**: Tests run in thread context, not actual ISR context
3. **Race Conditions**: Some concurrency scenarios are simplified

### Assumptions
1. Ticker module is compiled with standard configuration
2. HAL interfaces can be mocked without affecting core logic
3. Deterministic test execution order

## Future Enhancements

### Planned Additions
- [ ] Test collision handling and resolution
- [ ] Test priority-based ticker preemption
- [ ] Test slot window rescheduling
- [ ] Test race condition scenarios
- [ ] Test stress scenarios with many concurrent tickers
- [ ] Integration tests with actual BLE controller use cases

### Configuration Variants
- [ ] Test with `CONFIG_BT_TICKER_SLOT_AGNOSTIC` enabled
- [ ] Test with `CONFIG_BT_TICKER_LOW_LAT` enabled
- [ ] Test without extended features

## References

- Ticker API: `subsys/bluetooth/controller/ticker/ticker.h`
- Ticker Implementation: `subsys/bluetooth/controller/ticker/ticker.c`
- BLE Controller Usage: `subsys/bluetooth/controller/ll_sw/`
- Ztest Framework: https://docs.zephyrproject.org/latest/develop/test/ztest.html

## Maintainer Notes

### Adding New Tests
1. Add test function with `ZTEST(ticker_tests, test_name)` macro
2. Use `test_timeout_callback` and `test_op_callback` for validation
3. Call `ticker_job()` to process operations
4. Reset callbacks with `reset_test_callbacks()` between operations
5. Use `zassert_*` macros for validation

### Debugging Tests
- Enable verbose assertions with `CONFIG_ASSERT_VERBOSE=y`
- Check `op_callback_status` for operation results
- Use `timeout_callback_count` to verify callback invocations
- Set breakpoints in callbacks to trace execution

## License

SPDX-License-Identifier: Apache-2.0

Copyright (c) 2024 Nordic Semiconductor ASA

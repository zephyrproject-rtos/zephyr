# PTY Symlink Test Suite

This test suite validates the symlink creation functionality in Zephyr's native simulator PTY UART driver.

## Overview

The test suite validates that the PTY symlink implementation works correctly with the native_sim platform, including symlink creation, bidirectional communication, error handling, and end-to-end functionality.

## Test Structure

### End-to-End Test Script (`test_e2e.sh`)

Comprehensive shell script that performs complete PTY symlink validation:

- Builds and runs the test application with both standard and interactive configurations
- Validates PTY connection establishment and symlink creation
- Tests bidirectional UART communication (hello/world exchange)
- Validates symlink cleanup on application exit
- Tests error handling scenarios (invalid paths, file collisions)
- Provides detailed test results and diagnostics

The main test file (`src/main.c`) contains environment validation and conditional interactive UART communication tests.

## Running the Tests

### End-to-End Tests

```bash
cd zephyr/tests/boards/native_sim/pty_symlink
./test_e2e.sh
```

This script will:
1. Set up test environment and build the application
2. Run unit tests for environment validation
3. Build interactive configuration and test bidirectional communication
4. Validate symlink creation, communication, and cleanup
5. Test error handling scenarios
6. Provide comprehensive test summary

## Test Coverage

The test suite validates:

- ✅ Test environment setup and configuration
- ✅ PTY connection establishment (`uart connected to pseudotty: /dev/pts/N`)
- ✅ Build system integration with native_sim platform
- ✅ Symlink creation with `--uart_symlink` command line option
- ✅ Symlink target validation (points to `/dev/pts/N` format)
- ✅ PTY device accessibility (read/write permissions)
- ✅ Bidirectional UART communication (hello/world exchange)
- ✅ Automatic symlink cleanup on application exit
- ✅ Error handling for invalid paths and file collisions
- ✅ Application self-shutdown mechanism

## Expected Results

### End-to-End Test Results

The end-to-end script validates:

1. ✅ Test environment setup
2. ✅ Application builds successfully for native_sim
3. ✅ Unit tests pass (environment validation)
4. ✅ PTY connections are established
5. ✅ Symlink creation and validation
6. ✅ Bidirectional UART communication
7. ✅ Automatic symlink cleanup
8. ✅ Error handling scenarios

Example successful output:
```
=== PTY Symlink End-to-End Test ===
✓ Test environment setup
✓ Application builds successfully
✓ Unit tests pass
✓ PTY symlink creation works
✓ Symlink points to valid PTY device
✓ PTY device is accessible
✓ Symlink cleanup: automatic
✓ Error handling scenarios tested
All PTY symlink functionality tests completed successfully!
```

### Interactive Communication Test

When `CONFIG_PTY_INTERACTIVE_TEST` is enabled:

1. Application creates symlink at specified path
2. Symlink points to a `/dev/pts/N` device
3. Device is readable and writable
4. Bidirectional communication works (sends "world" in response to "hello")
5. Application triggers self-shutdown after communication
6. Symlink is automatically cleaned up

## Configuration Options

### Test Configuration

- `CONFIG_PTY_INTERACTIVE_TEST`: Enables interactive UART communication tests
- `prj.conf`: Standard configuration with interactive tests disabled
- `prj_interactive.conf`: Configuration with interactive tests enabled

### Driver Configuration

- `CONFIG_UART_NATIVE_POSIX`: Required for native PTY driver
- `--uart_symlink=<path>`: Command line option for symlink creation

## Integration with Testing Infrastructure

This test suite integrates with Zephyr's testing infrastructure:

- Uses `ztest` framework for environment validation
- Follows Zephyr test naming conventions
- Includes proper test metadata in `testcase.yaml`
- Provides comprehensive end-to-end validation via shell script
- Supports both automated and interactive testing modes

## Troubleshooting

### Common Issues

1. **Permission denied**: Ensure `/tmp` directory is writable
2. **PTY allocation failed**: May occur on systems with limited PTY devices
3. **Symlink creation failed**: Check filesystem permissions and available space
4. **Build failures**: Ensure `CONFIG_UART_NATIVE_POSIX` is enabled
5. **Communication test failures**: Verify PTY device is accessible and writable

### Debug Information

The end-to-end test script provides comprehensive output including:
- Detailed build logs
- Application output and error messages
- Symlink validation results
- Communication test results
- Error handling test logs

## Related Files

- PTY Implementation: `drivers/serial/uart_native_pty_bottom.c`
- Command line parsing: `drivers/serial/uart_native_pty.c`
- Test configuration: `prj.conf`, `prj_interactive.conf`, `testcase.yaml`, `CMakeLists.txt`
- Test application: `src/main.c`
- End-to-end validation: `test_e2e.sh`

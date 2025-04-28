# Testing USB Printer Implementation

This document describes the testing procedures for the USB printer implementation, including both manual testing steps and automated test procedures.

## Prerequisites

- Python 3.6 or later
- PyUSB library (`pip install -r requirements.txt`)
- USB-capable development board flashed with the printer sample
- Host computer with USB port

## Test Categories

### 1. Basic Functionality Tests

#### Manual Testing
1. Connect the device and verify enumeration
2. Check device appears as USB printer in system
3. Send basic text file for printing
4. Verify output format

#### Automated Testing
```bash
python test_printer.py --basic
```

This tests:
- Device enumeration
- Basic text printing
- Status reporting
- Error-free operation

### 2. PCL Command Tests

#### Manual Testing
1. Send PCL-formatted document
2. Test font selection commands
3. Verify page orientation changes
4. Test line spacing adjustments

#### Automated Testing
```bash
python test_printer.py --pcl
```

This tests:
- PCL command recognition
- Command parameter parsing 
- State changes after commands
- Error handling for invalid commands

### 3. PJL Job Control Tests

#### Manual Testing
1. Send PJL job commands
2. Monitor job status changes
3. Test job cancellation
4. Verify job completion reporting

#### Automated Testing
```bash
python test_printer.py --pjl
```

This tests:
- Job control sequences
- Status reporting
- Job queue management
- PJL response formatting

### 4. Error Handling Tests

#### Manual Testing
1. Simulate paper-out condition
2. Test buffer overflow scenarios
3. Send invalid commands
4. Test recovery procedures

#### Automated Testing
```bash
python test_printer.py --error
```

This tests:
- Error detection
- Status reporting
- Recovery mechanisms
- Queue preservation

## Running All Tests

To run the complete test suite:

```bash
python test_printer.py --all
```

## Test Script Details

### Command Line Arguments
- `--all`: Run all test categories
- `--basic`: Run basic functionality tests
- `--pcl`: Run PCL command tests
- `--pjl`: Run PJL job control tests
- `--error`: Run error handling tests

### Test Output
The script provides detailed output including:
- Test progress
- Byte counts sent/received
- Status codes
- Error messages
- Test results summary

### Exit Codes
- 0: All tests passed
- 1: One or more tests failed
- Other: Script execution error

## Adding New Tests

1. Create new test method in PrinterTester class
2. Add command line argument
3. Update main() to include new test
4. Document test procedure here

## Troubleshooting

### Common Issues

1. Device Not Found
   - Check USB connection
   - Verify correct firmware flash
   - Check USB permissions (Linux)

2. Test Failures
   - Check debug logs
   - Verify test prerequisites
   - Check USB connection stability

3. Timing Issues
   - Adjust sleep durations
   - Check system load
   - Verify USB controller stability

### Debug Tools

1. USB Protocol Analyzer
   - Wireshark with USBPcap
   - USB Protocol Analyzer hardware

2. System Logs
   - dmesg output
   - Windows Device Manager
   - USB device enumeration

## Continuous Integration

The test script is designed to work in CI environments:
- Exit codes for pass/fail
- Structured output
- No interactive requirements
- Configurable timeouts

## References

- [USB Printer Class Specification](https://www.usb.org/documents)
- [PCL Reference](https://developers.hp.com/hp-linux-imaging-and-printing/pcl)
- [PJL Reference](https://developers.hp.com/hp-linux-imaging-and-printing/pjl)
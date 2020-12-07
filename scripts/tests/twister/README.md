# Sanitycheck Testing

Running the tests require the environment variable ZEPHYR_BASE to be set.

Sanitycheck Testsuite are located in $ZEPHYR_BASE/scripts/tests directory with all the data files in $ZEPHYR_BASE/scripts/test_data directory.

## Dependencies

Install all the dependencies using

```
pip install -r $ZEPHYR_BASE/scripts/requirements-build-test.txt
```

## Executing testsuite

The testcases can be executed from the root directory using

```
pytest $ZEPHYR_BASE/scripts/tests/sanitycheck
```

## Sanitycheck Coverage

The coverage for all the tests can be run using the command below. This will collect all the tests available.

```bash
coverage run -m pytest $ZEPHYR_BASE/scripts/tests/sanitycheck/
```

Then we can generate the coverage report for just sanitylib script using

```bash
coverage report -m $ZEPHYR_BASE/scripts/sanity_chk/sanitylib.py
```

To generate the coverage report for sanitycheck script use below command

```bash
coverage report -m $ZEPHYR_BASE/scripts/sanitycheck
```

The html coverage report for sanitycheck can be generated using

```bash
coverage html sanitycheck
```

If needed,the full coverage html report can be generated in every run of "pytest" in the tests directory using configuration file (setup.cfg).

## Organization of tests

- conftest.py: Contains common fixtures for use in testing the sanitycheck tool.
- test_sanitycheck.py : Contains basic testcases for environment variables, verifying testcase & platform schema's.
- test_testsuite_class.py : Contains testcases for Testsuite class (except reporting functionality) in sanitylib.py.
- test_testinstance.py : Contains testcases for Testinstance and Testcase class.
- test_reporting_testsuite.py : Contains testcases for reporting fucntionality of Testsuite class of sanitycheck.

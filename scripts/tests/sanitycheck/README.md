# Sanitycheck Testing

Running the tests require the environment variable ZEPHYR_BASE to be set.

Sanitycheck Testsuite are located in $ZEPHYR_BASE/scripts/tests directory with all the data files in $ZEPHYR_BASE/scripts/test_data directory.

## Dependencies

Install all the dependencies using

```
pip install -r $ZEPHYR_BASE/scripts/tests/sanitycheck/requirements.txt
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

Then we can generate the coverage report for just sanitycheck script using

```bash
coverage report -m $ZEPHYR_BASE/scripts/sanitycheck
```

The html coverage report for sanitycheck can be generated using

```bash
coverage html sanitycheck
```

If needed,the full coverage html report can be generated in every run of "pytest" in the tests directory using configuration file (setup.cfg).

## Organization of tests

- test_sanitycheck.py : Contains basic testcases for environment variables, verifying testcase & platform schema's.

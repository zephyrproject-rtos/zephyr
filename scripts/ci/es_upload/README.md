# ElasticSearch index map examples

This directory contains [ElasticSearch data store](https://github.com/elastic/elasticsearch)
index map examples for use by [upload_test_results_es.py](../upload_test_results_es.py)
script for [Twister](https://docs.zephyrproject.org/latest/develop/test/twister.html)
JSON reports upload.

An index map file allows to create destination index files on ElasticSearch server
with explicit scheme having all required Twister JSON report objects associated
to proper data types, eventually to store the expected document structure.
Besides, it allows to track changes in Twister JSON report scheme
and the corresponding data scheme in the same source code repository.
An alternative way is to have index files created at the ElasticSearch server
by other tools, or rely on default data mapping with potential type misalignments.

The command examples below are simplified with parameters essential for data transformations.
For other command line options and more details run the upload script with `--help`,
or read its source code with inline comments there.

Use these examples as a reference for your setup. Always pay attention to possible changes
in the current Twister report format and in the upload script itself.
Tune resulting data scheme and size depending on your particular needs.

It is recommended to try the upload script with `--dry-run` option first
to check the resulting data without its actual upload.

You should set environment variables `ELASTICSEARCH_SERVER` and `ELASTICSEARCH_KEY`
to connect with your server running the upload script.


## Index creation with a map file

Execute the upload script once for the first time to create your index at the ElasticSearch
server with the appropriate map file, for example:
```bash
python3 ./scripts/ci/upload_test_results_es.py --create-index \
  --index YOUR_INDEX_NAME \
  --map-file YOUR_INDEX_MAP.json
```


## Upload transformations

The upload script has several command line options to change `twister.json` data:
exclude excess data, extract substrings by regular expressions, change data structure
making it suitable for Kibana default visual components
(see `--help` for more details):

* `--exclude` removes excess testsuite properties not needed to store them
  at Elasticsearch index.

* `--transform` applies regexp group parsing rules to string properties extracting
  derived object properties.

* `--flatten` changes testsuite data structure in regard of one of its list components:
  either `testcases` or `recording`, so each item there becomes an independent data record
  inheriting all other testsuite properties, whereas the children object's properties
  are renamed with the parent object's name as a prefix:
  `testcases_` or `recording_` respectively.
  Only one testsuite property object can be flattened this way per index upload.
  All other testsuite objects will be treated according to the index structure.

* Other command line options: `--flatten-dict-name`, `--flatten-list-names`,
`--flatten-separator`, `--transpose-separator`, `--escape-separator`.


## Index map examples and use cases


### Twister test results

Create the index:
```bash
python3 ./scripts/ci/upload_test_results_es.py --create-index \
    --index zephyr-test-example \
    --map-file zephyr_twister_index.json
```

Upload Twister test results 'as-is', without additional transformations:
```bash
python3 ./scripts/ci/upload_test_results_es.py \
    --index zephyr-test-example \
    ./twister-out/**/twister.json
```


### Twister tests with recording

Store test results with `recording` data entries, for example from
[Kernel Timer Behavior](https://github.com/zephyrproject-rtos/zephyr/tree/main/tests/kernel/timer/timer_behavior)
test suite.

Create the index:
```bash
python3 ./scripts/ci/upload_test_results_es.py --create-index \
    --index zephyr-test-recording-example-1 \
    --map-file zephyr_twister_flat_recording_metrics_index.json
```

Upload data with 'flattened' test suites creating documents for each `recording` data entry.
```bash
python3 ./scripts/ci/upload_test_results_es.py \
    --index zephyr-test-recording-example-1 \
    --exclude path run_id \
    --flatten recording \
    ./twister-out/**/twister.json
```


### Twister test with recording and extracting more data

Store test results with `recording` data entries which are also scanned by regular expressions
to extract embedded values, for example
[Kernel Latency Benchmarks](https://github.com/zephyrproject-rtos/zephyr/tree/main/tests/benchmarks/latency_measure)
test suite.

Create the index:
```bash
python3 ./scripts/ci/upload_test_results_es.py --create-index \
    --index zephyr-test-recording-example-2 \
    --map-file zephyr_twister_flat_recording_index.json
```

Upload data with 'flattened' test suites creating documents for each `record` data entry
with 3 additional data properties extracted by regular expressions.
```bash
python3 ./scripts/ci/upload_test_results_es.py \
    --index zephyr-test-recording-example-2 \
    --exclude path run_id \
    --flatten recording \
    --transform "{ 'recording_metric': '(?P<recording_metric_object>[^\.]+)\.(?P<recording_metric_action>[^\.]+)\.(?P<recording_metric_details>[^ -]+)' }" \
    ./twister-out/**/twister.json
```


### Memory Footprint results

To store Memory Footprint data reported in `twister-footprint.json` (see Twister `--footprint-report`).

Create the index:
```bash
python3 ./scripts/ci/upload_test_results_es.py --create-index \
    --index zephyr-memory-footprint-example \
    --map-file zephyr_twister_flat_footprint_index.json
```

Upload data with 'flattened' footprint entries creating documents for each of them.
```bash
python3 ./scripts/ci/upload_test_results_es.py  \
    --index zephyr-memory-footprint-example \
    --exclude path run_id \
    --flatten footprint \
    --flatten-list-names "{'children':'name'}" \
    --transform "{ 'footprint_name': '^(?P<footprint_area>([^\/]+\/){0,2})(?P<footprint_path>([^\/]*\/)*)(?P<footprint_symbol>[^\/]*)$' }" \
    ../footprint_data/**/twister_footprint.json
```

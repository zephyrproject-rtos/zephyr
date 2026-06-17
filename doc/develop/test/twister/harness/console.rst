.. _twister_console_harness:

Console
#######

The ``console`` harness tells Twister to parse a test's text output for a
regex defined in the test's YAML file.

The following options are currently supported:

type: <one_line|multi_line> (required)
    Depends on the regex string to be matched

regex: <list of regular expressions> (required)
    Strings with regular expressions to match with the test's output
    to confirm the test runs as expected.

ordered: <True|False> (default False)
    Check the regular expression strings in orderly or randomly fashion

record: <recording options> (optional)
  regex: <list of regular expressions> (required)
    Regular expressions with named subgroups to match data fields found
    in the test instance's output lines where it provides some custom data
    for further analysis. These records will be written into the build
    directory ``recording.csv`` file as well as ``recording`` property
    of the test suite object in ``twister.json``.

    With several regular expressions given, each of them will be applied
    to each output line producing either several different records from
    the same output line, or different records from different lines,
    or similar records from different lines.

    The .CSV file will have as many columns as there are fields detected
    in all records; missing values are filled by empty strings.

    For example, to extract three data fields ``metric``, ``cycles``,
    ``nanoseconds``:

    .. code-block:: yaml

      record:
        regex:
          - "(?P<metric>.*):(?P<cycles>.*) cycles, (?P<nanoseconds>.*) ns"

  merge: <True|False> (default False)
    Allows to keep only one record in a test instance with all the data
    fields extracted by the regular expressions. Fields with the same name
    will be put into lists ordered as their appearance in recordings.
    It is possible for such multi value fields to have different number
    of values depending on the regex rules and the test's output.

  as_json: <list of regex subgroup names> (optional)
    Data fields, extracted by the regular expressions into named subgroups,
    which will be additionally parsed as JSON encoded strings and written
    into ``twister.json`` as nested ``recording`` object properties.
    The corresponding ``recording.csv`` columns will contain JSON strings
    as-is.

    Using this option, a test log can convey layered data structures
    passed from the test image for further analysis with summary results,
    traces, statistics, etc.

    For example, this configuration:

    .. code-block:: yaml

      record:
        regex: "RECORD:(?P<type>.*):DATA:(?P<metrics>.*)"
        as_json: [metrics]

    when matched to a test log string:

    .. code-block:: none

      RECORD:jitter_drift:DATA:{"rollovers":0, "mean_us":1000.0}

    will be reported in ``twister.json`` as:

    .. code-block:: json

      "recording":[
          {
                "type":"jitter_drift",
                "metrics":{
                    "rollovers":0,
                    "mean_us":1000.0
                }
          }
      ]

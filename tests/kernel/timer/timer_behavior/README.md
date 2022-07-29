# Test a timer implementations variance and long term drift

Records and calculates statistical values against a timer validating that.

1. Timer variance and standard deviation is below defined acceptable values.
2. Periodic timers do not drift in either direction from expected total time.

Timers are meant to be precise and accurate. This test validates an implementation is both.

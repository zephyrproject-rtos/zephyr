Delta library test vectors
==========================

``vectors.inc`` holds the (source, target, patch) triples consumed by
``tests/lib/delta/src/main.c``. It is committed to the tree so the test
builds without any host tool installed.

To regenerate after changing the vector set:

.. code-block:: shell

   pip install bsdiffhs heatshrink2
   python3 tests/lib/delta/vectors/gen_vectors.py

The generator uses the upstream ``bsdiffhs`` Python library for the
"natural" vectors, and emits a hand-crafted patch for the
``negative_seek`` case so that the bsdiff control tuple with a negative
source seek is exercised deterministically (the host algorithm's match
choices are otherwise opaque from Python).

The vectors are generated with ``window_sz2 = 8`` and
``lookahead_sz2 = 4``, which must match the heatshrink static-buffer
sizes set in the ``libraries.delta.static`` scenario of
``tests.yaml``.

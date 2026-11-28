AVDTP signalling disconnect regression
#####################################

This suite exercises the real AVDTP request, buffer and disconnect paths.
Test replacements provide the L2CAP transport and route Bluetooth work to the
system workqueue, so no controller or peer is required. Source-local compile
definitions rename the real functions to ``real_*``, while the test implements
their original names. This avoids relying on the linker's ``--wrap`` behavior.
The PDU allocator replacement still calls the real allocator to exercise the
actual signalling buffer pool.

The buffer-wait test exhausts the actual signalling PDU pool, disconnects the
session while discovery is blocked, and then releases the buffers. Discovery
must return ``-ENOTCONN`` without installing a request, transmitting a PDU,
starting a timeout or invoking a request callback.

The remaining cases verify pending-request cleanup, serialization of disconnect
with timeout startup, rejection of requests from the disconnect callback before
L2CAP clears its connection pointer, and preservation of ``-EBUSY`` for an
already-pending request on a connected session.

.. zephyr:code-sample:: pm-event-domain
   :name: pm event domain
   :relevant-api: pm_event_device pm_event_domain

   Simple "press button", "turn on led" sample for
   measuring gpio wakeup to led toggle latency.
   Every time the button is pressed, the latency
   request is updated to the next supported
   latency defined by ``event-domain-latencies-us``
   of the event domain.

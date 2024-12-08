.. zephyr:code-sample:: prometheus
   :name: Prometheus Sample
   :relevant-api: http_service http_server tls_credentials prometheus

   Implement a Prometheus Metric Server demonstrating various metric types.

Overview
--------

This sample application demonstrates the use of the ``prometheus`` library.
This library provides prometheus client library(pull method) implementation.
By integrating this library into your code, you can expose internal metrics
via an HTTP endpoint on your application's instance, enabling Prometheus to
scrape and collect the metrics.

Requirement
-----------

`QEMU Networking <https://docs.zephyrproject.org/latest/connectivity/networking/qemu_setup.html#networking-with-qemu>`_

Building and running the server
-------------------------------

To build and run the application:

.. zephyr-app-commands::
   :zephyr-app: samples/net/prometheus
   :board: <board to use>
   :conf: <config file to use>
   :goals: build
   :compact:

When the server is up, we can make requests to the server using HTTP/1.1.

**With HTTP/1.1:**

- Using a browser: ``http://192.0.2.1/metrics``

See `Prometheus client library documentation
<https://prometheus.io/docs/instrumenting/clientlibs/>`_.

Metric Server Customization
---------------------------

The server sample contains several parameters that can be customized based on
the requirements. These are the configurable parameters:

- ``CONFIG_NET_SAMPLE_HTTP_SERVER_SERVICE_PORT``: Configures the service port.

- ``CONFIG_HTTP_SERVER_MAX_CLIENTS``: Defines the maximum number of HTTP/2
  clients that the server can handle simultaneously.

- ``CONFIG_HTTP_SERVER_MAX_STREAMS``: Specifies the maximum number of HTTP/2
  streams that can be established per client.

- ``CONFIG_HTTP_SERVER_CLIENT_BUFFER_SIZE``: Defines the buffer size allocated
  for each client. This limits the maximum length of an individual HTTP header
  supported.

- ``CONFIG_HTTP_SERVER_MAX_URL_LENGTH``: Specifies the maximum length of an HTTP
  URL that the server can process.

To customize these options, we can run ``west build -t menuconfig``, which provides
us with an interactive configuration interface. Then we could navigate from the top-level
menu to: ``-> Subsystems and OS Services -> Networking -> Network Protocols``.


Prometheus Configuration
------------------------

.. code-block:: yaml

    scrape_configs:
      - job_name: 'your_server_metrics'
        static_configs:
          - targets: ['your_server_ip:your_server_port']
        # Optional: Configure scrape interval
        # scrape_interval: 15s

Replace ``'your_server_metrics'`` with a descriptive name for your job,
``'your_server_ip'`` with the IP address or hostname of your server, and
``'your_server_port'`` with the port number where your server exposes Prometheus metrics.

Make sure to adjust the configuration according to your server's setup and requirements.

After updating the configuration, save the file and restart the Prometheus server.
Once restarted, Prometheus will start scraping metrics from your server according
to the defined scrape configuration. You can verify that your server's metrics are
being scraped by checking the Prometheus targets page or querying Prometheus for
metrics from your server.

See `Prometheus configuration docs
<https://prometheus.io/docs/prometheus/latest/configuration/configuration>`_.

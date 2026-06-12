.. _kapa_ai:

Kapa.ai Documentation Assistant
###############################

The Zephyr documentation website is augmented with an AI assistant powered by Kapa.ai. The assistant
is trained on Zephyr's own documentation, source code, and GitHub activity (see `Data sources`_
below) and can answer free-form questions in natural language, citing the documentation pages and
resources it used to build each answer.

The same knowledge base is also exposed through a `Model Context Protocol
<https://modelcontextprotocol.io/>`_ (MCP) server, so that external AI assistants and IDEs can query
Zephyr knowledge directly (see `Using the MCP server`_).

.. note::

   Answers are AI-generated and may contain inaccuracies. Always verify important information
   against the official documentation, and treat the assistant as a starting point rather than an
   authoritative source.

Using the chatbot
*****************

The chatbot is available from every page of the documentation:

* Click the chatbot button displayed at the bottom of the page, or
* Press :kbd:`Ctrl+K` (:kbd:`Cmd+K` on macOS) to open it from anywhere.

The assistant opens in a sidebar where you can ask questions in plain language, for example *"How do
I configure a UART device in the devicetree?"* or *"What is the difference between a workqueue and a
thread?"*. Answers include links back to the relevant documentation pages, GitHub issues, or source
files so you can dig deeper.

The first time you use the assistant, a one-time consent screen explains how the feature works and
what data is collected. See the `Privacy`_ section below.

.. tip::

   You don't have to ask your questions in English. The assistant detects the language you use and
   responds in the same language, so you can interact with it in your preferred language (English,
   Chinese, Spanish, Japanese, French, German, and many others).

AI-powered search
*****************

In addition to the chatbot, Kapa.ai can power the documentation search box:

#. Click the gear (:guilabel:`Search settings`) icon next to the search field.
#. Select :guilabel:`Kapa AI search` from the menu.
#. Type your query and press :kbd:`Enter`.

Instead of the built-in keyword search, your query is answered by the AI assistant. Your selection
is remembered, so subsequent searches keep using the engine you picked until you change it back.

Using the MCP server
*********************

Kapa.ai also exposes the Zephyr knowledge base as an :abbr:`MCP (Model Context Protocol)` server.
MCP is an open standard that lets AI assistants and AI-enabled IDEs connect to external tools and
knowledge sources. By adding the Zephyr MCP server to your assistant, you give it the ability to
query up-to-date Zephyr documentation, source code, and GitHub activity on your behalf.

The server is available at the following endpoint, using the streamable HTTP transport:

.. code-block:: none

   https://zephyrproject.mcp.kapa.ai

The exact configuration steps depend on the MCP client you use. Most clients read a JSON
configuration file with an ``mcpServers`` section similar to the following:

.. code-block:: json

   {
     "mcpServers": {
       "zephyr-docs": {
         "url": "https://zephyrproject.mcp.kapa.ai"
       }
     }
   }

Some clients instead expect the server to be launched through a local stdio bridge such as
`mcp-remote <https://www.npmjs.com/package/mcp-remote>`_:

.. code-block:: json

   {
     "mcpServers": {
       "zephyr-docs": {
         "command": "npx", "args": ["-y", "mcp-remote", "https://zephyrproject.mcp.kapa.ai"]
       }
     }
   }

Refer to the documentation of your specific MCP client (AI coding assistant, IDE extension, etc.)
for the precise location and syntax of its configuration.

.. tip::

   Connecting the MCP server to an AI coding assistant lets it answer Zephyr questions and ground
   its code suggestions in the project's actual documentation and source, rather than relying solely
   on its training data.

Data sources
************

Kapa.ai builds its answers exclusively from the following Zephyr project sources, which are
synchronized at the indicated frequencies:

.. list-table::
   :header-rows: 1
   :widths: 25 50 25

   * - Source
     - Details
     - Refresh frequency
   * - Source code
     - ``zephyrproject-rtos/zephyr`` repository (excluding the ``boards/`` and ``doc/`` folders); C,
       Markdown, Python, and text files
     - Hourly
   * - API reference
     - https://docs.zephyrproject.org/latest/doxygen/
     - Daily
   * - Devicetree bindings
     - https://docs.zephyrproject.org/latest/build/dts/api/
     - Daily
   * - Main documentation
     - https://docs.zephyrproject.org/latest/ (excluding the API reference and Devicetree bindings
       sections)
     - Daily
   * - Project wiki
     - https://github.com/zephyrproject-rtos/zephyr/wiki
     - Daily
   * - GitHub issues
     - ``zephyrproject-rtos/zephyr`` issues from the last 6 months
     - Every 5 minutes
   * - GitHub pull requests
     - ``zephyrproject-rtos/zephyr`` pull requests from the last 6 months (excluding those closed
       without being merged)
     - Every 10 minutes

For the authoritative and most up-to-date configuration, refer to the `Kapa.ai page on the Zephyr
project infrastructure wiki <https://github.com/zephyrproject-rtos/infrastructure/wiki/Kapa.ai>`_.

Privacy
*******

The assistant uses your questions to provide answers based on the Zephyr documentation, source code,
and GitHub issues and pull requests. Questions and interactions may be anonymously collected and
analyzed to help improve the documentation by identifying areas that need clarification. No
personally identifiable information is collected.

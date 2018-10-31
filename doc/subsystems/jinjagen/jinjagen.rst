..
    Copyright (c) 2004-2015 Ned Batchelder
    SPDX-License-Identifier: MIT
    Copyright (c) 2018 Bobby Noelte
    SPDX-License-Identifier: Apache-2.0
    Copyright (c) 2018 Nordic Semiconductor ASA
    SPDX-License-Identifier: Apache-2.0
.. _jinjagen_intro:

Introduction
############

For some repetitive or parameterized coding tasks, it's convenient to
use a code generating tool to build C code fragments, instead of writing
(or editing) that source code by hand. Such a tool can also access CMake build
parameters and device tree information to generate source code automatically
tailored and tuned to a specific project configuration.

The Zephyr project supports a code generating tool that processes template files
and generates output files. It can be used, for example, to generate source code 
that creates and fills data structures, adapts programming logic, creates
configuration-specific code fragments, and more.

.. image:: template_flow.png
   :width: 500px
   :align: center
   :alt: Principle

The code generator uses a template engine for python called jinja2. The jinja2
templates are typically stored in files with the file extention .jinja2 which are
rendered into other files like c-source files. The documentation for jinja2 can 
be found here `Jinja2 <http://jinja.pocoo.org/>`_. Please refer to this site for more
extensive information, the information in the following sections are short
descriptions of some of the key features in jinja2. 

Jinja2 files are rendered into other files using the cmake extensions functions

* zephyr_sources_jinjagen(..)
* zephyr_library_sources_jinjagen(..). 

The generated source files are added to the Zephyr sources. While building a project
the jinja2 files are processed by jinja2 template engine and the generated source
files are written to the build directory.

A jinja2 template is written in a domain specific language. The language inherits
concepts from python, so anyone familiar with python should find it easy to learn
how to write jinja2 templates.

Data object
***********

All templates have access to a data object called ``data``. Through this data
object the template will have access to information, like the elements in
the .config file, 

.. code-block:: c

	{{ data['config']['CONFIG_BOARD'] }}

devicetree data,
 
.. code-block:: c

	{% for device in data['devicetree']['devices'] %}
	    {{ device }}
	{% endfor %}


and runtime information such as defines that have been injected using the JINJAGEN_DEFINES
key word in the CMakeLists.txt file.

.. code-block:: c

	{{ data['runtime']['defines']['MY_DEFINE'] }}

EDTS API object
***************

The EDTS database interface is exposed to all templates through a object called ``edts_api``.
Through this object can the EDTSConsumer interface be accessed inside the template. Some methods 
in the EDTSConsumer class returns EDTSDevice objects. The EDTSDevice interface will be
available on these objects.

.. code-block:: c

        {% set device = edts_api.get_device_by_device_id(device_id)  %}
        {% set label = device.get_property('label') %}


Template API
************

render_string()
===============

This function can be used to pre-render a jinja2-template formatted string with a custom context. 
The output will be inserted in the calling template as a rendered string.

.. code-block:: c

        {{ render_string("Hello {{ context['data'] }}", {'data':'World!'}) }}


Filters
*******

Filters are functions that can be applied to data in jinja2 templates and are used to
modify the data in one form or the other. The filters are applied to the data by using
the `pipe` character 

.. code-block:: c

    {{ "Hello %s\\n"|format("World!") }}


Built-in
========

Jinja2 has a set of built-in filters that in many cases will be familiar to anyone who
have written code in python. information about built in filter can be found here
`Built-in Filters <http://jinja.pocoo.org/docs/2.10/templates/#list-of-builtin-filters>`_.

.. code-block:: c

        {{ "fOOBAR!"|capitalize() }} => Foobar!

Custom
======

Filters can be added to the jinja2 engine by defining custom filters in the python
script that calls the jinja2 engine. This way functionality can easily be added to 
the jinja2 engine to tailor it for any particular application.

There is no square root filter built into jinja, but this can be added by using 
the ``sqrt()`` function in python to create a jinja2 filter. The python code should
be added to the render_template.py file in the scripts/ folder. 

Python code:

.. code-block:: python

        def squarerootfilter(value):
                return sqrt(value)

        environment.filters['sqrt'] = squarerootfilter

Jinja2 template:

.. code-block:: c
        
        Square root of 5: {{ 5|sqrt() }}


Extending and reusing templates
*******************************

Templates available in any of the search path folders can be reused/extend by other templates.
The path of the main template is automatically added to the search path list, but other 
folders can be added using the SEARCH_PATH key word in the CMakeLists.txt file.

.. code-block:: c

        zephyr_library_sources_jinjagen(template.h.jinja2 SEARCH_PATH "/some_folder_with_templates/")

Include
=======

Include works similarly to #include pre-processor directive in C, and will
insert the content in another template starting from the location of the include keyword.

.. code-block:: c

	{% include 'another_template.jinja2' %}


Import
======

The import keyword will import all macros defined in another template into a namespace,
similar to how import in python works. 

.. code-block:: c

	{% import 'template_with_macros.jinja2' as macros %}

	{{ macros.an_imported_macro() }}


Extends
=======

The extends keyword is used to extend and overload macros from another template, 
similar to how a child class extends a parent class in object oriented programming.

.. code-block:: c

	{% extends 'base_template.jinja2' %}

	{% macro added_macro() %}
	  This macro is from the child template
	{% endmacro %}

	{% block overload_content_in_parent_template %}
	  This is inserted into the parent template class block from the child template
	{% endblock %}

Block
-----

The block keyword can be used to define a block in the parent template that can be overloaded
by the child template. The child block can also include the content parent block by calling
super().

.. code-block:: c

    {# This is the parent template #}

    {% block a_block %}
      This is the content of the parent block
    {% endblock %}

    {# This is the child template #}
    {% extends 'parent_template.jinja2' %}

    {% block  a_block() %}
       This is the text from the parent block: {{ super() }}
       I add some text here!
    {% endblock %}

Python functions
================

Regular python functions can be exposed to the templates either through data objects or in
global scope. The exposed ``EDTS interface`` is an example how that can be done through a data
object while the ``Template API`` is and example how that can be done in global scope.


Code Generation in the Build Process
************************************

Code generation is invoked as part of the build process. A template file can be inserted into
the build by one of the following CMake functions.

.. code-block:: c

    zephyr_sources_jinjagen(jinjagen_file.c [JINJAGEN_DEFINES defines..] [SEARCH_PATH path1 [path2] [..]])

    zephyr_library_sources_jinjagen(jinjagen_file.c [JINJAGEN_DEFINES defines..] [SEARCH_PATH path1 [path2] [..]])

The arguments given by the ``JINJAGEN_DEFINES`` keyword have to be of the form
``define_name=define_value``. The arguments become a part of the data object and can be accessed through 
``data['runtime']['defines'][define_name]``

The template engine looks for templates in the current folder, and in folders listed by the 
``SEARCH_PATH`` keyword.


.. _security_standards:

Security standards and Zephyr
#############################

Historically, organizations managed cybersecurity independently, defining their own assessment and
response protocols. Today, governments are increasingly regulating this area through new mandates
and security standards. These standards define specific guidelines and compliance requirements for
connected products.

This section evaluates the implications of these security standards for the Zephyr project itself,
as well as downstream product developers. The goal is to provide developers with the information
needed to build certifiable, compliant products using Zephyr.

In the EU, the cybersecurity requirements of the Radio Equipment Directive (RED) have applied to
new radio equipment since August 1, 2025, and :ref:`EN 18031 <en_18031>` is the harmonized
standard for them. The :ref:`Cyber Resilience Act <cra_faq>` (CRA) extends cybersecurity
requirements to all products with digital elements, including wired devices, and adds obligations
for vulnerability handling and support periods. It applies in full from December 11, 2027, and the
Commission has announced that the RED cybersecurity requirements will be repealed once the CRA
applies. Until then, radio equipment has to meet the RED requirements, and much of the work done
for EN 18031 carries over to the CRA.

.. toctree::
   :maxdepth: 1

   cyber-resilience-act.rst
   en-18031.rst
   etsi-303645.rst

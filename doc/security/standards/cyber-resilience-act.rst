.. _cra_faq:

EU Cyber Resilience Act (CRA)
#############################

.. warning::
   This document is for informational purposes only and does not constitute legal advice.
   Consult with your legal counsel for compliance guidance specific to your situation.

Overview
********

The Cyber Resilience Act ([CRA24]_) is an EU regulation that establishes cybersecurity requirements
for products with digital elements (PDEs) placed on the EU market.
It entered into force on December 10, 2024.

.. admonition:: Key Dates
   :class: important

   * **June 11, 2026**: Assessment bodies operational
   * **September 11, 2026**: Manufacturers must report vulnerabilities and incidents
   * **December 11, 2027**: Full regulation applies

This page explains how the CRA relates both to manufacturers using Zephyr in commercial products,
and to the Zephyr Project itself in its role as an open-source software steward.

For Manufacturers Using Zephyr
******************************

Does the CRA apply to my product?
=================================

The CRA applies if you place a product with digital elements (PDE) on the EU market for commercial
purposes. This includes hardware devices with embedded software, and standalone software products.

Which category does my product belong to?
=========================================

The CRA classifies products into categories based on risk: **Important Products** (`Annex III`_) and
**Critical Products** (`Annex IV`_). Products not listed in either category are considered
**Default** products and have lower requirements.

For example, default products can typically rely on self-assessment (see :ref:`compliance_path`)
with fewer documentation and assurance requirements.

.. list-table::
   :header-rows: 1
   :widths: 15 25 60

   * - Category
     - Short description
     - Example Zephyr Use Cases
   * - Default
     - Products with digital elements that are not listed as "important" or "critical".
     - - Wi-Fi smart bulb or switch (e.g., running Matter over Thread/Wi-Fi).
       - Wearable activity tracker or smartwatch for personal wellness.
       - Bluetooth LE audio accessory or wireless sensor tag.
   * - Important (Class I)
     - Higher-risk products, often consumer-facing, performing security- or access-relevant
       functions.
     - - Smart door lock or access control reader for residential use.
       - Smart home hub or router managing network traffic.
       - Connected alarm system or security sensor.
   * - Important (Class II)
     - Higher-risk products used in enterprise/industrial/infra contexts or with privileged network
       roles.
     - - Industrial Programmable Logic Controller (PLC) or robot controller.
       - Micro-controllers with Secure Enclave/TEEs used for device identity.
       - Industrial IoT gateway performing edge processing.
   * - Critical
     - Products whose compromise could severely impact critical infrastructure or essential
       services.
     - - Smart electricity meter or water meter with remote shut-off.
       - Hardware Security Module (HSM) or smartcard firmware.
       - Safety-critical sensors used in energy or transport grids.

.. admonition:: Core Functionality vs. Integration
   :class: important

   Classification is determined by the **core functionality** of the final product, not by the
   individual components it integrates (`Article 7`_).

   * Integrating an important or critical component (e.g., a secure element, embedded browser)
     into another product does **not** automatically render that product important or critical.
   * A product that *can* perform the functions of an important/critical category but whose core
     functionality is different is **not** considered to have that core functionality.

   **Examples:**

   * If you build a network firewall using Zephyr, the core function is security, making the
     product an Important Product (Class II).
   * If you build a coffee machine using Zephyr, and you utilize Zephyr's networking stack features
     to protect the device, the core function remains making coffee. This is a "default" product.

   In a nutshell, using security-critical Zephyr features (like cryptography or secure boot) in a
   device does **not** elevate that device to a higher risk class.

For detailed classification, see `Annex III`_ and `Annex IV`_. The full list of product categories
with technical descriptions is provided in `Implementing Regulation (EU) 2025/2392`_.

.. _compliance_path:

Which compliance path must I choose?
====================================

The CRA defines different conformity assessment procedures based on your product category.
You must choose the path that corresponds to your classification and reliance on harmonized
standards.

.. list-table:: CRA Product Categories & Assessment Routes
  :widths: 20 55 25
  :header-rows: 1

  * - Category
    - Conformity Assessment Procedure
    - Third-Party Audit?
  * - Default
    - Module A (Internal Control). Self-assessment by the manufacturer.
    - No
  * - Important Class I
    - Module A **ONLY IF** harmonized standards are fully applied. Otherwise: Module B + Module C,
      or Module H.
    - Yes, if standards not fully used
  * - Important Class II
    - Module B + Module C, or Module H. (Self-assessment is **NOT** allowed).
    - Yes (Mandatory)
  * - Critical
    - **European Cybersecurity Certification** (e.g., EUCC) or Module B + Module C + Module H
      (pending delegate acts).
    - Yes (Mandatory)

The "Modules" refer to specific assessment procedures defined in `Decision No 768/2008/EC`_
("New Legislative Framework") and adapted by the CRA in `Annex VIII`_:

* `Module A`_ (Internal production control): You create the technical documentation, perform the
  risk assessment, and declare conformity yourself. No external auditor is required.
* `Module B`_ (EC-type examination) + `Module C`_ (Conformity to type): A Notified Body [#nb]_
  examines the technical design (Module B) and issues a certificate. You then ensure production
  conforms to that type (Module C).
* `Module H`_ (Full Quality Assurance): A Notified Body [#nb]_ audits your Quality Management System
  (QMS) governing design, production, and testing.

.. [#nb]  Notified Bodies will become operational by **June 11, 2026**.

What are my main obligations as a manufacturer?
===============================================

The CRA defines manufacturer obligations primarily in `Article 13`_ (product requirements and
due diligence) and `Article 14`_ (vulnerability handling and reporting). Regardless of a product's
classification, the following core obligations apply to all manufacturers of products with digital
elements.

**Risk assessment**
  Assess and document cybersecurity risks throughout the product lifecycle.

**Due diligence**
  Exercise due diligence when integrating third-party components (including open-source software
  like Zephyr).

**Vulnerability handling**
  Handle vulnerabilities for at least 5 years (support period), including receiving reports and
  applying updates.

**Incident reporting**
  Report actively exploited vulnerabilities and severe incidents to the relevant CSIRT and ENISA.

**Technical documentation**
  Create documentation per `Article 31`_ and `Annex VII`_.

**Conformity assessment**
  Assess conformity per `Article 32`_ and `Annex VIII`_.

**CE marking**
  Affix CE mark and draw up EU declaration of conformity.

What are the penalties for non-compliance?
==========================================

* Up to EUR 15,000,000 or 2.5% of worldwide annual turnover (`Article 13`_ & `Article 14`_
  violations).
* Up to EUR 10,000,000 or 2% of turnover (violations of other obligations).

.. _cra_vulnerability_reporting_obligations:

What are the vulnerability reporting obligations?
=================================================

The CRA distinguishes between "ordinary" vulnerabilities (handled through your normal vulnerability
management process) and cases that trigger strict notification timelines under `Article 14`_:
**actively exploited vulnerabilities** and **severe incidents**.

The tables below summarize the minimum reporting steps.

.. list-table:: Actively exploited vulnerabilities (`Article 14`_ (1) and (2))
   :header-rows: 1
   :widths: 20 20 60

   * - Step
     - Deadline
     - Content (minimum)
   * - Early warning
     - Within 24 hours of becoming aware
     - Notify via the single reporting platform to your CSIRT coordinator and ENISA that an actively
       exploited vulnerability affects your product. Indicate, where known, in which Member States
       the product is available.
   * - Vulnerability notification
     - Within 72 hours of becoming aware
     - Provide general information on the affected product, the general nature of the exploit and
       the vulnerability, corrective or mitigating measures already taken, and measures users can
       take. Indicate, where applicable, how sensitive you consider the notified information to be.
   * - Final report
     - No later than 14 days after a corrective or mitigating measure is available
     - Describe the vulnerability, its severity and impact, any information on malicious actors
       exploiting it (if available), and details on the security update or other corrective measures
       made available.

.. list-table:: Severe incidents (`Article 14`_ (3) through (6))
   :header-rows: 1
   :widths: 20 20 60

   * - Step
     - Deadline
     - Content (minimum)
   * - Early warning
     - Within 24 hours of becoming aware
     - Notify via the single reporting platform to your CSIRT coordinator and ENISA that a severe
       incident impacts the security of your product. Indicate whether it is suspected to be caused
       by unlawful or malicious acts and, where known, in which Member States the product is
       available.
   * - Incident notification
     - Within 72 hours of becoming aware
     - Provide general information on the nature of the incident, an initial assessment (including
       impact/severity as known at the time), corrective or mitigating measures already taken, and
       measures users can take. Indicate, where applicable, how sensitive you consider the notified
       information to be.
   * - Final report
     - Within 1 month after the incident notification
     - Provide a detailed description of the incident, including severity and impact, the type of
       threat or likely root cause, and applied and ongoing mitigation measures.

How can I obtain an SBOM (Software Bill of Materials)?
======================================================

Zephyr can automatically generate SBOMs for your application using the ``west spdx`` command.

See :ref:`west-spdx` for details on how to configure and use this tool.

How should I handle Zephyr vulnerabilities?
===========================================

As a manufacturer integrating Zephyr into a product, you remain responsible for vulnerability
management and, where applicable, CRA reporting.
Zephyr provides vulnerability information, but you must assess and act on it for your own product.

A practical workflow is:

1. **Stay informed**. Register to the `Zephyr Vulnerability Alert Registry`_ to receive
   notifications when vulnerabilities are disclosed.

2. **Assess impact**. For each advisory, use your SBOM and configuration to determine whether the
   affected Zephyr component is present, reachable, and security-relevant in your product.

3. **Plan remediation**. Decide on the appropriate response (e.g., apply a patch, adjust
   configuration, ...).

4. **Deploy fixes**. Integrate, test, and roll out the chosen fix, and update your SBOM and product
   documentation as needed.

5. **Meet reporting obligations**. If the vulnerability affects your product and is actively
   exploited, or leads to a severe incident, report it in line with the `Article 14`_ timelines and
   as per the previous section, :ref:`cra_vulnerability_reporting_obligations`.

What timelines does Zephyr follow for vulnerability handling?
=============================================================

Zephyr operates its own PSIRT process with target timelines for triage, notification, and
disclosure. These are *project* timelines, not legal deadlines for manufacturers.

Your CRA reporting obligations as per `Article 14`_ are triggered by when *you* become aware that
your product is affected by an actively exploited vulnerability or a severe incident.
This can be **earlier than** some of the Zephyr milestones below, meaning you might have to send an
early warning or incident report even before a Zephyr fix is available or before public disclosure.

Zephyr uses private GitHub security advisories and an embargo period (at most 90 days) to coordinate
fixes and disclosure. While the full process is described in :ref:`reporting`, the key milestones
are:

* **Within 7 days**: PSIRT feedback to initial reporter.
* **Within 30 days**: Manufacturers notified via alert registry and fix made available from the
  project.
* **Up to 90 days total**: Security-sensitive vulnerabilities are made public after embargo period.

Do I need to report vulnerabilities I find in Zephyr?
=====================================================

**Yes**, under `Article 13(6)`_, if you discover a vulnerability in a component (including Zephyr),
you **must** report it to the entity maintaining it. See :ref:`reporting`.

Additionally, consider voluntary reporting to CSIRT or ENISA per `Article 15`_.

For Zephyr as an Open Source Steward
************************************

What is Zephyr's role under the CRA?
====================================

Zephyr is an **"open-source software steward"** under `Article 3`_ (14): a legal person that
systematically provides sustained support for developing PDEs intended for commercial activities.

Zephyr's obligations under Article 24:

**Cybersecurity policy**
  Document security policies and vulnerability handling.

**Cooperation**
  Work with market surveillance authorities to mitigate risks.

**Incident reporting**
  Report actively exploited vulnerabilities and severe incidents affecting Zephyr's infrastructure
  (to the extent Zephyr is involved).

Does the CRA apply to Zephyr contributors?
==========================================

**No.** The CRA does not apply to individual contributors to Zephyr (`Recital 18`_).

Contributors developing features or fixing bugs are not subject to CRA obligations.

How is Zephyr meeting its steward obligations?
==============================================

`Article 24(1)`_: Security policy (Complete)
  * Documented at :ref:`security-overview`
  * Vulnerability reporting process: :ref:`reporting`
  * Secure coding guidelines: :ref:`secure code`

`Article 24(2)`_: Cooperation with authorities (In Progress)
  * Registered CVE Numbering Authority (CNA) since 2017
  * Active Zephyr Project Security Incident Response Team (PSIRT)
  * **In Progress**: Identifying CSIRT coordinator for EU

`Article 14(1)`_ & `Article 14(3)`_: Incident reporting (In Progress)
  * **In Progress**: Determining if NVD processes work for CSIRT/ENISA reporting
  * Plan to align with EU reporting requirements

`Article 14(8)`_: User notification (Complete)
  * Vulnerability Alert Registry for manufacturers and integrators
  * CVE publication and security advisories

`Article 52(3)`_: Corrective actions (Complete)
  * Established processes in place with CVE authorities
  * Timely response through PSIRT

External Resources
******************

Official CRA documentation
==========================

* `EU CRA Regulation 2024/2847
  <https://eur-lex.europa.eu/legal-content/EN/TXT/HTML/?uri=OJ:L_202402847>`_
* `Implementing Regulation (EU) 2025/2392`_ (Technical descriptions of important and critical
  product categories)
* `ENISA CRA Requirements-Standards Mapping
  <https://www.enisa.europa.eu/publications/cyber-resilience-act-requirements-standards-mapping>`_
* `European Commission CRA FAQ
  <https://digital-strategy.ec.europa.eu/en/faqs/cyber-resilience-act-questions-and-answers>`_

Standards and Technical Specifications
======================================

Relevant existing standards:

* `ETSI EN 303 645 <https://www.etsi.org/deliver/etsi_en/303600_303699/303645/>`_ - Cyber Security
  for Consumer Internet of Things: Baseline Requirements

ETSI is developing harmonized standards in response to the `CRA Standardisation Request (M/606)
<https://ec.europa.eu/growth/tools-databases/enorm/mandate/606_en>`_. Public draft standards
include product-specific requirements for:

* Operating Systems (prEN 304 626)
* Browsers (prEN 304 617)
* Password Managers (prEN 304 618)
* Firewalls (prEN 304 636)

For the complete list of draft standards and participation in public consultations, see the
`ETSI Cyber Resilience Act Portal <https://www.etsi.org/committee/1430-tc-cyber>`_.

Educational resources
=====================

* `Linux Foundation: Understanding the EU CRA
  <https://training.linuxfoundation.org/express-learning/understanding-the-eu-cyber-resilience-act-cra-lfel1001>`_
* `Linux Foundation CRA Readiness Report <https://www.linuxfoundation.org/research/cra-readiness>`_
* `Linux Foundation CRA Compliance Best Practices
  <https://www.linuxfoundation.org/research/cra-compliance-best-practices>`_
* `OpenSSF CRA Guidance <https://openssf.org/cra/>`_

Zephyr-specific resources
=========================

* :ref:`security-overview`
* :ref:`reporting`
* `Zephyr Vulnerability Alert Registry`_
* :ref:`Zephyr Vulnerabilities <vulnerabilities>`

..

.. _`Decision No 768/2008/EC`: https://eur-lex.europa.eu/legal-content/EN/TXT/HTML/?uri=CELEX:32008D0768
.. _`Module A`: https://eur-lex.europa.eu/legal-content/EN/TXT/HTML/?uri=CELEX:32008D0768#d1e41-98-1
.. _`Module B`: https://eur-lex.europa.eu/legal-content/EN/TXT/HTML/?uri=CELEX:32008D0768#d1e288-98-1
.. _`Module C`: https://eur-lex.europa.eu/legal-content/EN/TXT/HTML/?uri=CELEX:32008D0768#d1e439-98-1
.. _`Module H`: https://eur-lex.europa.eu/legal-content/EN/TXT/HTML/?uri=CELEX:32008D0768#d1e1719-98-1

.. _`CRA Requirements-Standards Mapping`: https://www.enisa.europa.eu/publications/cyber-resilience-act-requirements-standards-mapping

.. _`Recital 18`: https://eur-lex.europa.eu/legal-content/EN/TXT/HTML/?uri=OJ:L_202402847#rct_18

.. _`Article 3`: https://eur-lex.europa.eu/legal-content/EN/TXT/HTML/?uri=OJ:L_202402847#art_3
.. _`Article 7`: https://eur-lex.europa.eu/legal-content/EN/TXT/HTML/?uri=OJ:L_202402847#art_7
.. _`Article 13`: https://eur-lex.europa.eu/legal-content/EN/TXT/HTML/?uri=OJ:L_202402847#art_13
.. _`Article 13(6)`: https://eur-lex.europa.eu/legal-content/EN/TXT/HTML/?uri=OJ:L_202402847#013.006
.. _`Article 13(14)`: https://eur-lex.europa.eu/legal-content/EN/TXT/HTML/?uri=OJ:L_202402847#013.014
.. _`Article 14`: https://eur-lex.europa.eu/legal-content/EN/TXT/HTML/?uri=OJ:L_202402847#art_14
.. _`Article 14(1)`: https://eur-lex.europa.eu/legal-content/EN/TXT/HTML/?uri=OJ:L_202402847#014.001
.. _`Article 14(3)`: https://eur-lex.europa.eu/legal-content/EN/TXT/HTML/?uri=OJ:L_202402847#014.003
.. _`Article 14(8)`: https://eur-lex.europa.eu/legal-content/EN/TXT/HTML/?uri=OJ:L_202402847#014.008
.. _`Article 15`: https://eur-lex.europa.eu/legal-content/EN/TXT/HTML/?uri=OJ:L_202402847#art_15
.. _`Article 24(1)`: https://eur-lex.europa.eu/legal-content/EN/TXT/HTML/?uri=OJ:L_202402847#024.001
.. _`Article 24(2)`: https://eur-lex.europa.eu/legal-content/EN/TXT/HTML/?uri=OJ:L_202402847#024.002
.. _`Article 31`: https://eur-lex.europa.eu/legal-content/EN/TXT/HTML/?uri=OJ:L_202402847#art_31
.. _`Article 32`: https://eur-lex.europa.eu/legal-content/EN/TXT/HTML/?uri=OJ:L_202402847#art_32
.. _`Article 52(3)`: https://eur-lex.europa.eu/legal-content/EN/TXT/HTML/?uri=OJ:L_202402847#052.003

.. _`Annex III`: https://eur-lex.europa.eu/legal-content/EN/TXT/HTML/?uri=OJ:L_202402847#anx_III
.. _`Annex IV`: https://eur-lex.europa.eu/legal-content/EN/TXT/HTML/?uri=OJ:L_202402847#anx_IV
.. _`Annex VII`: https://eur-lex.europa.eu/legal-content/EN/TXT/HTML/?uri=OJ:L_202402847#anx_VII
.. _`Annex VIII`: https://eur-lex.europa.eu/legal-content/EN/TXT/HTML/?uri=OJ:L_202402847#anx_VIII

.. _`Implementing Regulation (EU) 2025/2392`: https://eur-lex.europa.eu/legal-content/EN/TXT/HTML/?uri=CELEX:32025R2392

.. _`Zephyr Vulnerability Alert Registry`: https://www.zephyrproject.org/vulnerability-registry/

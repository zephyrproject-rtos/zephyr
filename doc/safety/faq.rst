Safety FAQ
##########

.. admonition:: Last update

   July 2026.

What is Safety?
******************

    In some industries, failure of a system could mean harm to its users, operators, 3rd parties or the environment.

    To prevent unreasonable risk due to hazards caused by failure and/or unintended behavior of such safety critical systems, each industry has created standards.

    These standards share the same foundations, but each have requirements of their own. You will find some of these standards mentioned in the scope section below. These standards include, but not limited to, definition of: terminology, activities to be performed, organizational/project requirements, independence requirements, evidence of lifecycle execution requirements... 

    Safety lifecycle activities are added on top of the typical software lifecycle activities to ensure that the software is operating as designed as well as designed to prevent and/or reduce the risk of failures and/or mitigate their effect.

Why does Zephyr need safety?
****************************

   Some companies are looking at using Zephyr for applications in machinery, automotive, appliances, medical, which is why the Zephyr RTOS project is looking at strengthening and enhancing its software lifecycle to comply with the respective industry standards.

Why IEC 61508?
**************

   IEC 61508 is used as a baseline for many other standards. As such IEC 61508 is a good reference standard to start with and for any other functional safety standards that the Zephyr project will want to achieve.

#. **How can I contribute?**

   Follow the instructions given at Safety Working Group · zephyrproject-rtos/zephyr Wiki and for meetings, current areas of contribution and useful links and consult Safety — Zephyr Project Documentation for guidelines.

#. **Who can contribute to the Zephyr safety effort?**

   Anyone willing to!

#. **What skills or experience do I need to contribute?**

   The verification process and roles are such that no contribution will make it to the baseline without proper verification. As such, it is preferred but not mandatory to have training and/or experience at the relevant V-model - Wikipedia process for Zephyr and/or the process itself to contribute. However, it is required that you make yourself familiar with 1) the specific process you are contributing towards (e.g. SW requirements, Architecture, Design, Code, Test) and 2) the ancillary processes (e.g. configuration management, verification review process, raising issues).

#. **What is left to be done?**

   As of January 2026 processes are being defined for the community to contribute on all levels of the safety lifecycle / v model and only the requirement process is officially available. All artifacts still need to be created/updated to meet safety standards requirements: requirements, architecture (i.e. as documented in Zephyr documentation Kernel — Zephyr Project Documentation), design (i.e. as documented in code through Doxygen Zephyr API Documentation: Introduction), code, tests and other safety work products.

Where can I see progress?
*************************
   Work is listed in github issues and important milestones will be communicated to the safety working group as they are being planned and completed. For the moment, there is no publicly available dashboard to show overall progress on the Zephyr RTOS certification effort.

When will Zephyr RTOS be certified / qualified for use in safety critical applications?
***************************************************************************************
   As of 2025, Zephyr RTOS safety committee chair, safety manager and safety architect are targeting to get an end-to-end example of the Zephyr certification practice and artifacts for 2026. Completion of the Zephyr project certification depends heavily on community and company contributions and may not happen before 2027 or later. Come and help us make it real!

#. **For the industrial domain?**

       The current target is for IEC 61508.

#. **For the automotive domain?**

       There is interest from the Zephyr active community members to reach compliance to ISO 26262 but no firm target date yet. There is a contract with Zephyr project’s assessor and an objective to certify to ISO 26262 ASIL D.

#. **For the medical domain?**

       There is interest from the Zephyr active community members to reach compliance to ISO 62304 / ISO 14971 but no firm target yet.

#. **For home appliances domain?**

       There is interest from the Zephyr active community members to reach compliance to IEC 60730 / IEC 60335 but no firm target yet.

#. **For the rail domain?**

       The Zephyr team has no plan to reach compliance to EN 50176 (that replaced EN 50128) at the moment.

#. **For the aerospace domain?**

       The Zephyr team has no plan to reach compliance to DO-178C at the moment.

What is the scope of Zephyr RTOS certification?
***********************************************
   The initial scope of Zephyr RTOS certification is IEC 61508 SIL 3. The scope includes all Kernel components. Additional components and configuration matrices are still being refined. Specific target hardware, devices and compiler are not in scope of the Zephyr project certification. An initial scope is defined there: Add Requirements for the Zephyr project · Issue #68005 · zephyrproject-rtos/zephyr

#. **Up to what SIL / ASIL / DAL will the Zephyr RTOS be certified?**
   SIL 3 is said to be equivalent to:
   #. **Home appliance:** (to be defined later)

   #. **Automotive:** ISO 26262 ASIL D, however, the Zephyr team is not planning on performing MC/DC coverage as prescribed by ASIL D, which would result in a gap (among others) for compliance to ISO 26262 ASIL D requirements

   #. **Medical:** Although IEC 61508 excludes medical devices from its scope, the requirements medical device manufacturers have to fulfill according to IEC 62304 are similar and in many cases direct mappings can be established. That said, medical device manufacturers still have to treat Zephyr as SOUP and perform validation activities according to their development procedures.

   #. **Aerospace:** DO-178C DAL B

#. **What target hardware / SoC will Zephyr RTOS be certified on?**

   The Zephyr team is not targeting any specific HW for certification. See section on how to use the Zephyr RTOS safety evidence for more information on that topic.

#. **Will I need to qualify a compiler for building Zephyr RTOS / my applications?**
   Yes, as an integrator you will need to qualify your own compiler for the platform you are targeting. The Zephyr project team is not providing binaries for use in your environment but only source code.

#. **Will I need to qualify library X (Rust, vendor X AI/ML, C++ X libraries...) for my application?**

   If the library is not in the scope of certification, you will have to qualify it for your application use. We qualify library interfaces as far as they are in the scope.

How can I get / use the Zephyr RTOS safety evidence / documentation?
********************************************************************

   Safety manual instructions (available to specific tiered members only) will provide details on how to leverage the Zephyr safety lifecycle artifacts for use in the context of a specific HW or application. Contact the Zephyr project team for more details Contact – Zephyr Project

#. **How can I get the Zephyr RTOS safety evidence / documentation for my use?**

    The Zephyr safety evidence / documentation will be provided to specific tiered members. Contact the Zephyr project team for more details Contact – Zephyr Project

#. **How can I certify / qualify Zephyr RTOS for my use / my application?**

    See section answer above. Additionally, if you perform any modification to the Zephyr RTOS baseline, you will have to evaluate impacts and, if needed, re-execute safety lifecycle activities.

#. **How can I certify / qualify Zephyr RTOS for my target HW?**

    See section answer above.

#. **How can I certify / qualify my applications running on Zephyr RTOS?**

    As the integrator, you have the onus of qualifying Zephyr RTOS for your application as well as certifying your application and the overall system running it for safety. Typically, companies either:

    #. Use their own set of processes to certify their systems, including Zephyr RTOS
    #. Contract other companies to develop safety lifecycle artifacts based on their software / system

Zephyr RTOS footprint in Safety applications
********************************************

If you want to understand interest around Zephyr RTOS for safety applications, we encourage you to have a look at Safety Working Group · zephyrproject-rtos/zephyr Wiki for: The safety working group minutes and attendance The Discord safety channel

#. **Are there any companies currently using Zephyr RTOS for safety critical applications?**

Watch this space!
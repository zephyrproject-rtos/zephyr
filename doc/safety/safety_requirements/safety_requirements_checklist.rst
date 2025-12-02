.. _safety_requirements_checklist:

Safety Requirements Checklist
#############################

Introduction
************

The requirements that are created, modified or updated need to be suitable. This means they need to be unambiguous, uniquely idendifiable, feasible, testable etc.
To ensure this, each requirement needs to be reviewed in its pull request, if it fulfills the basic requirements to be a requirement.
This checklist shall serve as the minimum that needs to be considered when doing a review of a requirement PR.

Requirements Structure and Guidelines
*************************************

You can find the approach taken by the Zephyr Project in :ref:`safety_requirements`

Review Instructions
*******************

This checklist lists the topics and questions that shall be considered when approving requirements during a review.
The review outcome must be documented in the PR review, in a way that it is clear that this checklist has been used and the actually outcome of the review regarding
open questions, findings or approval.
This checklist shall be applied at least once for new or changed requirements within the scope of a safety assessment before the safety scope is released.


The requirements are gathered in the separate repository:
`Requirement repository
<https://github.com/zephyrproject-rtos/reqmgmt>`__

The review can be done on two levels - regarding a single requirement or as a group of requirements. The checkpoints are as following:

Review checkpoints for each and every requirement
*************************************************

Each new or changed requirement needsto be analysed, to make sure it is suitable and to identify if it is affecting the safety of Zephyr in any kind.

.. list-table:: Checkpoints for requirements review
   :widths: 20 80 80 120
   :header-rows: 1

   * - ID
     - Question
     - Explanation
     - Example

   * - VerReq_1_0
     - Is this requirement clearly comprehensible and unambiguous?
     - To make sure the requirement can be analysed it must be easy to understand and leave no room for (functional) interpretation.
     - Avoid vague statements such as **some, sufficient, typical, many, several, few, ...**. Statements like **The Zephyr Kernel should provide functionaliy xyz where possible** are not accurate enough and leave room for interpretation. Also **should** must be replaced with the unambigous **shall**. The **where possible** must be replaced with a definition where the author of the requiremement wants the funtionality to be provided. This can be a list of cases, system states or other conditions. As long as they are clearly defined.

   * - VerReq_1_1
     - Is this requirement atomic?
     - The requirement really must only describe one thing that needs to be fulfilled. Otherwise things like safety analysis, unique linking to architectural components and implementation and verifibility can be impaired.
     - Statements like **The signal a shall switch on and off the yellow LED and the error state shall be indicated by switching on the red LED** are not atomic. This needs to go into two requirements: **REQ1: The signal a shall switch on and off the yellow LED.**and **REQ2: The error state shall be indicated by switching on the red LED.** This also can be suplemented by a **REQ3(variant a): Yellow LED and the red Error LED must be separate** or **REQ3(varianb b): Yellow LED and red Error LED can be combined in case of TBD**.

   * - VerReq_1_2
     - Is the requirements internally consistent?
     - The requirement itself must be fully consistent within itself.
     - Statements like **The system shall use a preemtive RTOS to guarantee deterministic task scheduling. The system also need to ensure that all tasks, including low-priority background logging tasks, complete without preemption to preserve data integrity.** are contradicting. Stuff like this must be avoided. If both options shall be available, you must define the conditions for these options and how to configure the options.

   * - VerReq_1_3
     - Is the requirement feasible, achievable and verifiable?
     - Requirements are not a fantasy wish-list, they need to be able to reach with reasonable matter (although what is reasonable can differ between projects for sure). So generally this means something that sounds good on paper but is technically impossible, internally contradictory, or violates fundamental constraints of the system. Verifiable here means that you need to be able to verify, either by test, (code-) review, analysis or other means that this requirement has been met.
     - A bad example is e.g. **The RTOS shall guarantee zero‑latency interrupt handling for all devices under all conditions.**, which is physically impossible, as no hardware can respond in litterally zero time. Better would be something more concrete like **The RTOS shall guarantee an interrupt latency of ≤ 5 µs on the target HW architecture at 80 MHz under maximum system load.**

   * - VerReq_1_4
     - Is the requirement complete?
     - Is all information that needs to be defined to clearly state the intention of the requirement available?
     - The statement must either clearly state all needed data and information or give a reliable reference to where to find that data. This means that referenced data sources have to be under control of the requirement author or stakeholder, references that might change over time, like internet URLs to external sources are not suitable.

   * - VerReq_1_5
     - Is the requirement's definition free from any implementation?
     - Requirements must describe the "what" not the "how" something is done. There are corner cases where the requirement intentionally restricts the implementation variants, usually for some desgin reasons, these reasons then must be stated in the requirement rational or comment (whatever is used)
     - Avoid statements like **The RTOS shall use a priority-based preemptive scheduler implemented with a round-robin algorithm for tasks of equal priority**, consider instead to focus on the functional needs **The RTOS shall support task scheduling that ensures higher-priority tasks can preempt lower-priority tasks, and tasks of equal priority are scheduled in a manner that prevents starvation.**

   * - VerReq_1_6
     - Is the (safety/security) criticality of the requirements defined?
     - All requirements that affect a function that can compromise Zephyr's safety (or security) integrity needs to be marked as safety (security) critical so it can be handled accordingly.
     - Currently there is no specific way defined to mark a function as safety or security relevant, a requirement's or function's relevance is based on the scope that it is defined.

   * - VerReq_1_7
     - Are there verification and/or acceptance criteria for each requirement defined?
     - Each requirement must be verifieable. If you cannot define acceptance criteria for your requirement, the requirement needs to be reconsidered and maybe defined differently. Acceptance critieria must at least be derivable from how the requirement is written.
     - Make sure there is either a test available or that creating a test is planned. Evaluate if the test coverage is sufficient for this requirement. If is not sufficient, contact the Safety Working Group or Testing Working group to get this addressed.

   * - VerReq_1_8
     - Are all terms used in this requirement clear to the intended audience?
     - This targets both accuracy and comprehensibility of the requirement. It needs to be clear, what is meant by the terms used in this requirement. Terms that have
       a special meaning for the project need to be clearly defined in a glossary or along with the requirement.
     - The statement **The RTOS shall support IPC mechanisms such as mailboxes and semaphores for task synchronization** could be problematic, as the audience (testers, managers, POs etc.) might not be familiar with IPC or even what is meant by mailbox or semaphores in the context of an RTOS. If there is no glossary that explains these terms to all stakeholders, consider phrasing this requirement more like **The RTOS shall provide mechanisms that allow tasks to exchange data and coordinate their execution, such as message-passing and signaling features.**

Review checkpoints looking at a group of requirements
*****************************************************

.. list-table:: Checkpoints for requirements review
   :widths: 20 80 80 120
   :header-rows: 0

   * - ID
     - Question
     - Explanation
     - Example

   * - VerReq_2_0
     - Are the requirements appropriately grouped and structured?
     - Usually a suitable requirements structure follows a hierarchical order and within the hierarchical layers the requirements are grouped e.g. regarding their functionality, criticality, parent requirement's source etc.
     - An unsuitably grouped requirement is e.g. in the wrong section, or in a wrong hierarchy level (e.g. in systems requirements although the requirement defines a specific property of a kernel component and therefore has to be on software component level), sections also might be overloaded and need more structuring in sub-sections.

   * - VerReq_2_1
     - Is each requirement externally consistent?
     - he requirements must not conflict or contradict each other.
     - Consider having two requirements: **Requirement A: The RTOS shall support a maximum of 10 concurrent tasks. Requirement B: The RTOS shall support up to 20 concurrent tasks for high-performance applications.** - These two requirements clearly contradict each other, but might both be correct for the persons writing each one of them. Usually havinig requirements like this needs communication between the authors of the requirements, and a possible solution might be the following: **The RTOS shall support up to 20 concurrent tasks, with a default configuration of 10 tasks for standard applications.**

   * - VerReq_2_2
     - Is each requirement suitably traceable?
     - Each requirement needs to have a clear source (where does it come from, why is it here...), associated verification and/or acceptance criteria and descending requirements or architectural elements.
     - A requirement always has to have a parent requirement linked, except for the highest level requirements, but still there needs to be an explanation in the Requirement rationale, note or comment.

   * - VerReq_2_3
     - Is the requirement unique?
     - Requirements must not be dublicated. If you need a requirement to clearify things in multiple locations, make sure one is the "main requirement" and the others are just linked information.
     - This is self explanatory, two same or similar requirements must be avoided.

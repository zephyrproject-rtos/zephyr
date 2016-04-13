.. _changes:

Submitting a Change to Gerrit
#############################

Carefully review the following before submitting a change. These
guidelines apply to developers that are new to open source, as well as
to experienced open source developers.

Change Requirements
*******************

This section contains guidelines for submitting code changes.
For more information on how to submit a change using Gerrit, please
refer to :ref:`gerrit`.

Changes are submitted as Git commits. Each commit must contain:

* a short and descriptive subject line that is 72 characters or fewer,
  followed by a blank line.
* a change description with your logic or reasoning for the changes,
  followed by a blank line
* a Signed-off-by line, followed by a colon (Signed-off-by:)
* a Change-Id identifier line, followed by a colon (Change-Id:). Gerrit won't
  accept patches without this identifier.

A commit with the above details is considered well-formed.

All changes and topics sent to Gerrit must be well-formed. Informationally,
commit messages must include:

* **what** the change does,
* **why** you chose that approach,
* **what** assumptions were made, and
* **how** you know it works -- for example, which tests you ran.

Commits must build cleanly when applied in top of each other, thus
avoiding breaking bisectability. Commits must pass the
:file:`scripts/checkpatch.pl` requirements. For further detail, see the
:ref:`coding_style` section. Each commit must address a single identifiable
issue and must be logically self-contained.

For example: One commit fixes whitespace issues, another renames a
function and a third one changes the code's functionality.  An example commit
file is illustrated below in detail:

.. raw::
   doc: Updating tables presentation.

   Tables were misaligned; fixed spacing.

   Change-Id: IF7b6ac513b2eca5f2bab9728ebd8b7e504d3cebe1
   Signed-off-by: commit-sender@email.address

Each commit must contain the following line at the bottom of the commit
message:

Signed-off-by: your@email.address

The name in the Signed-off-by line and your email must match the change
authorship information. Make sure your :file:`.git/config` is set up
correctly. Always submit the full set of changes via Gerrit.

When a change is included in the set to enable the other changes, but it
will not be part of the final set, let the reviewers know this. Use
:abbr:`RFCs (requests for comments)` to send work proposals, progress snapshots
of your work, or to get early feedback on features or changes that will
affect multiple areas in the code base.

Before submitting, ensure each commit conforms with the coding and contribution
guidelines of the project found in `Change Requirements`_.

If you are submitting a change that has someone else's Signed-off-by, please be
sure that you explicitly include that person as a Reviewer in Gerrit.
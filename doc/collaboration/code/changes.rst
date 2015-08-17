.. _changes:

Submitting a Change to Gerrit
#############################

Read the following information carefully before submitting a change. It
applies for developers that are new to open source as well as for
experienced open source developers.

Change Requirements
******************

Here you can find the required format and content for changes. For more
information on how to submit a change using Gerrit please refer to
:ref:`gerrit`.

Changes are submitted as Git commits. Each commit must contain a short descriptive subject line,
typically less than 72 characters, a change description, and a Signed-off-by line. A commit with
all three is considered well formed. A group of related commits, a topic, must have all three plus
a cover letter to be considered well formed.

All changes and topics sent to Gerrit must be well formed. Commit
messages must include “what” the change does, “why” you chose that
approach, what assumptions you made and “how” you know it works, for
example, which tests you ran.


Commits must build cleanly when applied in top of each other, thus
avoiding breaking bisectability. Commits must pass the
:file:`scripts/checkpatch.pl` requirements. For more details see the
:ref:`style` section. Each commit must address a single identifiable
issue and must be logically self contained.

For example: One commit fixes whitespace issues, another renames a
function and a third one changes the code's functionality.

Every commit must contain the following line at the bottom of the commit
message:

Signed-off-by: your@email.address

The name in the Signed-off-by line and your email must match the change
authorship information. Make sure your :file:`.git/config` is set up
correctly. Always submit the full set of changes via Gerrit.

When a change is included in the set to enable the other changes but it
will not be part of the final set, let the reviewers know this. Use
:abbr:`RFCs (requests for comments)` to send work proposals, progress snapshots
of your work or to get early feedback on features or changes that will
affect multiple areas in the code.

Before you submit, ensure each of your commits conforms with the
coding and contribution guidelines of the project found in
`Change Requirements`_.

If you are submitting a change that has someone else's Signed-off-by make
sure that you include that person as a Reviewer in Gerrit.
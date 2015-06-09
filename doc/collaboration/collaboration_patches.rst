Submitting a Patch
##################

Read the following information carefully before submitting a patch. It
applies for developers that are new to open source as well as for
experienced open source developers.

New to Open Source?
*******************

Welcome to open source software development. Whether you are an
experienced developer within a corporate environment, a beginner trying
out something new or someone in between, we thank you for your interest
in the development of the |project|.

Before you begin we suggest you read the `Patch Philosophy`_.

.. _`Patch Philosophy`: http://kernelnewbies.org/PatchPhilosophy

Be sure to read the documentation before submitting any patches.

It is of great importance that each patch includes a Signed-off-by line,
containing your full legal name. Adding your Signed-off-by line means
you assert this patch meets the legal requirements of the
`developer certificate of origin`_. Read and understand that document
before contributing code or documentation to the project.

.. _`developer certificate of origin`: http://developercertificate.org/

Patch Requirements
******************

Here you can find the required format and content for patches. For more
information on how to submit a patch using Gerrit please refer to
`Submitting a Patch Via Gerrit`_.

Each patch must contain a short descriptive subject line, typically less
than 72 characters, a patch description, and a Signed-off-by line. A
patch with all three is considered well formed. A patch set must have
all three plus a cover letter to be considered well formed.

All patches and patch sets sent to Gerrit must be well formed. Commit
messages must include “what” the patch does, “why” you chose that
approach, what assumptions you made and “how” you know it works, for
example, which tests you’ve run.


Commits must build cleanly when applied in top of each other, thus
avoiding breaking bisectability. Commits must pass the
:file:`scripts/checkpatch.pl` requirements. For more details see the
:ref:`Coding` section. Each commit must address a single identifiable
issue and must be logically self contained.

For example: One commit fixes whitespace issues, another renames a
function and a third one changes the code's functionality.

Every commit must contain the following line at the bottom of the commit
message:

Signed-off-by: your@email.address

The name in the Signed-off-by line and your email must match the patch
authorship information. Make sure your *.git/config* is set up
correctly. Always submit the full patchset via Gerrit.

When a patch is included in the set to enable the other patches but it
will not be part of the final set, let the reviewers know this. Use
requests for comment (RFCs) to send work proposals, progress snapshots
of your work or to get early feedback on features or changes that will
affect multiple areas in the code.

Before you submit, please ensure each of your commits conforms with the
coding and contribution guidelines of the project found in
`Patch Requirements`_.

If you are submitting a patch that has someone else's Signed-off-by make
sure that you include that person as a Reviewer in Gerrit.

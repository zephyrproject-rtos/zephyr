.. _gerrit_practices:

Gerrit Recommended Practices
############################

This document presents some best practices to help you use Gerrit more effectively.
The intent is to provide you information about submitting content in an easier way.
Use the recommended practices to reduce your troubleshooting time and improve your participation
within the community.

Browsing the Git Tree
*********************

Visit `Gerrit`_, then select :menuselection:`Projects --> List --> SELECT-PROJECT --> Branches`.
Select the branch that interests you, click on :guilabel:`gitweb` located on the right hand side.
Now, :program:`gitweb` loads your selection on the Git web interface and redirects you.

.. _Gerrit: http://oic-review.01.org/gerrit/

Watching a Project
******************

Visit `Gerrit`_, select :guilabel:`Settings`, located on the top right corner.
Select :guilabel:`Watched Projects` and then add any projects that interest you.


Commit Messages
***************

Gerrit follows the Git commit message format.
Ensure the headers are at the bottom and don't contain blank lines between one another.
The following example shows the format and content expected in a commit message:::

   Subsystem: Brief one line description.

   Summary of the changes made referencing why, what and how.
   For documented code reference what part of the code the change is applied.

   Change-Id: LONGHEXHASH
   Signed-off-by: Your Name your.email@example.org
   AnotherExampleHeader: An Example of another Value

The Gerrit server provides a precommit hook to autogenerate the Change-Id which is one time use.
Use the following command as an example:

.. code-block:: bash

   $ scp -p -P 29418 GERRIT-SERVER:hooks/commit-msg REPODIR/.git/hooks/

You just need to enter the command above one time.


Avoid Pushing Untested Work to a Gerrit Server
***************************************

To avoid pushing untested work to Gerrit, we recommend you follow these steps:

1. Make the leak difficult to occur:

   - Change the name of the remote Git tree from *origin* to *another name*.
     This complicates unintentionally pushing work to Gerrit.

   .. code-block:: bash

      $ git remote rename origin another-name

   - Use `precommit hooks`_ to scan for problematic words in your commit.
     Follow the installation instructions in the :file:`README.rst` for check patch.
     Update the checkarray with keywords that might signal danger in your commits.

   .. _precommit hooks: https://github.com/niden/Git-Pre-Commit-Hook-for-certain-words

2. Think before you act:

   - Check your work at least three times before pushing your change to Gerrit.
     Be mindful of what information you are publishing.

Keeping Track of Changes
************************

* Set Gerrit to send you email:

   - Gerrit will subscribe you to the mailing list created for that change if a developer adds you
     as a reviewer or you comment on a specific Patch Set.
* Opening a change in Gerrit's review interface is as a quick way to follow that change.
* Watch projects in the Gerrit projects section at `Gerrit`_, select at least
   *New Changes, New Patch Sets, All Comments* and *Submitted Changes*.


Emails contain some helpful headers for filtering:

* **In-Reply-To:** used for threading.
   The following platforms may or may not use this header for filtering:

   - iPhone - OK.
   - Evolution - OK.
   - Thunderbird - OK.
   - Outlook - Not supported.

* **X-Gerrit-MessageType:** comment, newpatchset, etc.
* **Reply-To:** Replies to whom actions caused the email to be sent.

  - Autobuilders usually look like ``sys_EXAMPLE@intel.com``

Always track the projects you are working on; also see the feedback/comments mailing list
to learn and help others ramp up.


Topic branches
**************

Topic branches are temporary branches that you push to commit a set of
logically grouped dependent commits:

To push changes from :file:`REMOTE/master` tree to Gerrit for being reviewed as a topic
in  **TopicName** use the following command as an example:

.. code-block:: bash

   $ git push REMOTE HEAD:refs/for/master/TopicName

The topic will show up in the review :abbr:`UI` and in the :guilabel:`Open Changes List`.
Topic branches will disappear from the master tree when its content is merged.


Creating a Cover Letter for a Topic
===================================

You might decide if you want the cover letter to appear in the history or not.

1. To make a cover letter that appears in the history, use this command:

.. code-block:: bash

   $ git commit --allow-empty

Edit the commit message, this message then becomes the cover letter.
The command used doesn't change any files in the source tree.

2. To make a cover letter that doesn't appear in the history follow these steps:
   Put the empty commit at the end of your commits list so it can be ignored
   without having to rebase. Now add your commits

   .. code-block:: bash

      $ git commit ...
      $ git commit ...
      $ git commit ...

Then push the commits to a topic branch, use the following command as an example:

.. code-block:: bash

   $ git push REMOTE HEAD:refs/for/master/TopicName

If you already have commits but you want to set a cover letter, create an empty commit for
the cover letter and move the commit so it becomes the last commit of the list. Use the following
command as an example:

.. code-block:: bash

   $ git rebase -i HEAD~#Commits

Be careful to uncomment the commit before moving it.
:makevar:`#Commits` is the sum of the commits plus your new cover letter.


Finding Available Topics
========================

.. code-block:: bash

   $ ssh -p 29418 oic-review.01.org gerrit query \ status:open project:forto-collab branch:master \
   | grep topic: | sort -u

* *oic-review.01.org* Is the current URL where the project is hosted
* *status* Indicates the topic's current status: open , merged, abandoned, draft, merge conflict.
* *project* Refers to the current name of the project, in this case forto-collab
* *branch* The topic is searched at this branch.
* *topic* The name of an specific topic, leave it blank to include them all.
* *sort* Sorts the found topics, in this case by update (-u).

Downloading or Checking Out a Change
************************************

In the review UI, on the top right corner, the **Download** link provides a list of commands and
hyperlinks to checkout or download diffs or files.

We recommend the use of the *git review* plugin.
The steps to install git review are beyond the scope of this document.
Refer to the `git review documentation`_ for the installation process.

.. _git review documentation: https://wiki.openstack.org/wiki/Documentation/HowTo/FirstTimers

In general, to check out the change using Git use the following command:

.. code-block:: bash

   $ git review -d CHANGEID

If you don't have Git-review installed use the following commands:

.. code-block:: bash

   $ git fetch REMOTE refs/changes/NN/CHANGEIDNN/VERSION \ && git checkout FETCH_HEAD

For example, for the 4th version of change 2464, NN is 24 (the first two digits):

.. code-block:: bash

   $ git fetch REMOTE refs/changes/24/2464/4 \ && git checkout FETCH_HEAD


Using Draft Branches
********************

You can use draft branches to add certain reviewers before you publish your change.
The Draft Branches are pushed to :file:`refs/drafts/master/TopicName`

The next command ensures a local branch is created:

.. code-block:: bash

   $ git checkout -b BRANCHNAME


The next command pushes your change to the drafts branch under **TopicName**:

.. code-block:: bash

   $ git push REMOTE HEAD:refs/drafts/master/TopicName



Using Sandbox Branches
**********************

You can create your own branches for features development.
The branches are pushed to the :file:`refs/sandbox/USERNAME/BRANCHNAME` location.

The next commands ensures the branch is created in Gerrit's server.

.. code-block:: bash

   $ git checkout -b sandbox/USERNAME/BRANCHNAME

.. code-block:: bash

   $ git push --set-upstream REMOTE HEAD:refs/heads/sandbox/USERNAME/BRANCHNAME

Usually, the process to create content is: develop the code, break the information
into small commits, then submit changes, apply feedback and finally, rebase.

The next command pushes forcibly without review

.. code-block:: bash

   $ git push REMOTE sandbox/USERNAME/BRANCHNAME

You can also push forcibly with review

.. code-block:: bash

   $ git push REMOTE HEAD:ref/for/sandbox/USERNAME/BRANCHNAME


Updating the Versions of a Change
*********************************

During the review process, you might be asked to update your change.
It is possible to submit multiple versions of the same change.
Each version of the change is called a patch set.

Always maintain the **Change-Id** that was assigned.
For example, there is a list of commits, **c0...c7**, which were submitted as a topic branch:

.. code-block:: bash

   $ git log REMOTE/master..master

   c0
   ...
   c7

   $ git push REMOTE HEAD:refs/for/master/SOMETOPIC

Now, you get reviewers' feedback and there are changes in **c3** and **c4** that must be fixed.
If the fix requires rebasing, rebasing changes the commit Ids, see the :ref:`rebasing` section
for more information. However, you must keep the same Change-Id and push the changes again:

.. code-block:: bash

   $ git push REMOTE HEAD:refs/for/master/SOMETOPIC

This new push creates a patches revision, your local history is then cleared.
However you can still access the history of your changes in Gerrit on the :guilabel:`review UI`
section, for each change.

It is also allowed to add more commits when pushing new versions.

.. _rebasing:

Rebasing
********

Usually rebasing is the last step before pushing changes to Gerrit,
this allows you to make the necessary *Change-Ids*.  The *Change-Ids* must be kept the same.

* **squash:** mixes two or more commits into a single one.
* **reword:** changes the commit message.
* **edit:** changes the commit content.
* **reorder:** allows you to interchange the order of the commits.
* **rebase:** stacks the commits on top of the master.

For more information you can visit `Atlasian`_ , `git book`_  and `git rebase`_.

.. _Atlasian: https://www.atlassian.com/git/tutorials/rewriting-history/
.. _git book: http://git-scm.com/book/en/v2/Git-Branching-Rebasing
.. _git rebase: http://www.slideshare.net/forvaidya/git-rebase-howto

Rebasing During a Pull
**********************

Before pushing a rebase to your master, ensure that the history has a consecutive order.

For example, your :file:`REMOTE/master` has the list of commits from **a0** to **a4**;
Then, your changes **c0...c7** are on top of **a4**; thus:

.. code-block:: bash

   $ git log --oneline REMOTE/master..master

   a0
   a1
   a2
   a3
   a4
   c0
   c1
   ...
   c7

If :file:`REMOTE/master` receives commits **a5**,** a6** and **a7**. Pull with a rebase as follows:

.. code-block:: bash

   $ git pull --rebase REMOTE master

This pulls **a5-a7** and re-apply **c0-c7** on top of them:


.. code-block:: bash

   $ git log --oneline REMOTE/master..master
   a0
   ...
   a7
   c0
   c1
   ...
   c7

Getting better Logs from Git
****************************

Use these commands to change the configuration of Git in order to produce better logs:

.. code-block:: bash

   $ git config log.abbrevCommit true

The command above sets the log to abbreviate the commits' hash.

.. code-block:: bash

   $ git config log.abbrev 5

The command above sets the abbreviation length to the last 5 characters of the hash.

.. code-block:: bash

   $ git config format.pretty oneline

The command above avoids the insertion of an unnecessary line before the Author line.

To make these configuration changes specifically for the current Git user,
you must add the path option :option:`–-global` to :command:`config` as follows:

.. code-block:: bash

   $ git config –-global log.abbrevCommit true
   $ git config –-global log.abbrev 5
   $ git config –-global format.pretty oneline
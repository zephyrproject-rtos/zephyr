# Contributing to Chef Managed Community Cookbooks

We're glad you want to contribute to Chef managed community cookbooks! The first step is the desire to improve the project.

## Getting in Contact with Us

If you're interested in contributing to community cookbooks or just have a question about one of our cookbooks we'd love to chat. You can find us in #cookbook-engineering on the [Chef Community Slack](https://community-slack.chef.io/).

## Submitting Issues

Not every contribution comes in the form of code. Submitting, confirming, and triaging issues is an important task for any project. At Chef we use GitHub to track all project issues.

All of our cookbooks can be found in our [GitHub organization](https://github.com/chef-client/). All cookbooks include GitHub issue templates to help gather information needed for a thorough review.

We ask you not to submit security concerns via GitHub. For details on submitting potential security issues please see <https://www.chef.io/security/>

## Contribution Process

We have a 3 step process for contributions:

1. Commit changes to a git branch, making sure to sign-off those changes for the [Developer Certificate of Origin](#developer-certification-of-origin-dco).
2. Create a GitHub Pull Request for your change, following the instructions in the pull request template.
3. Perform a [Code Review](#code-review-process) with the cookbook maintainers on the pull request.

### Pull Request Requirements

Chef cookbooks are built to last. We strive to ensure high quality throughout the experience. In order to ensure this, all pull requests must meet these specifications:

1. **Tests:** To ensure high quality code and protect against future regressions, we require all cookbook code to be tested in some way. This can be either unit testing with ChefSpec or integration testing with Test Kitchen / InSpec. See the TESTING.md file for additional information on testing in Chef cookbooks and feel free to ask if you need help with the testing process.
2. **Green CI Tests:** We use [Travis CI](https://travis-ci.org/) and/or [AppVeyor](https://www.appveyor.com/) CI systems to test all pull requests. We require these test runs to succeed on every pull request before being merged.

### Code Review Process

Code review takes place in GitHub pull requests. See [this article](https://help.github.com/articles/about-pull-requests/) if you're not familiar with GitHub Pull Requests.

Once you open a pull request, cookbook maintainers will review your code using the built-in code review process in Github PRs. The process at this point is as follows:

1. A cookbook maintainer will review your code and merge it if no changes are necessary. Your change will be merged into the cookbooks's `master` branch and will be noted in the cookbook's `CHANGELOG.md` at the time of release.
2. If a maintainer has feedback or questions on your changes they they will set `request changes` in the review and provide an explanation.

### Developer Certification of Origin (DCO)

Licensing is very important to open source projects. It helps ensure the software continues to be available under the terms that the author desired.

Chef uses [the Apache 2.0 license](https://github.com/chef/chef/blob/master/LICENSE) to strike a balance between open contribution and allowing you to use the software however you would like to.

The license tells you what rights you have that are provided by the copyright holder. It is important that the contributor fully understands what rights they are licensing and agrees to them. Sometimes the copyright holder isn't the contributor, such as when the contributor is doing work on behalf of a company.

To make a good faith effort to ensure these criteria are met, Chef requires the Developer Certificate of Origin (DCO) process to be followed.

The DCO is an attestation attached to every contribution made by every developer. In the commit message of the contribution, the developer simply adds a Signed-off-by statement and thereby agrees to the DCO, which you can find below or at <http://developercertificate.org/>.

```
Developer's Certificate of Origin 1.1

By making a contribution to this project, I certify that:

(a) The contribution was created in whole or in part by me and I
    have the right to submit it under the open source license
    indicated in the file; or

(b) The contribution is based upon previous work that, to the
    best of my knowledge, is covered under an appropriate open
    source license and I have the right under that license to   
    submit that work with modifications, whether created in whole
    or in part by me, under the same open source license (unless
    I am permitted to submit under a different license), as
    Indicated in the file; or

(c) The contribution was provided directly to me by some other
    person who certified (a), (b) or (c) and I have not modified
    it.

(d) I understand and agree that this project and the contribution
    are public and that a record of the contribution (including
    all personal information I submit with it, including my
    sign-off) is maintained indefinitely and may be redistributed
    consistent with this project or the open source license(s)
    involved.
```

For more information on the change see the Chef Blog post [Introducing Developer Certificate of Origin](https://blog.chef.io/2016/09/19/introducing-developer-certificate-of-origin/)

#### DCO Sign-Off Methods

The DCO requires a sign-off message in the following format appear on each commit in the pull request:

```
Signed-off-by: Julia Child <juliachild@chef.io>
```

The DCO text can either be manually added to your commit body, or you can add either **-s** or **--signoff** to your usual git commit commands. If you forget to add the sign-off you can also amend a previous commit with the sign-off by running **git commit --amend -s**. If you've pushed your changes to GitHub already you'll need to force push your branch after this with **git push -f**.


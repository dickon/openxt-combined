This fork of OpenXT is an investigation in to building OpenXT with all the github repos combined
into a single repository. 

# Status

Dickon was thinking about how to do automated builds and verison control in a way that works for
multiple organsiations and figured to set this up as a strawman.

# Background

Historically XT was built from 50+ repositories. All developers had push access and
developers would push on master or a small number of topic and release branches. 
Post-receive hooks in the central repositories triggered automated buildbot builds.
Early in the automated build a tag in each centralwas created on the commit in each repository
that was being used for that build, which allowed a record of the code that went into a build.

That approach has some assumptions:

1. That centralised builds had write access to that branch. 
2. That there was a single set of repositories that contained all the branches that can be
   built automatically.

Assumption 2 is likely to be problematic in a scaled up operation.

Multi repository development have some disadvantages:

1. Coordinating pull requests and reviews over multiple repositories is problematic.
2. We need to construct machinery to tag across multiple repositories and to checkout
   appropriately.

# Approach

Since OpenXT has no history the repos are currently quite small (80MB total) and there is a good
opportunity to move to a single repo format. 

Concerns:

1. Getting OE to pick up source code already checked out.
2. Name clashes in openxt with a simple mapping to top level directories in this repo.
3. Integration into buildbot.
4. Reusable modules may be better done as independent repos.
5. The repositoriy may be slow to work with.

(4) and (5) can be address by moving some pieces back out as standalone git repos,
using the facilities in OpenEmbedded for referencing a particular version. Moving them out
means they no longer get covered by the same pull request.














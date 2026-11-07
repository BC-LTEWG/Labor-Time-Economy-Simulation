> **Note — delete every Note before the push.**
> This is the wiki Git workflow (Rev. 2026-10-15) unchanged, plus our table G1–G6. Only G5 has two versions:
> **a** = ours, **b** = Google's [CL descriptions](https://google.github.io/eng-practices/review/developer/cl-descriptions.html).
> Delete the one you do not want. Everything else already matches Google's
> [eng-practices](https://google.github.io/eng-practices/) (imperative commit titles, small changes, author merges after approval).

# Development
(Rev. 2026-10-15)
1. Create an issue.  (It may be appropriate to create a _discussion_ first.)
  - The name of the issue should be a short imperative statement --- e.g., "Allow standard work week to float."
2. Assuming that you will develop the code for this issue ---
  - Assign it to yourself.
  - Click on the link or button in the issue to create a new (feature) branch. GitHub will create a new tracking branch whose name begins with the issue number and continues with a copy of the issue's title. If possible and reasonable, keep that name.
  - Mark your issue as "in progress."
3. On your machine (locally), switch to the new branch.  E.g., if the branch is called **123-allow-standard-work-week-to-float**, in your terminal, in the top-level repo directory, enter `git fetch` and then `git switch 123-allow-standard-work-week-to-float`. (Tab completion should work.)
4. Make your changes in your local branch.
  - Write brief commit messages that are _imperative statements_ capturing the most important essential change in _conceptual_ terms --- e.g., _Compute workers' busyness on every time step_.
  - Before you push changes, make sure that you can build and run the executable successfully. Also run the unit tests and make sure that they pass.
5. When your code is ready for review, create a pull request (PR) from the issue (feature) branch on GitHub.
  - In the PR description, include the line, `Resolves #XXX`, where `XXX` is the number of the original issue (123 in the above example).
  - Also in the description, explain your changes at a high level.
  - Choose a reviewer from whom to get a review.
7. When the reviewer submits comments, please respond to all comments. Do not mark anything as "resolved."
  - The reviewer should then read the responses and mark the conversations as "resolved" when they are indeed resolved.
8. When the reviewer has approved the PR, merge it into `main` on GitHub.  (The author of the code is the one to merge it into `main`, not the reviewer.)
9. When the code has been merged into `main`, delete the feature branch on GitHub and the local copy on your machine.

# Code Review

When reviewing, please bear in mind the following:
- The logic must be clear.  Ask questions if it is not.
- Wherever possible, code should _not_ rely on comments for clarity.  Code should be self-explanatory.  We should _avoid comments_ as much as possible. (It is OK to use a judiciously-placed comment to refer to documentation about an otherwise complex algorithm, or to warn future developers, e.g., about dependencies in a sequence of events.) The idea is to put the burden of clarity on naming and logic.
- The logic must be sound.
- We must avoid code duplication.
- Functions should be short. A function should do one thing (though it may have some limited side effects).
- Code should be reasonably efficient.
- Code should follow our style guidelines — capitalization, spacing, etc.
- In a class definition, put the public API functions first, starting with the constructors. In the `private` section, put all functions above the data members.
- In implementation files, function implementations should follow the same order as their corresponding declarations in class (`class` or `struct`) definitions.
- The structure of the code base should maintain good _separation of concerns_.

> **Note:** the comments bullet above follows C2a in `cpp-style-guide.md`. If you choose C2b, it changes too.

# Commits and Pull Requests
| Id | Rule |
|---|---|
| G1 | One concern per PR. A title that needs "and" is two PRs. |
| G2 | Every commit builds and passes the tests on its own. |
| G3 | Renames are separate commits from behavior changes (N132). |
| G4 | PRs that build on each other are stacked: each one targets the branch of the one before it and is reviewed in that order. |
| G5a | A PR description says what changes in this PR, nothing else: what moved where, with file and function references. |
| G5b | A PR description says what changes and **why**: the problem, why this approach, and its shortcomings. Background (issue, benchmark, design doc) is summarized, not only linked. |
| G6 | A proposal shows **today** and **proposed** side by side (W3), with file and function for today. |

> **Note G1–G3:** Google says the same ("small CLs", "don't break the build", "separate out refactorings").
>
> **Note G5:** **a** leaves out the "why"; the wiki (step 5) only asks to "explain your changes at a high level".
> **b** is Google's rule.

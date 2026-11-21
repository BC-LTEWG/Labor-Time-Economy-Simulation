@AGENTS.md
@docs/cpp-style-guide.md
@docs/git-workflow.md

# Working with few tokens

| Do | Instead of |
|---|---|
| Check **before each commit**: build and run the tests. Every commit passes on its own. | Checking the whole branch at the end, then replaying commits to fix them |
| Reference code by file and function (`Person::shop`). Line numbers only inside a long function. | `file:line` that moves with every edit and reformat |
| Print only pass/fail and findings (`grep`, `tail`); hide CRLF warnings | Full logs and git warnings in the conversation |
| Edit files with the editor tool | Inline Python or `sed` with backslashes (the shell eats them) |
| One step per session; the plan lives in a file | One long session for a whole task |
| Trust a check that passed; read a file once | Re-running passed checks, re-reading files already seen |

# Governance

This document describes how decisions get made in `intp`. It is short
on purpose — small projects need clear rules more than they need
elaborate ones.

---

## Phases

`intp` will pass through three governance phases. We are currently in
**Phase 0**.

### Phase 0 — Dissertation phase (now → ~v1.0)

The project is the working artefact of a Master's dissertation
(PPGCC/PUCRS, defense expected March 2027). During this phase:

- The **lead maintainer** (see [MAINTAINERS](MAINTAINERS)) has final say
  on architecture and scope. This is a *benevolent dictator* model,
  appropriate while the codebase is small and the design is still
  converging on the dissertation's evaluated configuration.
- Contributions are welcome and reviewed seriously, but disagreements
  about scope or direction default to "no" until v1.0.
- The plugin ABI in `include/intp/plugin.h` is *unstable* during this
  phase. Out-of-tree plugins are encouraged, but we make no compatibility
  promise across pre-1.0 releases.

### Phase 1 — Maintainers council (v1.0 → v2.0)

Once `intp 1.0` ships and the plugin ABI freezes, governance shifts to
a small council of maintainers (target size: 3–5).

- **Decisions** are made by lazy consensus on the public PR / issue
  tracker. If a decision is contested, any maintainer can call a vote;
  votes pass on a simple majority of the active council, ties broken
  by the lead maintainer.
- **New maintainers** are nominated by an existing maintainer and
  accepted on lazy consensus (no objection within seven days). The
  bar is sustained, high-quality contributions over several months,
  including reviews.
- **Stepping down** is normal and expected. A maintainer who has been
  inactive for six months is moved to "emeritus" status; they keep the
  title and the credit, lose the merge bit, and can rejoin without
  re-nomination if they come back.
- The **lead maintainer** is one of the council, with a tie-breaker
  vote and the keys to the release infrastructure. The role rotates
  by council vote at most once per year.

### Phase 2 — Hand-off (post-defense, indefinite)

If the project outlives the dissertation by enough that distro
packagers, downstream consumers, and the maintainer council are all
load-bearing, governance can be handed off to a neutral foundation
(Linux Foundation, OpenSSF, etc.) by council vote. This is a *should
be possible*, not a *must happen* — it depends on adoption.

---

## Decision records

Architectural decisions are recorded as ADRs (Architecture Decision
Records) in [`docs/design/`](docs/design/), numbered sequentially.
Anything that changes the public C API, the plugin ABI, the output
schema, or the build-dependency surface needs an ADR. Bug fixes and
internal refactors do not.

ADRs are short (one page is fine) and follow the "context →
decision → consequences" format. A rejected proposal still gets an
ADR with status `Rejected` so future contributors can see we
considered it.

---

## Scope discipline

The shipped `intp` binary collects the **seven core OS-level metrics**
from the IntP paper and nothing else. New metrics, new data sources,
new application-level signals — all of those go in plugins or
companion packages. This is not a question of where things technically
*could* live; it is a deliberate choice to keep the distro-packaged
core small enough that distro packagers will adopt it.

If your proposal grows the base package's `Build-Depends` or moves the
minimum kernel requirement, expect pushback. There is almost always a
plugin-shaped version of the same idea.

---

## Conflict resolution

Disagreements between maintainers, between maintainers and contributors,
or between users and the project default to public, written discussion
in the GitHub issue or PR. If that doesn't resolve it:

1. The lead maintainer makes a call and explains the reasoning in
   writing.
2. If the disagreement is about Code of Conduct ([code_of_conduct.txt](code_of_conduct.txt)),
   it is handled by the maintainers privately, per the enforcement
   contact in [SECURITY.md](SECURITY.md).
3. Persistent technical disagreement that cannot be resolved by the
   lead maintainer goes to a council vote (Phase 1+) or stays at
   "no" (Phase 0).

---

## Communication

- Public technical discussion: GitHub Issues / Discussions / PRs.
- Private maintainer matters (security, CoC, personnel): the
  enforcement contact in [SECURITY.md](SECURITY.md).
- Release announcements: GitHub Releases page + the `intp-announce`
  channel referenced in the README once it exists.

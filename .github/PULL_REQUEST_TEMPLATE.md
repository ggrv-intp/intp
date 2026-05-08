<!--
Thanks for contributing! Please fill in the sections below. Anything
marked optional can be deleted if it does not apply.

Before opening: please skim CONTRIBUTING.md if you have not already.
-->

## Summary

<!-- One or two sentences: what does this change do, and why? -->

## Area

<!-- Tick what applies. Maintainers will adjust labels. -->

- [ ] `area/core`
- [ ] `area/backend` (procfs / perf_event / resctrl / netlink / ebpf)
- [ ] `area/plugin-abi`  &nbsp;⚠️ **ABI-touching change — flag for `abidiff` review**
- [ ] `area/plugin` (bpftrace / gpu-* / db-*)
- [ ] `area/output`
- [ ] `area/ipc`
- [ ] `area/integrations` (prometheus / otel / k8s / slurm / iada / grafana)
- [ ] `area/bindings`
- [ ] `area/packaging` (debian / rpm / arch / alpine)
- [ ] `area/docs`
- [ ] `area/ci`
- [ ] `area/bench`

## Type

- [ ] Bug fix (`fix:` …)
- [ ] Feature / new capability (`feat:` …)
- [ ] Refactor (no behaviour change)
- [ ] Documentation
- [ ] Build / packaging / CI
- [ ] **Breaking change** &nbsp;⚠️ — describe migration in the section below

## What changed

<!-- Concrete description. Bullet list is fine. -->

-

## Why

<!-- Reviewers care most about this section. What problem does the
change solve? If you considered alternatives, what made you pick this
one? -->

## How was this tested

<!-- Be specific. -->

- [ ] `meson test -C build` passes locally.
- [ ] `clang-format --dry-run -Werror` clean on touched C files.
- [ ] `shellcheck` clean on touched shell files.
- [ ] For metric/backend changes: ran `bench/orchestrate.sh smoke`
  on kernel ___ / CPU ___; output diff vs. previous snapshot below.
- [ ] For plugin-ABI changes: `tests/plugin-abi/` pass and `abidiff`
  reports no breakage (or breakage is intentional and explained).
- [ ] For packaging changes: `debuild -us -uc` clean; `lintian` no errors.

<details>
<summary>Test output / fidelity diff (optional)</summary>

```
<!-- paste here if relevant -->
```

</details>

## Breaking changes / migration notes

<!-- Delete this section if not applicable.

If you ARE breaking something:
- What breaks?
- Who is affected (libintp consumers / out-of-tree plugins / output-format consumers / packagers)?
- What does the migration look like?
- Is there a deprecation path or a hard cut?
-->

## Checklist

- [ ] PR title follows [Conventional Commits](https://www.conventionalcommits.org/).
- [ ] Each commit is signed off (`git commit -s`) per DCO.
- [ ] Linked issue (`Fixes #`, `Closes #`, `Refs #`) when applicable.
- [ ] Updated `CHANGELOG.md` under `[Unreleased]` (skip for pure refactors / internal docs).
- [ ] Added or updated tests covering the change.
- [ ] Updated docs under `docs/` if user-visible behaviour changed.
- [ ] For new external dependencies: ADR added under `docs/design/`.

<!-- Reviewers: please leave at least one comment that is either an
approval, a request for change, or an explicit "not blocking, but…"
note so the author knows where they stand. -->

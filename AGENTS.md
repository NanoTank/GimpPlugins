# Codex project instructions — NanoTank/GimpPlugins

These rules apply to the entire repository. Read the task-specific specification
and `docs/DEVELOPMENT.md` before changing a filter. Keep this file small and
stable; detailed implementation notes belong in `docs/`.

## Scope and goals

- Work **only** in repositories owned by the personal GitHub account `NanoTank`.
  Do not inspect, search, or modify repositories of organizations that the user
  belongs to, even when technically accessible.
- This is a public repository: never commit credentials, personal files,
  screenshots with private data, caches, binaries, or local paths containing secrets.
- Target: **GIMP 3.2.6 Flatpak on Kubuntu**, with the matching GEGL runtime
  (GEGL 0.4.72 at the time of foundation acceptance).
- Every end-user image filter belongs under **Filter → NanoTank**.
  Use distinct names and operation IDs, prefixed `nanotank-`/`nanotank:`.
- **GEGL-first:** implement pixel/material effects as native GEGL operations
  in C, with properties exposed as normal GIMP filter controls. Use Python or
  Script-Fu only as a *thin helper* when GEGL alone cannot handle required GIMP
  procedures (e.g. creating layers or preparing inputs); explain the need.

## Sources of truth

1. The current user task and its acceptance criteria.
2. The specific filter brief, for example `docs/GLASS-BRIEF.md`.
3. Shared conventions in `docs/DEVELOPMENT.md`, plus `README.md`
   and `docs/GEGL-DEVELOPMENT.md`.
4. For a new filter, copy and complete `docs/FILTER-SPEC-TEMPLATE.md`
   rather than assuming requirements.

When requirements conflict, do not quietly invent behavior. Document the
trade-off or ask for the missing product decision where essential.

## Design and quality

- Favor convincing output and correct behavior over premature optimization.
- Respect arbitrary selection shapes (including feathered edges), alpha,
  layer boundaries, image precision and transparency.
- Provide understandable **German** parameter labels, sensible defaults,
  useful ranges, presets where justified, and a usable live preview.
- Prefer non-destructive GEGL layer effects. Test Undo/Redo, reopening the
  effect, and saving/reloading XCF before calling a filter production-ready.
- A GEGL operation usually receives its own input, **not automatically the
  composite of all visible GIMP layers**. Verify background-input needs,
  especially for transparency, refraction, blur, and magnification.
- Do not equate translucency with opacity of a solid substrate: glass drawn
  on top of an opaque button must not make that button transparent.
- Do not promise exact 3D physics from a single 2D image. Describe simulated
  effects and unavoidable limits precisely.

## Implementation and local validation

- Keep each filter in its own feature branch and source directory (e.g.
  `gegl/nanotank-glass/`). Do not change unrelated filters or their output.
- Reference native test: `gegl/nanotank-gegl-test/`.
  `bash scripts/build-gegl.sh` currently builds **only** that GEGL test;
  do not assume it compiles every new filter. Add a targeted build path when
  implementing a new filter.
- Native modules are built against the **GIMP Flatpak SDK**, not accidentally
  linked to Kubuntu host GEGL. Check `pkg-config`, `gegl-op.h`, and the
  resulting module's runtime compatibility.
- Build products go into `output/` and remain untracked. The Python plug-in
  development path and native GEGL deployment are different; follow the
  documented workflow.
- **Close GIMP before replacing/removing loaded GEGL modules.** Never
  overwrite unrelated installed plug-ins or reconfigure all of GIMP.
- Run focused local checks appropriate to the change, including actual visual
  checks for visual effects. Do not rerun extensive unrelated regression suites
  for every small edit. Clearly separate *tested locally* from *still awaiting
  user acceptance*.

## GitHub and delivery discipline

- GitHub is primarily an exchange/versioning repository for the user, Codex
  and ChatGPT — **not a build farm**.
- Do not create GitHub Actions workflows, push-triggered CI, automated builds,
  releases, artifact uploads, or scheduled jobs without explicit user approval.
  Perform builds and tests locally.
- Do not merge into `main`, rewrite shared history, delete branches, publish
  releases, or change repository settings unless the user explicitly requests it.
- Keep commits small and relevant. A feature-branch push/PR is acceptable when
  needed to share the requested work; never mistake a push for user acceptance.
- State in the handoff: branch/commit, what changed, checks actually run,
  known limitations, and the exact minimal steps for the user to test.

## Acceptance and safety

- The Python **Systemtest** and native **GEGL-Test** were run successfully
  by the user in GIMP 3.2.6 Flatpak; use them as foundation examples.
- This does **not** mean every new filter has passed alpha, selection,
  Undo/Redo, XCF, precision, or performance testing. Verify these per filter.
- Preserve the current GIMP installation, other plug-ins, project files,
  and separate feature branches. No hidden mass cleanup or destructive steps.

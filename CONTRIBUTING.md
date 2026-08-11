# Contributing

Contributions should preserve the narrow, verified scope of the project.

## Build and test

```powershell
$env:VCPKG_ROOT = "C:\path\to\vcpkg"
cmake --preset msvc-release
cmake --build --preset msvc-release
ctest --preset msvc-release
```

New behavior requires a positive real-path test and an adversarial case that would fail under an obvious implementation defect. Tests should assert observable values, ordering, identity, boundaries, or typed failures rather than existence alone.

## Changes

- Keep one semantic concern per commit with a descriptive subject.
- Do not manufacture commit dates or development activity.
- Avoid new dependencies unless a current capability requires them.
- Do not add unsupported backends, modes, or extension points as placeholders.
- Keep machine-specific paths, generated artifacts, protected samples, credentials, and private tool configuration out of the repository.
- Prefer expressive names and structure over explanatory code comments.

## AI-assisted contributions

AI tools are welcome. Pull requests must disclose material AI assistance, name the affected areas, and state how the result was verified. The contributor remains responsible for the submitted code and its provenance.

# Lifter

Lifter is an experimental Windows x86-64 PE lifting pipeline. It maps a PE image, recovers a function-level control-flow graph with Zydis, lowers it to LLVM IR, optimizes the IR, and can emit a callable DLL through LLVM and LLD.

## Status

Lifter is pre-1.0 research software. The supported path is intentionally narrow:

- Windows x86-64 PE input.
- A single target function selected by RVA and output symbol name.
- A target ABI of four 64-bit integer or pointer slots returning one 64-bit value.
- PE mapping with LIEF.
- x86-64 decoding and control-flow recovery with Zydis.
- Structural LLVM IR lifting.
- LLVM optimization and COFF DLL emission through LLD.
- CLI and TOML configuration front ends over the same pipeline.

Not currently supported:

- Complete multi-path symbolic execution.
- General VM devirtualization.
- ELF, Mach-O, x86-32, ARM, or non-Windows output.
- Whole-program lifting, import reconstruction, or binary reinjection.
- Other calling conventions, floating-point/vector arguments, stack arguments, and non-64-bit return values.
- Stack access outside the guarded 1,024-byte synthetic stack, derived stack addresses in general registers, and
  instruction-pointer-relative memory.
- Conditions that consume flags produced in another basic block or unsupported carry/overflow semantics.

The boundaries and data flow are documented in [Architecture](docs/Architecture.md). Research directions are recorded in [Roadmap](docs/Roadmap.md).

## Build

Requirements:

- Windows x64.
- Visual Studio 2022 with the Desktop development with C++ workload.
- CMake 3.24 or newer.
- vcpkg with `VCPKG_ROOT` set to its installation directory.

The manifest pins its vcpkg baseline. The reference preset enables strict warnings, builds the generated fixture from source, and runs the full test suite.

```powershell
$env:VCPKG_ROOT = "C:\path\to\vcpkg"
cmake --preset msvc-release
cmake --build --preset msvc-release
ctest --preset msvc-release
```

## Usage

```text
lifter <binary> --name <symbol> [--function <rva>] [-o output.ll]
lifter <binary> --name <symbol> --compile [--function <rva>] [-o output.dll]
lifter --config <file.toml>
lifter --help
lifter --version
```

`--function` is an RVA relative to the image base. When omitted, the PE entry point is used. `--name` defines the emitted function symbol and is required for processing.

Example TOML:

```toml
entry = "0x1000"
name = "TransformCore"

[[input.source]]
kind = "file"
path = "target.exe"

[output]
mode = "compile"
path = "recovered.dll"
```

## Verification

The integration suite builds `samples/Transform/Transform.c` into a fresh DLL. Tests discover the exported function RVA at runtime, lift and recompile that function, and compare the generated function against the loaded fixture over fixed boundary cases and 200,000 deterministic randomized inputs.

No protected commercial samples, PDB files, reverse-engineering databases, or machine-specific tool configuration are part of the public tree.

## Repository layout

```text
samples/Transform       reproducible integration fixture
src/Lifter              production source by pipeline responsibility
tests/Support           owned test infrastructure
tests/Unit              invariant and end-to-end tests
docs                    architecture, AI-assistance policy, and roadmap
```

## Responsible use

Use Lifter only on software you own or are authorized to analyze. The project is intended for interoperability, research, testing, and defensive analysis. See [Security](SECURITY.md) for reporting and operational guidance.

## License

Licensed under the [Apache License 2.0](LICENSE). Dependency licensing and binary-distribution obligations are summarized in [Third-party software](THIRD_PARTY.md).

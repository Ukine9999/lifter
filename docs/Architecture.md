# Architecture

Lifter source is organized by pipeline responsibility. Process construction is confined to the composition root.

## Runtime flow

1. `Cli` parses arguments or loads TOML through `Config`.
2. `Pipeline::BinaryProcessor` maps the requested image through `AddressSpace::IImageProvider`.
3. `Disasm::IDisassembler` recovers the selected function's control-flow graph.
4. `Lift::ILifter` lowers the graph to `Ir::IIrModule`.
5. `Backend::IOptimizer` optimizes the module.
6. The pipeline prints IR or calls `Backend::IRecompileBackend` to emit a DLL.

The CLI and TOML front ends converge on `Pipeline::ProcessRequest`; they do not implement separate lifting semantics.

## Ownership

| Area | Responsibility | Direct collaborators |
|---|---|---|
| `AddressSpace` | Load PE files and expose mapped virtual regions | LIEF |
| `Disasm` | Decode x86-64 instructions and recover function CFGs | Zydis, `AddressSpace` |
| `Lift` | Lower decoded semantics into LLVM IR | LLVM, `Disasm`, `Ir` |
| `Ir` | Own LLVM context/module lifetime behind the project interface | LLVM |
| `Backend` | Optimize IR and emit COFF DLLs | LLVM, LLD, `Ir` |
| `Pipeline` | Orchestrate one processing request | inward interfaces from the phases above |
| `Config` | Parse the supported TOML schema | toml++ |
| `Cli` | Validate delivery input and present results | `Config`, `Pipeline` APIs |
| `Composition` | Construct the executable object graph and own lifecycle wiring | all concrete runtime components |

## Internal seams

The current test and composition seams are source-level C++ interfaces, not a supported external API or stable binary ABI:

- `IImageProvider` for image acquisition.
- `IDisassembler` for instruction and CFG recovery.
- `ILifter` for lowering policy.
- `IOptimizer` for IR optimization policy.
- `IRecompileBackend` for output generation.

Concrete implementations are created only in `Composition/CompositionRoot.cpp`. Runtime code receives dependencies through constructors and does not perform service lookup.

## Build boundary

The current build intentionally compiles production code into one internal static target and links it into the CLI executable. Source folders express ownership and review boundaries, but CMake does not claim separate binary-module enforcement. No library target is installed or exported as a public SDK.

## Invariants

- Image addresses remain virtual addresses until a component explicitly asks `IAddressSpace` for bytes.
- A processing request names exactly one file source and one emitted function symbol.
- Both configuration front ends create the same `ProcessRequest` shape.
- LLVM context lifetime dominates the module that references it.
- Recompilation receives an optimized module and returns an explicit output path or a failure.
- Integration fixtures are built from owned source during the build; repository state does not supply opaque oracle binaries.
- Direct RSP/RBP synthetic-stack accesses carry runtime bounds guards. Deriving stack addresses into general registers, instruction-pointer-relative memory, and cross-block flag semantics fail during lifting instead of producing speculative IR.

## Platform boundary

The current backend is Windows-specific. It emits an x86-64 Windows target triple, links COFF through LLD, and tests generated DLLs with the Windows loader. Supporting another platform requires a separate current backend and test oracle rather than conditional behavior scattered through the existing path.

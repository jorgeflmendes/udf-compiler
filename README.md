# UDF Compiler

> A C++20 compiler pipeline covering scanning, parsing, semantic analysis, type checking, and code generation.

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue)](https://isocpp.org/)
[![Build](https://img.shields.io/badge/Build-Makefile-informational)](Makefile)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

This repository implements a compiler for the UDF language on top of the CDK
infrastructure used by Instituto Superior Técnico. It maps source text through
Flex/Bison parsing into a typed abstract syntax tree, validates language
semantics, and emits XML or postfix-oriented target code.

## Overview

The compiler demonstrates the complete structure of a classical language
implementation: tokenization, grammar-driven AST construction, symbol-aware
semantic analysis, type inference, visitor-based traversal, and backend
lowering. UDF includes primitive, pointer, function, and tensor-oriented
constructs, making the semantic and code-generation phases more substantial
than a minimal expression language.

The official language definition is documented in the
[UDF Language Reference Manual](https://web.tecnico.ulisboa.pt/~david.matos/w/pt/index.php/Compiladores/Projecto_de_Compiladores/Projecto_2024-2025/Manual_de_Refer%C3%AAncia_da_Linguagem_UDF).

## Academic Context

This project was developed as part of the **Compilers** course unit at
**Instituto Superior Técnico, University of Lisbon**.

This project explores language processing, parsing, semantic analysis,
intermediate representation, and code generation.

## Key Features

- Flex scanner and Bison grammar
- Typed AST for declarations, expressions, functions, control flow, and tensors
- Symbol-table-driven semantic validation
- `auto` declaration and return-type inference
- Primitive, pointer, functional, and tensor types
- Visitor-based XML and postfix backends
- Runtime-backed tensor operations and structured diagnostics

## Architecture

```text
UDF source
   |
   v
Flex scanner -> Bison parser -> AST
                                |
                                v
                    semantic analysis + typing
                                |
                    +-----------+-----------+
                    v                       v
                XML writer             postfix writer
```

The scanner and parser construct UDF-specific nodes under `ast/`.
`targets/type_checker.cpp` annotates and validates the tree. Backend visitors
then consume the typed AST without coupling code generation to parsing.

Further detail is available in [docs/architecture.md](docs/architecture.md),
[docs/language.md](docs/language.md), and [docs/semantics.md](docs/semantics.md).

## Tech Stack

- C++20
- Flex and Bison
- IST CDK compiler infrastructure
- Make
- Visitor and factory patterns

## Repository Structure

```text
.
|-- .auto/                  # Generated visitor and node declarations
|-- ast/                    # UDF abstract syntax tree nodes
|-- docs/                   # Architecture, language, and development notes
|-- scripts/                # Local regression helpers
|-- targets/                # Semantic analysis and backend visitors
|-- factory.cpp             # CDK compiler registration
|-- udf_parser.y            # Grammar and AST construction
|-- udf_scanner.l           # Lexical specification
`-- Makefile
```

## Getting Started

The compiler depends on the CDK and RTS packages supplied for the course
environment. On the supported openSUSE setup:

```bash
zypper ar https://download.opensuse.org/repositories/home:/d4vid:/co25/openSUSE_Tumbleweed/ CO25
zypper refresh
zypper install libcdk20-devel librts6-devel
git clone https://github.com/jorgeflmendes/udf-compiler.git
cd udf-compiler
make
```

For a manually installed toolchain, configure the `ROOT` variable in the
[Makefile](Makefile), then run:

```bash
make clean
make rebuild
```

## Running Tests

The helper below compares compiler output against course fixtures:

```bash
./scripts/check-expected.sh
```

The repository snapshot does not include the course test inputs and expected
outputs, so the regression workflow requires a local authorized fixture set.
Automated GitHub CI is intentionally not included because the required CDK/RTS
toolchain and fixtures are tied to the course environment.

## Example Pipeline

Important implementation entrypoints:

| Phase | Source |
| --- | --- |
| scanning | [`udf_scanner.l`](udf_scanner.l) |
| parsing and AST construction | [`udf_parser.y`](udf_parser.y) |
| semantic analysis | [`targets/type_checker.cpp`](targets/type_checker.cpp) |
| XML output | [`targets/xml_writer.cpp`](targets/xml_writer.cpp) |
| postfix code generation | [`targets/postfix_writer.cpp`](targets/postfix_writer.cpp) |

## Limitations

- Building requires the course CDK/RTS toolchain.
- Authorized regression fixtures are not distributed in this repository.
- The compiler targets the UDF course language rather than a general-purpose ABI.
- Portability outside the supported Linux environment has not been validated.

## Roadmap

- Add self-contained public language fixtures
- Add focused semantic-diagnostic examples
- Document representative source, AST, and generated output side by side
- Introduce CI when the toolchain can be reproduced without restricted fixtures

## License

Licensed under the [MIT License](LICENSE). The external CDK/RTS toolchain and
course materials remain subject to their respective terms.

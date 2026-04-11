# UDF Compiler

[![Language](https://img.shields.io/badge/language-C%2B%2B20-blue.svg)](https://isocpp.org/)
[![Build](https://img.shields.io/badge/build-Makefile-1f6feb.svg)](Makefile)
[![Course](https://img.shields.io/badge/course-IST%20Compilers-black.svg)](#)

A compiler for **UDF**, developed as part of the **Compilers course at Instituto Superior Tecnico (IST)** on top of the CDK infrastructure used in the course environment.

The repository is documented against the official course language specification:
[UDF Language Reference Manual](https://web.tecnico.ulisboa.pt/~david.matos/w/pt/index.php/Compiladores/Projecto_de_Compiladores/Projecto_2024-2025/Manual_de_Refer%C3%AAncia_da_Linguagem_UDF).

## Topics

`compiler`, `compilers`, `lexer`, `parser`, `flex`, `bison`, `semantic-analysis`, `type-checking`, `code-generation`, `ast`, `ist`

## Overview

This repository implements the main stages of a classical compiler pipeline:

- lexical analysis with `flex`
- syntax analysis with `bison`
- AST construction for UDF-specific language constructs
- semantic analysis and type checking
- backend emission to XML and postfix-oriented target code

The project follows the visitor-based architecture encouraged by the IST CDK toolchain. Parsing constructs the AST, semantic analysis annotates it with types and validates language rules, and backend visitors consume the typed tree to emit target representations.

## Technical Scope

The current implementation covers the core compiler responsibilities expected in an academic compiler project:

- tokenization and grammar-driven parsing
- symbol-aware semantic validation
- type inference for `auto`-style declarations
- support for primitive, pointer, functional, and tensor-oriented constructs
- structured code generation for expressions, control flow, functions, and tensor runtime calls

Based on the official UDF manual and the implementation currently present in this repository, the language model includes:

- primitive types: `int`, `real`, `string`, `void`
- composite types: pointers and tensors
- declarations with `public`, `forward`, and `auto`
- global/private visibility rules with explicit module exports through `public`
- control flow with `if`, `elif`, `else`, `for`, `break`, `continue`, and `return`
- I/O and utility constructs such as `input`, `write`, `writeln`, `sizeof`, `nullptr`, and `objects`
- tensor-specific operations including literals, indexing, reshape, rank/dimension queries, capacity, contraction, and runtime-backed tensor arithmetic

The repository documentation distinguishes between:

- the **official language definition**, which comes from the course manual
- the **current compiler implementation**, which maps that specification onto AST nodes, semantic rules, and backend emission

Additional language notes are documented in [docs/language.md](docs/language.md).

## Compiler Pipeline

The implementation is organized as a sequence of well-defined phases:

1. **Scanning**  
   [udf_scanner.l](udf_scanner.l) defines the lexical structure of the language, including keywords, literals, identifiers, and operators.

2. **Parsing and AST Construction**  
   [udf_parser.y](udf_parser.y) defines the grammar and constructs typed AST nodes for declarations, expressions, statements, functions, and tensor constructs.

3. **Semantic Analysis**  
   [targets/type_checker.cpp](targets/type_checker.cpp) validates typing rules, enforces semantic constraints, resolves declarations through the symbol table, and propagates inferred types through the tree.

4. **Intermediate Tree Visualization**  
   [targets/xml_writer.cpp](targets/xml_writer.cpp) emits an XML representation of the AST, which is useful for debugging parser and semantic behavior.

5. **Backend Code Generation**  
   [targets/postfix_writer.cpp](targets/postfix_writer.cpp) lowers the typed AST into postfix-oriented output and runtime calls expected by the target environment.

More detail is available in [docs/architecture.md](docs/architecture.md).

## Language Semantics Snapshot

Some of the most important language semantics documented in the official manual and reflected in this compiler are:

- weak typing with selected implicit conversions
- a single global namespace, with local shadowing inside functions
- `public` and `forward` as global linkage/visibility qualifiers
- `auto` inference for declarations and function return types
- `udf` as the designated public program entry function
- tensors as runtime-managed first-class values rather than ordinary raw pointers

These points are especially relevant when reading the semantic analysis and backend code.

## Feature Matrix

| Language area | Main syntax entry point | Main semantic/backend entry points |
| --- | --- | --- |
| declarations and symbol visibility | [udf_parser.y](udf_parser.y) | [targets/type_checker.cpp](targets/type_checker.cpp), [targets/postfix_writer.cpp](targets/postfix_writer.cpp) |
| functions, calls, and returns | [udf_parser.y](udf_parser.y) | [targets/type_checker.cpp](targets/type_checker.cpp), [targets/postfix_writer.cpp](targets/postfix_writer.cpp) |
| control flow | [udf_parser.y](udf_parser.y) | [targets/type_checker.cpp](targets/type_checker.cpp), [targets/postfix_writer.cpp](targets/postfix_writer.cpp) |
| utility expressions such as `nullptr`, `sizeof`, and `objects` | [udf_parser.y](udf_parser.y) | [targets/type_checker.cpp](targets/type_checker.cpp), [targets/postfix_writer.cpp](targets/postfix_writer.cpp) |
| tensor language constructs | [udf_parser.y](udf_parser.y) | [targets/type_checker.cpp](targets/type_checker.cpp), [targets/postfix_writer.cpp](targets/postfix_writer.cpp) |

## Repository Structure

```text
.
+-- .auto/                  # Auto-generated visitor and node declarations
+-- ast/                    # UDF abstract syntax tree nodes
+-- docs/                   # Architecture, language, and development notes
+-- scripts/                # Helper scripts for local workflows
+-- targets/                # Semantic analysis and backend visitors
+-- factory.cpp             # Compiler factory registration
+-- factory.h
+-- Makefile                # Main build entrypoint
+-- udf_parser.y            # Grammar specification and AST construction
+-- udf_scanner.l           # Lexical specification
+-- udf_scanner.h
+-- CONTRIBUTING.md
+-- LICENSE
+-- README.md
```

## Main Components

- [udf_scanner.l](udf_scanner.l): scanner specification for tokenization
- [udf_parser.y](udf_parser.y): grammar, precedence rules, and AST construction
- [targets/type_checker.cpp](targets/type_checker.cpp): semantic checks, type propagation, and symbol-aware validation
- [targets/postfix_writer.cpp](targets/postfix_writer.cpp): target backend for postfix-oriented code emission
- [targets/xml_writer.cpp](targets/xml_writer.cpp): XML backend for structural inspection
- [factory.cpp](factory.cpp): compiler registration with the CDK factory infrastructure

## Build and Toolchain

The project depends on the CDK toolchain distributed in the IST course environment. The exact build path is controlled through the `ROOT` variable in the [Makefile](Makefile).

The current course support libraries are also distributed as prebuilt packages through the course openSUSE repository:

- repository: [home:d4vid:co25/openSUSE_Tumbleweed](https://download.opensuse.org/repositories/home:/d4vid:/co25/openSUSE_Tumbleweed/)
- development packages: `libcdk20-devel` and `librts6-devel`

On an openSUSE system, the repository can be added with:

```bash
zypper ar https://download.opensuse.org/repositories/home:/d4vid:/co25/openSUSE_Tumbleweed/ CO25
zypper refresh
```

Then install the required development packages with:

```bash
zypper install libcdk20-devel librts6-devel
```

If you are using the course virtual machine or a manually installed toolchain tree, the `ROOT` variable may still point to a local course root such as `${HOME}/compiladores/root`. Adjust it as needed for your environment.

Typical local build:

```bash
make
```

Clean and rebuild:

```bash
make clean
make rebuild
```

If your local CDK installation is not in the expected default location, update the `ROOT` variable before building.

## Regression Workflow

A helper script for expected-output checking is available at:

```bash
./scripts/check-expected.sh
```

This script is intended for local regression runs against course fixtures. The current repository snapshot does not include the `tests/` inputs and expected outputs required by that workflow, so successful execution depends on your local setup.

## Design Notes

- The project follows a visitor-oriented compiler architecture, which keeps AST definitions separate from semantic and backend concerns.
- Tensor constructs are treated as first-class language features and are supported both in semantic analysis and backend emission.
- Runtime-backed operations are clearly isolated in the postfix backend, which makes the lowering strategy easier to inspect and extend.

## Documentation

- [docs/architecture.md](docs/architecture.md): compiler structure and phase breakdown
- [docs/language.md](docs/language.md): technical summary of the official UDF manual mapped onto this compiler
- [docs/semantics.md](docs/semantics.md): semantic rules and feature-to-implementation mapping
- [docs/development.md](docs/development.md): local workflow, formatting, and repository conventions
- [CONTRIBUTING.md](CONTRIBUTING.md): contribution expectations and collaboration guidelines


## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.

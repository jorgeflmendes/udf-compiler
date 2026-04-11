# Architecture Overview

The compiler follows a conventional multi-stage pipeline built on top of the CDK infrastructure used in the IST Compilers course.

## Pipeline

1. The scanner in `udf_scanner.l` tokenizes the source program.
2. The parser in `udf_parser.y` builds the abstract syntax tree (AST).
3. The type checker in `targets/type_checker.cpp` validates semantic correctness.
4. The XML backend in `targets/xml_writer.cpp` emits an intermediate tree representation.
5. The postfix backend in `targets/postfix_writer.cpp` emits assembly-oriented output.



## Main Directories

- `ast/`: AST node definitions for the UDF language.
- `targets/`: semantic analysis and backend visitors.
- `.auto/`: generated declarations produced by the CDK tooling.

## Core Components

- `factory.{h,cpp}`: compiler factory registration.
- `udf_scanner.l`: lexical specification.
- `udf_parser.y`: grammar and AST construction.
- `targets/type_checker.*`: semantic validation and type inference.
- `targets/postfix_writer.*`: code generation backend.
- `targets/xml_writer.*`: XML visualization/debug backend.

## Language-to-Implementation View

The repository maps the official UDF language reference onto the compiler pipeline in a relatively clean way:

- lexical categories and reserved words are defined in `udf_scanner.l`
- grammatical structure and AST construction live in `udf_parser.y`
- semantic rules are centralized in `targets/type_checker.cpp`
- target lowering and runtime calls are centralized in `targets/postfix_writer.cpp`

This separation makes it easier to reason about whether a problem belongs to syntax, semantic analysis, or backend code generation.

## Notes

- The repository assumes a local CDK installation, configured through the `ROOT` variable in the `Makefile`.
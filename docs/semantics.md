# Semantic Notes

This document highlights the semantic rules that are most relevant when reading or extending the compiler. It is intended as a bridge between the official UDF language manual and the implementation found in this repository.

Primary specification:
[UDF Language Reference Manual](https://web.tecnico.ulisboa.pt/~david.matos/w/pt/index.php/Compiladores/Projecto_de_Compiladores/Projecto_2024-2025/Manual_de_Refer%C3%AAncia_da_Linguagem_UDF)

## Typing Model

UDF is specified as a weakly typed imperative language with selected implicit conversions. In practice, the implementation in this repository reflects that model through explicit semantic validation in [targets/type_checker.cpp](../targets/type_checker.cpp).

The most visible rules are:

- arithmetic expressions accept integer and real operands, with integer-to-real promotion when needed
- assignments must preserve semantic compatibility between the lvalue and rvalue
- `auto` declarations infer their type from the initializer
- function calls are checked against declared parameter and return types
- `nullptr` participates in pointer-oriented compatibility checks

## Name Resolution and Visibility

The official language definition uses a single global namespace for entities, with local declarations shadowing outer ones inside functions.

Repository mapping:

- parsing introduces declarations in [udf_parser.y](../udf_parser.y)
- semantic checks resolve and validate them in [targets/type_checker.cpp](../targets/type_checker.cpp)

Important consequences:

- `public` controls external visibility for module-level symbols
- `forward` introduces declarations intended to be resolved later
- function definitions are not valid inside blocks
- parameters are local to the active function scope

## Function Semantics

Functions are modeled as typed entities with an optional body. The semantic checker validates:

- declaration consistency
- call arity
- call argument compatibility
- return value compatibility with the declared function output

The language entry point is the public function `udf`. In the backend, [targets/postfix_writer.cpp](../targets/postfix_writer.cpp) emits a synthetic `_main` that initializes runtime support and then calls `udf`.

## Control Flow Semantics

The implementation includes semantic support for:

- conditional execution with `if`, `elif`, and `else`
- iteration with `for`
- early exit with `break`
- loop continuation with `continue`
- function termination with `return`

The type checker enforces that condition expressions are integer-typed, consistent with the rest of the language's boolean-as-integer model.

## Memory and Pointer Semantics

The language supports pointer types and explicit memory reservation through the `objects(...)` expression.

Implementation notes:

- `objects(...)` is parsed into a dedicated allocation node
- the semantic pass requires an integer allocation size
- allocation expressions default to a generic pointer-like result, later specialized through context such as assignment
- pointer arithmetic is handled separately from ordinary numeric arithmetic

## Tensor Semantics

Tensor support is the most language-specific part of this project. The current compiler includes dedicated parsing, semantic validation, and backend lowering for:

- tensor literals
- tensor indexing
- tensor reshaping
- rank and dimension inspection
- capacity queries
- contraction
- tensor/scalar and tensor/tensor arithmetic

The semantic checker validates:

- tensor literal contents
- tensor rank/index compatibility
- reshape dimension constraints
- contraction dimension compatibility
- tensor assignment compatibility

The backend delegates most tensor behavior to runtime helpers, which keeps the semantic phase focused on type and shape validation rather than low-level storage mechanics.

## Feature-to-Implementation Mapping

| Language feature | Parser / syntax | Semantic validation | Backend lowering |
| --- | --- | --- | --- |
| `public`, `forward`, `auto` declarations | [udf_parser.y](../udf_parser.y) | [targets/type_checker.cpp](../targets/type_checker.cpp) | [targets/postfix_writer.cpp](../targets/postfix_writer.cpp) |
| functions and calls | [udf_parser.y](../udf_parser.y) | [targets/type_checker.cpp](../targets/type_checker.cpp) | [targets/postfix_writer.cpp](../targets/postfix_writer.cpp) |
| `if`, `elif`, `else`, `for` | [udf_parser.y](../udf_parser.y) | [targets/type_checker.cpp](../targets/type_checker.cpp) | [targets/postfix_writer.cpp](../targets/postfix_writer.cpp) |
| `return`, `break`, `continue` | [udf_parser.y](../udf_parser.y) | [targets/type_checker.cpp](../targets/type_checker.cpp) | [targets/postfix_writer.cpp](../targets/postfix_writer.cpp) |
| `nullptr`, `sizeof`, `objects` | [udf_parser.y](../udf_parser.y) | [targets/type_checker.cpp](../targets/type_checker.cpp) | [targets/postfix_writer.cpp](../targets/postfix_writer.cpp) |
| tensor literals and indexing | [udf_parser.y](../udf_parser.y) | [targets/type_checker.cpp](../targets/type_checker.cpp) | [targets/postfix_writer.cpp](../targets/postfix_writer.cpp) |
| tensor reshape, rank, dims, capacity, contraction | [udf_parser.y](../udf_parser.y) | [targets/type_checker.cpp](../targets/type_checker.cpp) | [targets/postfix_writer.cpp](../targets/postfix_writer.cpp) |

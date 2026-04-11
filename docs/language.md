# UDF Language Notes

This document summarizes the **UDF language model** using the official IST reference manual as the primary source, while also relating that specification to the current compiler repository.

Primary reference:
[UDF Language Reference Manual](https://web.tecnico.ulisboa.pt/~david.matos/w/pt/index.php/Compiladores/Projecto_de_Compiladores/Projecto_2024-2025/Manual_de_Refer%C3%AAncia_da_Linguagem_UDF)



## Data Model

According to the official specification, UDF is an imperative language with weak typing and a limited set of implicit conversions.

### Primitive Types

- `int`: 32-bit signed integer
- `real`: 64-bit IEEE 754 floating-point value
- `string`: NUL-terminated character sequence
- `void`: absence of value

### Pointer Types

- `ptr<T>` denotes a pointer to `T`
- pointers occupy 4 bytes in the language model described by the manual
- pointer arithmetic is supported for ordinary pointers
- `ptr<auto>` behaves as a generic pointer type, similar in spirit to `void *`

### Tensor Types

- `tensor<d1, d2, ...>` denotes a tensor with explicitly declared dimensions
- tensors are first-class language entities
- semantically, tensor values behave as runtime-managed objects
- the manual distinguishes tensor handles from ordinary pointers, which is consistent with this repository's dedicated tensor AST, type checking, and backend lowering

## Naming, Scope, and Visibility

The manual defines a single global namespace for entities. Important consequences:

- global declarations are visible at file/module scope
- local declarations are visible until the end of the enclosing function
- local redeclarations may shadow outer declarations
- functions cannot be defined inside blocks
- `public` marks a symbol as externally visible
- `forward` introduces an external/forward declaration

By default, global symbols are private to the module unless explicitly declared `public`.

## Lexical Conventions

The language specification defines:

- whitespace as a separator
- line comments with `//`
- block comments with `/* ... */`
- nested block comments

Reserved keywords include:

- types: `int`, `real`, `ptr`, `string`, `tensor`, `void`
- declarations: `forward`, `public`, `auto`
- statements: `if`, `elif`, `else`, `for`, `break`, `continue`, `return`, `write`, `writeln`
- expressions and utilities: `input`, `nullptr`, `objects`, `sizeof`

The identifier `udf` is not globally reserved, but when used as a function name it designates the program entry point and must be declared `public`.

## Declarations and Initialization

The language supports:

- regular variable declarations
- `public` and `forward` global declarations
- `auto` declarations with inferred type
- explicit initialization at declaration sites

Important semantic notes from the manual:

- `auto` requires inference from an initializer or return expression
- function arguments cannot use `public` or `forward`
- default argument values are not part of the language
- `auto` is not used for parameter types except in the generic-pointer sense

## Functions

The specification models functions with:

- a return type
- an identifier
- an optional list of parameters
- either a body or a declaration-only form

Repository mapping:

- parser support is centered in [udf_parser.y](../udf_parser.y)
- semantic validation of calls and return types lives in [targets/type_checker.cpp](../targets/type_checker.cpp)
- lowering to target code is implemented in [targets/postfix_writer.cpp](../targets/postfix_writer.cpp)

### Entry Point

The official manual defines `udf` as the main entry function. In this repository, the postfix backend generates a synthetic `_main` entry point that initializes runtime services and then invokes `udf`.

## Statements

The language includes the following statement families:

- blocks
- conditional statements: `if`, `elif`, `else`
- iteration with `for`
- termination with `break`
- continuation with `continue`
- value-returning `return`
- expression statements
- output statements: `write`, `writeln`

These are reflected directly in the parser and lowered in the backend visitors.

## Expressions

The official reference covers both primitive and derived expressions.

### Core Expression Forms

- identifiers
- literals
- parenthesized expressions
- input expressions
- unary plus and minus
- logical negation
- arithmetic and comparison operators
- assignment
- pointer indexing
- memory reservation
- position/address expressions
- size queries via `sizeof`

### Special Constants and Utilities

- `nullptr`
- `objects`

## Tensor-Dependent Expressions

Tensor support is a central feature of the language manual and of this compiler implementation. The reference specifies:

- explicit tensor literals
- tensor capacity queries
- tensor rank queries
- dimension inspection through `dim` and `dims`
- tensor indexing
- reshaping
- tensor contraction

Repository mapping:

- grammar support exists in [udf_parser.y](../udf_parser.y)
- semantic checks are handled in [targets/type_checker.cpp](../targets/type_checker.cpp)
- runtime-backed lowering is implemented in [targets/postfix_writer.cpp](../targets/postfix_writer.cpp)

The backend shows that tensor operations are delegated to runtime helpers such as allocation, printing, indexing, reshape, equality, scalar arithmetic, and contraction support.

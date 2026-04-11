#ifndef __UDF_TARGET_FRAME_SIZE_CALCULATOR_H__
#define __UDF_TARGET_FRAME_SIZE_CALCULATOR_H__

#include "targets/basic_ast_visitor.h"
#include "targets/symbol.h"
#include <cdk/symbol_table.h>
#include <stack>

namespace udf {

/**
 * This visitor traverses the AST to calculate the total space
 * needed for a function's local variables.
 */
class frame_size_calculator : public basic_ast_visitor {
  cdk::symbol_table<udf::symbol> &_symtab; // symbol table for context

  std::stack<std::shared_ptr<udf::symbol>> _functions; // stack to keep track of the current function

  size_t _localsize; // accumulator for the size of local variables

public:
  /**
   * Initializes the visitor.
   */
  frame_size_calculator(
      std::shared_ptr<cdk::compiler> compiler,
      cdk::symbol_table<udf::symbol> &symtab,
      std::stack<std::shared_ptr<udf::symbol>> functions)
      : basic_ast_visitor(compiler), _symtab(symtab),
        _functions(functions), _localsize(0) {}

public:
  /**
   * The destructor.
   */
  ~frame_size_calculator();

public:
  /**
   * Returns the calculated size for local variables.
   */
  size_t localsize() const { return _localsize; }

public:
  // do not edit these lines
#define __IN_VISITOR_HEADER__
#include ".auto/visitor_decls.h" // automatically generated
#undef __IN_VISITOR_HEADER__
  // do not edit these lines: end
};

} // namespace udf

#endif
#pragma once

#include <cdk/ast/basic_node.h>

namespace udf {

/**
 * Class for describing program nodes.
 *
 * The constructor takes the following parameters:
 * - lineno: The line number where the program appears in the
 * source code.
 * - statements: A basic node representing the statements in the program.
 */
class program_node : public cdk::basic_node {
  cdk::basic_node *_statements;

public:
  program_node(int lineno, cdk::basic_node *statements)
      : cdk::basic_node(lineno), _statements(statements) {}

  cdk::basic_node *statements() { return _statements; }

  void accept(basic_ast_visitor *sp, int level) {
    sp->do_program_node(this, level);
  }
};

} // namespace udf

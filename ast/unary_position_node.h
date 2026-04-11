#pragma once

#include <cdk/ast/expression_node.h>
#include <cdk/ast/lvalue_node.h>
#include <cdk/ast/unary_operation_node.h>

namespace udf {

/**
 * Class for describing the unary position operator (?).
 *
 * The constructor takes the following parameters:
 * - lineno: The line number where the unary position expression appears in the
 * source code.
 * - argument: A left-value argument.
 */
class unary_position_node : public cdk::expression_node {
public:
  cdk::lvalue_node *_argument;

  unary_position_node(int lineno, cdk::lvalue_node *argument)
      : cdk::expression_node(lineno), _argument(argument) {}

  cdk::lvalue_node *argument() { return _argument; }

  void accept(basic_ast_visitor *sp, int level) override {
    sp->do_unary_position_node(this, level);
  }
};

} // namespace udf
#pragma once

#include <cdk/ast/expression_node.h>

namespace udf {

/**
 * Class for describing the size_of operator.
 *
 * The constructor takes the following parameters:
 * - lineno: The line number where the size_of expression appears in the
 * source code.
 * - argument: An expression node representing the argument to be measured.
 */
class size_of_node : public cdk::expression_node {
public:
  cdk::expression_node *_argument;

  size_of_node(int lineno, cdk::expression_node *argument)
      : cdk::expression_node(lineno), _argument(argument) {}

  cdk::expression_node *argument() { return _argument; }

  void accept(basic_ast_visitor *sp, int level) {
    sp->do_size_of_node(this, level);
  }
};

} // namespace udf
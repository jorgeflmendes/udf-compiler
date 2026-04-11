#pragma once

#include <cdk/ast/expression_node.h>

namespace udf {

/**
 * Class for describing evaluation nodes.
 *
 * The constructor takes the following parameters:
 * - lineno: The line number where the evaluation node appears in the source
 * code.
 * - argument: The expression node representing the argument to be evaluated.
 */
class evaluation_node : public cdk::basic_node {
  cdk::expression_node *_argument;

public:
  evaluation_node(int lineno, cdk::expression_node *argument)
      : cdk::basic_node(lineno), _argument(argument) {}

  cdk::expression_node *argument() { return _argument; }

  void accept(basic_ast_visitor *sp, int level) {
    sp->do_evaluation_node(this, level);
  }
};

} // namespace udf

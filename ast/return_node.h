#pragma once

#include <cdk/ast/expression_node.h>

namespace udf {

/**
 * Class for describing return nodes.
 *
 * The constructor takes the following parameters:
 * - lineno: The line number where the return statement appears in the
 * source code.
 * - return_value: An expression node representing the value to be returned.
 */
class return_node : public cdk::basic_node {
  cdk::expression_node *_return_value;

public:
  return_node(int lineno) : cdk::basic_node(lineno), _return_value(nullptr) {}

  return_node(int lineno, cdk::expression_node *return_value)
      : cdk::basic_node(lineno), _return_value(return_value) {}

  cdk::expression_node *return_value() { return _return_value; }

  void accept(basic_ast_visitor *sp, int level) {
    sp->do_return_node(this, level);
  }
};

} // namespace udf

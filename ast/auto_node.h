#pragma once

#include <cdk/ast/expression_node.h>

namespace udf {

/**
 * Class for describing 'auto' nodes.
 *
 * The constructor takes the following parameters:
 * - lineno: The line number where the 'auto' expression appears in the
 * source code.
 * - value: A pointer to the expression node that represents the value
 */
class auto_node : public cdk::expression_node {
  cdk::expression_node *_value;

public:
  auto_node(int lineno, cdk::expression_node *value) noexcept
      : cdk::expression_node(lineno), _value(value) {}

  cdk::expression_node *value() const { return _value; }

  void accept(basic_ast_visitor *sp, int level) override {
    sp->do_auto_node(this, level);
  }
};

} // namespace udf

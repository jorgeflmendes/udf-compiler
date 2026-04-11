#pragma once

#include <cdk/ast/binary_operation_node.h>

namespace udf {

/**
 * Class representing a tensor capacity node in the AST.
 *
 * Represents a unary expression node that applies a "cap" operation to a tensor expression.
 *
 * The constructor takes the following parameters:
 * - lineno: The line number where the operation appears in the source code.
 * - value: A pointer to the tensor to which the capacity operation will be applied.
 */
class tensor_cap_node : public cdk::expression_node {
    cdk::expression_node *_operand;

public:
  tensor_cap_node(int lineno, cdk::expression_node *value) noexcept
      : cdk::expression_node(lineno), _operand(value) {}

  cdk::expression_node *operand() const { return _operand; }

  void accept(basic_ast_visitor *sp, int level) override {
    sp->do_tensor_cap_node(this, level);
  }
};

} // namespace udf
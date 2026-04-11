#pragma once

#include <cdk/ast/binary_operation_node.h>

namespace udf {

/**
 * Class for describing a tensor contraction node in the AST.
 *
 * Represents a binary operation node that performs tensor contraction between two expressions.
 *
 * The constructor takes the following parameters:
 * - lineno: The line number where the tensor contraction operation appears in the source code.
 * - lhs: A pointer to the left-hand side expression node (first tensor).
 * - rhs: A pointer to the right-hand side expression node (second tensor).
 */
class tensor_contract_node : public cdk::binary_operation_node {
public:
  tensor_contract_node(int lineno, cdk::expression_node *lhs, cdk::expression_node *rhs) noexcept
      : cdk::binary_operation_node(lineno, lhs, rhs) {}

  void accept(basic_ast_visitor *sp, int level) override {
    sp->do_tensor_contract_node(this, level);
  }
};

} // namespace udf
#pragma once

#include <cdk/ast/expression_node.h>
#include <cdk/ast/sequence_node.h>

namespace udf {

/**
 * Class for describing a tensor reshape node.
 *
 * The constructor takes the following parameters:
 * - lineno: The line number where the reshape operation appears in the source code.
 * - tensor: An expression node representing the tensor to be reshaped.
 * - dims: A sequence node specifying the new dimensions for the tensor.
 */
class tensor_reshape_node : public cdk::expression_node {
  cdk::expression_node *_operand;
  cdk::sequence_node *_dims;

public:
  tensor_reshape_node(int lineno, cdk::expression_node *tensor, cdk::sequence_node *dims) noexcept
      : cdk::expression_node(lineno), _operand(tensor), _dims(dims) {}

  cdk::expression_node *operand() const { return _operand; }

  cdk::sequence_node *dims() const { return _dims; }

  void accept(basic_ast_visitor *sp, int level) override {
    sp->do_tensor_reshape_node(this, level);
  }
};

} // namespace udf

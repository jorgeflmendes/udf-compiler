#pragma once

#include <cdk/ast/expression_node.h>
#include <cdk/ast/sequence_node.h>

namespace udf {

/**
 * Class representing a tensor index node in the AST.
 *
 * Represents an lvalue node that indexes a tensor with a given set of indices.
 *
 * The constructor takes the following parameters:
 * - lineno: The line number where the indexing operation appears in the source code.
 * - tensor: An expression node representing the tensor to be indexed.
 * - index: A sequence node specifying the indices to access elements of the tensor.
 */
class tensor_index_node : public cdk::lvalue_node {
  cdk::expression_node *_operand;
  cdk::sequence_node *_index;

public:
  tensor_index_node(int lineno, cdk::expression_node *tensor, cdk::sequence_node *index) noexcept
      : cdk::lvalue_node(lineno), _operand(tensor), _index(index) {}

  cdk::expression_node *operand() const { return _operand; }

  cdk::sequence_node *index() const { return _index; }

  void accept(basic_ast_visitor *sp, int level) override {
    sp->do_tensor_index_node(this, level);
  }
};

} // namespace udf
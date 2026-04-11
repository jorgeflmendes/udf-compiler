#pragma once

#include <cdk/ast/binary_operation_node.h>
#include <cdk/ast/expression_node.h>

namespace udf {

/**
 * Class for describing a tensor dimension node in the AST.
 * Represents an expression for getting all dimensions ('dims') or a
 * specific dimension ('dim(i)').
 */
class tensor_dim_node : public cdk::expression_node {
  cdk::expression_node *_operand;
  cdk::expression_node *_index; // can be nullptr for the 'dims' case

public:
  tensor_dim_node(int lineno, cdk::expression_node *tensor,
                  cdk::expression_node *index) noexcept
      : cdk::expression_node(lineno), _operand(tensor), _index(index) {}

  tensor_dim_node(int lineno, cdk::expression_node *tensor) noexcept
      : cdk::expression_node(lineno), _operand(tensor), _index(nullptr) {}

  cdk::expression_node *operand() { return _operand; }
  cdk::expression_node *index() { return _index; }

  void accept(basic_ast_visitor *sp, int level) override {
    sp->do_tensor_dim_node(this, level);
  }
};

} // namespace udf
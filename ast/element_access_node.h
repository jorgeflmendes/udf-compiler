#pragma once

#include <cdk/ast/expression_node.h>

namespace udf {

/**
 * Class for describing an element access node (e.g., array access).
 *
 * The constructor takes the following parameters:
 * - lineno: The line number where the index expression appears in the
 * source code.
 * - base: An expression node representing the base of the index (e.g., an
 * array).
 * - offset: An expression node representing the offset/index into the base.
 */
class element_access_node : public cdk::lvalue_node {
  cdk::expression_node *_base;
  cdk::expression_node *_offset;

public:
  element_access_node(int lineno, cdk::expression_node *base,
                      cdk::expression_node *offset)
      : cdk::lvalue_node(lineno), _base(base), _offset(offset) {}

  cdk::expression_node *base() { return _base; }

  cdk::expression_node *index() { return _offset; }

  void accept(basic_ast_visitor *sp, int level) {
    sp->do_element_access_node(this, level);
  }
};

} // namespace udf

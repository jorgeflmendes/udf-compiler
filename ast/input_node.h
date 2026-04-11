#pragma once

#include <cdk/ast/lvalue_node.h>

namespace udf {

/**
 * Class for describing read nodes.
 *
 * The constructor takes the following parameters:
 * - lineno: The line number where the read statement appears in the
 *  source code.
 */
class input_node : public cdk::expression_node {

public:
  input_node(int lineno)
    : cdk::expression_node(lineno)
  {
      type(cdk::primitive_type::create(4, cdk::TYPE_UNSPEC));
  }

  void accept(basic_ast_visitor *sp, int level) {
    sp->do_input_node(this, level);
  }
};

} // namespace udf

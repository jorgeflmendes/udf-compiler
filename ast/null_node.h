#pragma once

#include <cdk/ast/expression_node.h>

namespace udf {
/**
 * Class for describing pointer nodes.
 *
 * The constructor takes the following parameters:
 * - lineno: The line number where the pointer expression appears in the
 * source code.
 * - pointer: A left-value node representing the "pointer" to be referenced.
 */
class null_node : public cdk::expression_node {

public:
  null_node(int lineno)
      : cdk::expression_node(lineno) {}

  void accept(basic_ast_visitor *sp, int level) {
    sp->do_null_node(this, level);
  }
};

} // namespace udf
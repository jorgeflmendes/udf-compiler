#pragma once

#include <cdk/ast/expression_node.h>

namespace udf {

/**
 * Class for describing continue statement nodes.
 *
 * The constructor takes the following parameters:
 * - lineno: The line number where the continue statement appears in the source
 * code.
 */
class continue_node : public cdk::basic_node {

public:
  continue_node(int lineno) : cdk::basic_node(lineno) {}

  void accept(basic_ast_visitor *sp, int level) {
    sp->do_continue_node(this, level);
  }
};

} // namespace udf


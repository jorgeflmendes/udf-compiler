#pragma once

#include <cdk/ast/expression_node.h>

namespace udf {

/**
 * Class for describing break nodes.
 *
 * The constructor takes the following parameters:
 * - lineno: The line number where the break statement appears in the source
 * code.
 */
class break_node : public cdk::basic_node {

public:
  break_node(int lineno) : cdk::basic_node(lineno) {}

  void accept(basic_ast_visitor *sp, int level) {
    sp->do_break_node(this, level);
  }
};

} // namespace udf


#pragma once

#include <cdk/ast/expression_node.h>

namespace udf {

/**
 * Class for describing memory allocation nodes.
 *
 * The constructor takes the following parameters:
 * - lineno: The line number where the memory allocation statement appears in
 * the source code.
 * - size: An expression node representing the size of the memory to be
 * allocated.
 */
class memory_allocation_node : public cdk::expression_node {
  cdk::expression_node *_size;

public:
  memory_allocation_node(int lineno, cdk::expression_node *size)
      : cdk::expression_node(lineno), _size(size) {}

  cdk::expression_node *size() { return _size; }

  void accept(basic_ast_visitor *sp, int level) {
    sp->do_memory_allocation_node(this, level);
  }
};

} // namespace udf

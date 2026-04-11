#pragma once

#include <cdk/ast/expression_node.h>
#include <cdk/ast/sequence_node.h>

namespace udf {

  /**
   * Class for describing for nodes.
   *
   * The constructor takes the following parameters:
   * - lineno: The line number where the for loop appears in the source code.
   * - init: The initialization sequence (can be variable declarations, assignments, etc).
   * - condition: The condition sequence for the loop.
   * - update: The update sequence (e.g., increment).
   * - block: The block node representing the body of the loop.
   */
  class for_node : public cdk::basic_node {
    cdk::sequence_node *_init;
    cdk::sequence_node *_condition;
    cdk::sequence_node *_update;
    cdk::basic_node *_block;

  public:
    for_node(int lineno, cdk::sequence_node *init, cdk::sequence_node *condition, cdk::sequence_node *update, cdk::basic_node *block)
        : cdk::basic_node(lineno), _init(init), _condition(condition), _update(update), _block(block) {}

    cdk::sequence_node *init() { return _init; }
    cdk::sequence_node *condition() { return _condition; }
    cdk::sequence_node *update() { return _update; }
    cdk::basic_node *block() { return _block; }

    void accept(basic_ast_visitor *sp, int level) {
      sp->do_for_node(this, level);
    }
  };

} // namespace udf

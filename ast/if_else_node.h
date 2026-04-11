#pragma once

#include <cdk/ast/expression_node.h>

namespace udf {

/**
 * Class for describing if-then-else nodes.
 *
 * The constructor takes the following parameters:
 * - lineno: The line number where the if-then-else statement appears in the
 * source code.
 * - condition: An expression node representing the condition of the if
 * statement.
 * - thenblock: A basic node representing the block of code to be executed if
 * the condition is true.
 * - elseblock: A basic node representing the block of code to be executed if
 * the condition is false.
 */
class if_else_node : public cdk::basic_node {
  cdk::expression_node *_condition;
  cdk::basic_node *_thenblock, *_elseblock;

public:
  if_else_node(int lineno, cdk::expression_node *condition,
               cdk::basic_node *thenblock, cdk::basic_node *elseblock)
      : cdk::basic_node(lineno), _condition(condition), _thenblock(thenblock),
        _elseblock(elseblock) {}

  cdk::expression_node *condition() { return _condition; }

  cdk::basic_node *thenblock() { return _thenblock; }

  cdk::basic_node *elseblock() { return _elseblock; }

  void accept(basic_ast_visitor *sp, int level) {
    sp->do_if_else_node(this, level);
  }
};

} // namespace udf

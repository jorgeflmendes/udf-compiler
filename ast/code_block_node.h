#pragma once

#include <cdk/ast/sequence_node.h>

namespace udf {
/**
 * Class for describing a code block.
 *
 * The constructor takes the following parameters:
 * - lineno: The line number where the code block appears in the source code.
 * - declarations: A sequence node representing the variable declarations in the
 * code block (optional).
 * - instructions: A sequence node representing the instructions in the code
 * block.
 */
class code_block_node : public cdk::sequence_node {

  cdk::sequence_node *_declarations;
  cdk::sequence_node *_statements;

public:
  code_block_node(int lineno, cdk::sequence_node *declarations, cdk::sequence_node *instructions)
    : cdk::sequence_node(lineno),
      _declarations(declarations ? declarations : new cdk::sequence_node(lineno)),
      _statements(instructions ? instructions : new cdk::sequence_node(lineno)) {}

  cdk::sequence_node *declarations() { return _declarations; }

  cdk::sequence_node *statements() { return _statements; }

  void accept(basic_ast_visitor *sp, int level) {
    sp->do_code_block_node(this, level);
  }
};

} // namespace udf

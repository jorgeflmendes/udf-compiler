#pragma once

#include <cdk/ast/expression_node.h>

namespace udf {

/**
 * Class for describing write nodes.
 *
 * The constructor takes the following parameters:
 * - lineno: The line number where the write statement appears in the
 * source code.
 * - argument: A sequence node representing the arguments to be written.
 * - new_line: A boolean to separate into a write and writeln
 */
class write_node : public cdk::basic_node {
  cdk::sequence_node *_arguments;
  bool _new_line;

public:
  write_node(int lineno, cdk::sequence_node *argument, bool new_line)
      : cdk::basic_node(lineno), _arguments(argument), _new_line(new_line){}

  cdk::sequence_node *arguments() { return _arguments; }

  bool new_line() { return _new_line; }

  void accept(basic_ast_visitor *sp, int level) {
    sp->do_write_node(this, level);
  }
};

} // namespace udf


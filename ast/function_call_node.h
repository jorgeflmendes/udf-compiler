#pragma once

#include <cdk/ast/expression_node.h>
#include <cdk/ast/sequence_node.h>

namespace udf {

/*
 * Class for describing function call nodes.
 *
 * The constructor takes the following parameters:
 * - lineno: The line number where the function call appears in the
 * source code.
 * - name: The name of the function being called.
 * - arguments: A sequence node representing the function's arguments. If null,
 * an empty sequence is created.
 */
class function_call_node : public cdk::expression_node {
  std::string _name;
  cdk::sequence_node *_arguments;

public:
  function_call_node(int lineno, const std::string &name,
                     cdk::sequence_node *arguments = nullptr)
      : cdk::expression_node(lineno), _name(name), _arguments(arguments) {
    if (!_arguments) {
      _arguments = new cdk::sequence_node(lineno);
    }
  }

  const std::string &name() const { return _name; }

  cdk::sequence_node *arguments() { return _arguments; }

  void arguments(cdk::sequence_node *arguments) { _arguments = arguments; }

  void accept(basic_ast_visitor *sp, int level) {
    sp->do_function_call_node(this, level);
  }
};
} // namespace udf

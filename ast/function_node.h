#pragma once

#include <cdk/ast/expression_node.h>
#include <cdk/ast/sequence_node.h>

namespace udf {

/*
 * Class for describing function declaration nodes.
 *
 * The constructor takes the following parameters:
 * - lineno: The line number where the function declaration appears in the
 * source code.
 * - access_specifier: The access specifier id of the function (e.g., public).
 * - type: The return type of the function.
 * - name: The name of the function.
 * - arguments: A sequence node representing the function's arguments.
 * - body: A sequence node representing the function's body.
 */
class function_node : public cdk::expression_node {

  int _access_specifier;
  std::string _identifier;
  cdk::sequence_node *_arguments;
  cdk::sequence_node *_body;
  // cdk::basic_type *_type; the type is stored in the typed node (grandparent
  // class), so there is no need to store it here

public:
  function_node(int lineno, int access_specifier, std::shared_ptr<cdk::basic_type> type, const std::string &name,
        cdk::sequence_node *arguments, cdk::sequence_node *body):
        cdk::expression_node(lineno), _access_specifier(access_specifier), _identifier(name), _arguments(arguments), _body(body) {
    this->type(type);
  }

  function_node(int lineno, int access_specifier, cdk::sequence_node *arguments, cdk::sequence_node *body, const std::string &name)
      : cdk::expression_node(lineno), _access_specifier(access_specifier), _identifier(name), _arguments(arguments), _body(body) {

    this->type(cdk::primitive_type::create(0, cdk::TYPE_VOID));
  }

  int access_specifier() const { return _access_specifier; }

  const std::string &identifier() const { return _identifier; }

  cdk::sequence_node *arguments() { return _arguments; }

  cdk::sequence_node *body() { return _body; }

  void accept(basic_ast_visitor *sp, int level) {
    sp->do_function_node(this, level);
  }
};
} // namespace udf

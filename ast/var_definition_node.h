#pragma once

#include <cdk/ast/expression_node.h>
#include <cdk/types/basic_type.h>
#include <memory>

namespace udf {

/**
 * Class for describing variable definition nodes.
 *
 * The constructor takes the following parameters:
 * - lineno: The line number where the variable declaration appears in the
 * source code.
 * - access_specifier: The variable's access specifier (e.g., public).
 * - name: The name of the variable.
 * - type: A shared pointer to the basic type of the variable.
 */
class var_definition_node : public cdk::expression_node {
  int _access_specifier;
  std::string _name;
  cdk::expression_node *_value;
  // cdk::basic_type *_type; the type is stored in the typed node (grandparent
  // class), so there is no need to store it here

public:
  var_definition_node(int lineno, int access_specifier, const std::string &name,
                      std::shared_ptr<cdk::basic_type> type,
                      cdk::expression_node *value)
      : cdk::expression_node(lineno), _access_specifier(access_specifier),
        _name(name), _value(value) {
    this->type(type); // set the type in the grandparent class
  }

  var_definition_node(int lineno, int access_specifier, const std::string &name,
                      std::shared_ptr<cdk::basic_type> type)
      : cdk::expression_node(lineno), _access_specifier(access_specifier),
        _name(name), _value(nullptr) {
    this->type(type); // set the type in the grandparent class
  }

  int access_specifier() const { return _access_specifier; }

  const std::string &name() const { return _name; }

  cdk::expression_node *value() const { return _value; }

  void accept(basic_ast_visitor *sp, int level) {
    sp->do_var_definition_node(this, level);
  }
};

} // namespace udf

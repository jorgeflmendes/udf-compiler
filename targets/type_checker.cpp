#include "targets/type_checker.h"
#include ".auto/all_nodes.h"
#include <cdk/types/types.h>
#include <vector>

#define ASSERT_UNSPEC                                                          \
  {                                                                            \
    if (node->type() != nullptr &&                                             \
        !node->is_typed(cdk::TYPE_UNSPEC))                                     \
      return;                                                                  \
  }

udf::type_checker::~type_checker() { os().flush(); }

//---------------------------------------------------------------------------
//          BASIC TRAVERSAL
//---------------------------------------------------------------------------

void udf::type_checker::do_data_node(cdk::data_node *const node, int lvl) {
  // Intentionally empty.
}
void udf::type_checker::do_nil_node(cdk::nil_node *const node, int lvl) {
  // Intentionally empty.
}
void udf::type_checker::do_unary_position_node(
    udf::unary_position_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->argument()->accept(this, lvl + 2);
  // The position operator yields a pointer to the operand type.
  node->type(cdk::reference_type::create(4, node->argument()->type()));
}

void udf::type_checker::do_sequence_node(cdk::sequence_node *const node,
                                         int lvl) {
  for (size_t i = 0; i < node->size(); i++) {
    node->node(i)->accept(this, lvl);
  }
}

void udf::type_checker::do_program_node(udf::program_node *const node,
                                        int lvl) {
  if (node->statements()) {
    node->statements()->accept(this, lvl);
  }
}

void udf::type_checker::do_code_block_node(udf::code_block_node *const node,
                                           int lvl) {
  if (node->declarations())
    node->declarations()->accept(this, lvl);
  if (node->statements())
    node->statements()->accept(this, lvl);
}

void udf::type_checker::do_break_node(udf::break_node *const node, int lvl) {
  // Control-flow validation is handled in later phases.
}
void udf::type_checker::do_continue_node(udf::continue_node *const node,
                                         int lvl) {
  // Control-flow validation is handled in later phases.
}

void udf::type_checker::do_return_node(udf::return_node *const node,
                                       int lvl) {
  if (node->return_value()) {
    node->return_value()->accept(this, lvl + 2);
  }
}

//---------------------------------------------------------------------------
//          LITERALS AND BASIC EXPRESSIONS
//---------------------------------------------------------------------------

void udf::type_checker::do_double_node(cdk::double_node *const node,
                                       int lvl) {
  ASSERT_UNSPEC;
  node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
}

void udf::type_checker::do_integer_node(cdk::integer_node *const node,
                                        int lvl) {
  ASSERT_UNSPEC;
  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
}

void udf::type_checker::do_string_node(cdk::string_node *const node,
                                       int lvl) {
  ASSERT_UNSPEC;
  node->type(cdk::primitive_type::create(4, cdk::TYPE_STRING));
}

void udf::type_checker::do_null_node(udf::null_node *const node, int lvl) {
  ASSERT_UNSPEC;
  // A null pointer is represented as a reference to an unspecified target.
  node->type(cdk::reference_type::create(4, nullptr));
}

void udf::type_checker::do_variable_node(cdk::variable_node *const node,
                                         int lvl) {
  ASSERT_UNSPEC;
  const std::string &id = node->name();
  auto symbol = _symtab.find(id);
  if (symbol) {
    node->type(symbol->type());
  } else {
    throw std::string("undeclared variable '" + id + "'");
  }
}

void udf::type_checker::do_element_access_node(
    udf::element_access_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->base()->accept(this, lvl + 2);
  if (!node->base()->is_typed(cdk::TYPE_POINTER)) {
    throw std::string("pointer expression expected in index left-value");
  }

  node->index()->accept(this, lvl + 2);
  if (!node->index()->is_typed(cdk::TYPE_INT)) {
    throw std::string("integer expression expected in left-value index");
  }

  // The resulting type is the pointee type.
  auto btype = cdk::reference_type::cast(node->base()->type());
  node->type(btype->referenced());
}

void udf::type_checker::do_rvalue_node(cdk::rvalue_node *const node,
                                       int lvl) {
  ASSERT_UNSPEC;
  node->lvalue()->accept(this, lvl);
  node->type(node->lvalue()->type());
}

void udf::type_checker::do_assignment_node(cdk::assignment_node *const node,
                                           int lvl) {
  ASSERT_UNSPEC;
  node->lvalue()->accept(this, lvl + 4);
  node->rvalue()->accept(this, lvl + 4);

  if (node->lvalue()->is_typed(cdk::TYPE_INT)) {
    if (node->rvalue()->is_typed(cdk::TYPE_INT)) {
      node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
    } else if (node->rvalue()->is_typed(cdk::TYPE_UNSPEC)) {
      node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
      node->rvalue()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
    } else {
      throw std::string("wrong type in assignment to integer");
    }
  } else if (node->lvalue()->is_typed(cdk::TYPE_POINTER)) {
    // Propagate the expected pointer type into allocation expressions.
    if (auto ralloc =
            dynamic_cast<udf::memory_allocation_node *>(node->rvalue())) {
      ralloc->type(node->lvalue()->type());
    }

    if (node->rvalue()->is_typed(cdk::TYPE_POINTER)) {
      node->type(node->rvalue()->type());
    } else if (node->rvalue()->is_typed(cdk::TYPE_UNSPEC) &&
               dynamic_cast<udf::null_node *>(node->rvalue())) {
      node->type(node->lvalue()->type());
    } else {
      throw std::string("wrong type in assignment to pointer");
    }
  } else if (node->lvalue()->is_typed(cdk::TYPE_DOUBLE)) {
    if (node->rvalue()->is_typed(cdk::TYPE_DOUBLE) ||
        node->rvalue()->is_typed(cdk::TYPE_INT)) {
      node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
    } else if (node->rvalue()->is_typed(cdk::TYPE_UNSPEC)) {
      node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
      node->rvalue()->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
    } else {
      throw std::string("wrong type in assignment to real");
    }
  } else if (node->lvalue()->is_typed(cdk::TYPE_STRING)) {
    if (node->rvalue()->is_typed(cdk::TYPE_STRING)) {
      node->type(cdk::primitive_type::create(4, cdk::TYPE_STRING));
    } else if (node->rvalue()->is_typed(cdk::TYPE_UNSPEC)) {
      node->type(cdk::primitive_type::create(4, cdk::TYPE_STRING));
      node->rvalue()->type(cdk::primitive_type::create(4, cdk::TYPE_STRING));
    } else {
      throw std::string("wrong type in assignment to string");
    }
  } else if (node->lvalue()->is_typed(cdk::TYPE_FUNCTIONAL)) {
    if (node->rvalue()->is_typed(cdk::TYPE_FUNCTIONAL)) {
      node->type(node->lvalue()->type());
    } else {
      throw std::string("wrong type in assignment to function pointer");
    }
  } else if (node->lvalue()->is_typed(cdk::TYPE_TENSOR)) {
    if (node->rvalue()->is_typed(cdk::TYPE_TENSOR)) {
      if (node->lvalue()->type() == node->rvalue()->type()) {
        node->type(node->lvalue()->type());
      } else {
        throw std::string("mismatched tensor types in assignment: expected '" +
                          node->lvalue()->type()->to_string() + "' but got '" +
                          node->rvalue()->type()->to_string() + "'");
      }
    } else {
      throw std::string("cannot assign non-tensor value to a tensor variable");
    }
  } else {
    throw std::string("wrong types in assignment");
  }
}

void udf::type_checker::do_var_definition_node(
    udf::var_definition_node *const node, int lvl) {
  const std::string &id = node->name();

  // Avoid reprocessing declarations already installed in the local scope.
  if (_symtab.find_local(id) != nullptr) {
    return;
  }

  if (node->value() != nullptr) {
    node->value()->accept(this, lvl + 2);

    // Infer the declared type from the initializer when using `auto`.
    if (node->is_typed(cdk::TYPE_UNSPEC)) {
      node->type(node->value()->type());
    }

    if (node->is_typed(cdk::TYPE_INT)) {
      if (!node->value()->is_typed(cdk::TYPE_INT))
        throw std::string("wrong type for initializer (integer expected)");
    } else if (node->is_typed(cdk::TYPE_DOUBLE)) {
      if (!node->value()->is_typed(cdk::TYPE_INT) &&
          !node->value()->is_typed(cdk::TYPE_DOUBLE)) {
        throw std::string(
            "wrong type for initializer (integer or double expected)");
      }
    } else if (node->is_typed(cdk::TYPE_STRING)) {
      if (!node->value()->is_typed(cdk::TYPE_STRING)) {
        throw std::string("wrong type for initializer (string expected)");
      }
    } else if (node->is_typed(cdk::TYPE_POINTER)) {
      if (!node->value()->is_typed(cdk::TYPE_POINTER) &&
          !dynamic_cast<udf::null_node *>(node->value())) {
        throw std::string(
            "wrong type for initializer (pointer or null expected)");
      }
    } else if (node->is_typed(cdk::TYPE_FUNCTIONAL)) {
      if (!node->value()->is_typed(cdk::TYPE_FUNCTIONAL)) {
        throw std::string("wrong type for initializer (function expected)");
      }
    } else if (node->is_typed(cdk::TYPE_TENSOR)) {
      if (!node->value()->is_typed(cdk::TYPE_TENSOR)) {
        throw std::string("wrong type for initializer (tensor expected)");
      }
      if (node->type() != node->value()->type()) {
        throw std::string("mismatched tensor types for initializer: variable "
                          "is '" +
                          node->type()->to_string() + "' but value is '" +
                          node->value()->type()->to_string() + "'");
      }
    } else {
      throw std::string("unknown type for initializer");
    }
  } else {
    if (node->is_typed(cdk::TYPE_UNSPEC)) {
      throw std::string("'auto' requires an initializer");
    }
  }

  auto symbol = udf::make_symbol(false, node->access_specifier(), node->type(),
                                 id, (bool)node->value(), false);

  if (_symtab.insert(id, symbol)) {
    _parent->set_new_symbol(symbol);
  } else {
    throw std::string("variable '" + id + "' redeclared");
  }
}

void udf::type_checker::do_auto_node(udf::auto_node *const node, int lvl) {
  ASSERT_UNSPEC;
  if (node->value()) {
    node->value()->accept(this, lvl + 2);
    node->type(node->value()->type());
  } else {
    throw std::string("'auto' requires an initializer");
  }
}

//---------------------------------------------------------------------------
//          UNARY AND BINARY OPERATORS
//---------------------------------------------------------------------------

void udf::type_checker::do_unary_minus_node(cdk::unary_minus_node *const node,
                                            int lvl) {
  ASSERT_UNSPEC;
  node->argument()->accept(this, lvl);
  if (!node->argument()->is_typed(cdk::TYPE_INT) &&
      !node->argument()->is_typed(cdk::TYPE_DOUBLE)) {
    throw std::string("wrong type in argument of unary minus");
  }
  node->type(node->argument()->type());
}

void udf::type_checker::do_unary_plus_node(cdk::unary_plus_node *const node,
                                           int lvl) {
  ASSERT_UNSPEC;
  node->argument()->accept(this, lvl);
  if (!node->argument()->is_typed(cdk::TYPE_INT) &&
      !node->argument()->is_typed(cdk::TYPE_DOUBLE)) {
    throw std::string("wrong type in argument of unary plus");
  }
  node->type(node->argument()->type());
}

void udf::type_checker::do_not_node(cdk::not_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->argument()->accept(this, lvl + 2);
  if (node->argument()->is_typed(cdk::TYPE_INT)) {
    node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  } else {
    throw std::string("wrong type in unary logical expression");
  }
}

//----------------------------------------------------------------------------
// HELPER ROUTINES FOR BINARY OPERATIONS
//----------------------------------------------------------------------------

void udf::type_checker::do_IntExpression(
    cdk::binary_operation_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->left()->accept(this, lvl + 2);
  if (!node->left()->is_typed(cdk::TYPE_INT)) {
    throw std::string("integer expression expected in binary operator (left)");
  }

  node->right()->accept(this, lvl + 2);
  if (!node->right()->is_typed(cdk::TYPE_INT)) {
    throw std::string(
        "integer expression expected in binary operator (right)");
  }

  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
}

void udf::type_checker::do_IntDoubleExpression(
    cdk::binary_operation_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->left()->accept(this, lvl + 2);
  node->right()->accept(this, lvl + 2);

  if (node->left()->is_typed(cdk::TYPE_DOUBLE) &&
      node->right()->is_typed(cdk::TYPE_DOUBLE)) {
    node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
  } else if (node->left()->is_typed(cdk::TYPE_DOUBLE) &&
             node->right()->is_typed(cdk::TYPE_INT)) {
    node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
  } else if (node->left()->is_typed(cdk::TYPE_INT) &&
             node->right()->is_typed(cdk::TYPE_DOUBLE)) {
    node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
  } else if (node->left()->is_typed(cdk::TYPE_INT) &&
             node->right()->is_typed(cdk::TYPE_INT)) {
    node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  } else if (node->left()->is_typed(cdk::TYPE_TENSOR) &&
             node->right()->is_typed(cdk::TYPE_TENSOR)) {
    if (node->left()->type() != node->right()->type()) {
      throw std::string("mismatched tensor shapes for binary operation");
    }
    node->type(node->left()->type());
  } else if (node->left()->is_typed(cdk::TYPE_TENSOR) &&
             (node->right()->is_typed(cdk::TYPE_INT) ||
              node->right()->is_typed(cdk::TYPE_DOUBLE))) {
    node->type(node->left()->type());
  } else if ((node->left()->is_typed(cdk::TYPE_INT) ||
              node->left()->is_typed(cdk::TYPE_DOUBLE)) &&
             node->right()->is_typed(cdk::TYPE_TENSOR)) {
    node->type(node->right()->type());
  } else {
    throw std::string("wrong types in binary expression");
  }
}

void udf::type_checker::do_PointerIntDoubleExpression(
    cdk::binary_operation_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->left()->accept(this, lvl + 2);
  node->right()->accept(this, lvl + 2);

  if (node->left()->is_typed(cdk::TYPE_POINTER) &&
      node->right()->is_typed(cdk::TYPE_INT)) {
    node->type(node->left()->type());
  } else if (node->left()->is_typed(cdk::TYPE_INT) &&
             node->right()->is_typed(cdk::TYPE_POINTER)) {
    node->type(node->right()->type());
  } else {
    // Otherwise, defer to the regular numeric compatibility rules.
    do_IntDoubleExpression(node, lvl);
  }
}

void udf::type_checker::do_NumericLogicalExpression(
    cdk::binary_operation_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->left()->accept(this, lvl + 2);
  node->right()->accept(this, lvl + 2);
  if (node->left()->type() != node->right()->type()) {
    if (!((node->left()->is_typed(cdk::TYPE_INT) &&
           node->right()->is_typed(cdk::TYPE_DOUBLE)) ||
          (node->left()->is_typed(cdk::TYPE_DOUBLE) &&
           node->right()->is_typed(cdk::TYPE_INT))))
      throw std::string("same type expected on both sides of "
                        "comparison operator");
  }
  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
}

void udf::type_checker::do_BooleanLogicalExpression(
    cdk::binary_operation_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->left()->accept(this, lvl + 2);
  if (!node->left()->is_typed(cdk::TYPE_INT)) {
    throw std::string("integer expression expected in binary expression");
  }

  node->right()->accept(this, lvl + 2);
  if (!node->right()->is_typed(cdk::TYPE_INT)) {
    throw std::string("integer expression expected in binary expression");
  }

  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
}

void udf::type_checker::do_GeneralLogicalExpression(
    cdk::binary_operation_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->left()->accept(this, lvl + 2);
  node->right()->accept(this, lvl + 2);

  if (node->left()->type() != node->right()->type()) {
    // Equality comparisons between pointers and null are explicitly allowed.
    if ((node->left()->is_typed(cdk::TYPE_POINTER) &&
         dynamic_cast<udf::null_node *>(node->right())) ||
        (dynamic_cast<udf::null_node *>(node->left()) &&
         node->right()->is_typed(cdk::TYPE_POINTER))) {
      // Accepted combination.
    } else {
      throw std::string(
          "same type expected on both sides of equality operator");
    }
  }
  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
}

//----------------------------------------------------------------------------
// BINARY OPERATION DISPATCHERS
//----------------------------------------------------------------------------

void udf::type_checker::do_add_node(cdk::add_node *const node, int lvl) {
  do_PointerIntDoubleExpression(node, lvl);
}
void udf::type_checker::do_sub_node(cdk::sub_node *const node, int lvl) {
  do_PointerIntDoubleExpression(node, lvl);
}
void udf::type_checker::do_mul_node(cdk::mul_node *const node, int lvl) {
  do_IntDoubleExpression(node, lvl);
}
void udf::type_checker::do_div_node(cdk::div_node *const node, int lvl) {
  do_IntDoubleExpression(node, lvl);
}
void udf::type_checker::do_mod_node(cdk::mod_node *const node, int lvl) {
  do_IntExpression(node, lvl);
}
void udf::type_checker::do_gt_node(cdk::gt_node *const node, int lvl) {
  do_NumericLogicalExpression(node, lvl);
}
void udf::type_checker::do_ge_node(cdk::ge_node *const node, int lvl) {
  do_NumericLogicalExpression(node, lvl);
}
void udf::type_checker::do_le_node(cdk::le_node *const node, int lvl) {
  do_NumericLogicalExpression(node, lvl);
}
void udf::type_checker::do_lt_node(cdk::lt_node *const node, int lvl) {
  do_NumericLogicalExpression(node, lvl);
}
void udf::type_checker::do_eq_node(cdk::eq_node *const node, int lvl) {
  do_GeneralLogicalExpression(node, lvl);
}
void udf::type_checker::do_ne_node(cdk::ne_node *const node, int lvl) {
  do_GeneralLogicalExpression(node, lvl);
}
void udf::type_checker::do_and_node(cdk::and_node *const node, int lvl) {
  do_BooleanLogicalExpression(node, lvl);
}
void udf::type_checker::do_or_node(cdk::or_node *const node, int lvl) {
  do_BooleanLogicalExpression(node, lvl);
}

//---------------------------------------------------------------------------
//          STATEMENTS AND HIGH-LEVEL CONSTRUCTS
//---------------------------------------------------------------------------

void udf::type_checker::do_evaluation_node(udf::evaluation_node *const node,
                                           int lvl) {
  node->argument()->accept(this, lvl + 2);
}
void udf::type_checker::do_write_node(udf::write_node *const node, int lvl) {
  node->arguments()->accept(this, lvl + 2);
}
void udf::type_checker::do_input_node(udf::input_node *const node, int lvl) {
  // The node constructor fixes the input type contract.
}

void udf::type_checker::do_memory_allocation_node(
    udf::memory_allocation_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->size()->accept(this, lvl + 2);
  if (!node->size()->is_typed(cdk::TYPE_INT)) {
    throw std::string("integer expression expected in allocation expression");
  }
  // Untyped allocations default to returning a void pointer.
  node->type(cdk::reference_type::create(
      4, cdk::primitive_type::create(1, cdk::TYPE_VOID)));
}

void udf::type_checker::do_for_node(udf::for_node *const node, int lvl) {
  if (node->init())
    node->init()->accept(this, lvl + 4);

  if (node->condition()) {
    if (node->condition()->size() > 1) {
      throw std::string("for loop condition must be a single expression");
    }

    if (node->condition()->size() == 1) {
      auto condition_expr =
          dynamic_cast<cdk::expression_node *>(node->condition()->node(0));
      if (condition_expr == nullptr) {
        throw std::string(
            "for loop condition sequence must contain an expression");
      }

      condition_expr->accept(this, lvl + 4);
      if (!condition_expr->is_typed(cdk::TYPE_INT)) {
        throw std::string("expected integer condition in for loop");
      }
    }
  }

  if (node->update())
    node->update()->accept(this, lvl + 4);

  node->block()->accept(this, lvl + 4);
}

void udf::type_checker::do_if_node(udf::if_node *const node, int lvl) {
  node->condition()->accept(this, lvl + 4);
  if (!node->condition()->is_typed(cdk::TYPE_INT))
    throw std::string("expected integer condition");
}

void udf::type_checker::do_if_else_node(udf::if_else_node *const node,
                                        int lvl) {
  node->condition()->accept(this, lvl + 4);
  if (!node->condition()->is_typed(cdk::TYPE_INT))
    throw std::string("expected integer condition");
}

void udf::type_checker::do_function_call_node(
    udf::function_call_node *const node, int lvl) {
  ASSERT_UNSPEC;

  auto symbol = _symtab.find(node->name());
  if (symbol == nullptr) {
    throw std::string("function '" + node->name() + "' is not declared");
  }
  if (!symbol->isFunction()) {
    throw std::string("'" + node->name() + "' is not a function");
  }

  auto ftype = cdk::functional_type::cast(symbol->type());
  node->type(ftype->output(0)); // Function calls always expose a single output type.

  if (node->arguments()->size() == ftype->input_length()) {
    node->arguments()->accept(this, lvl + 4);
    for (size_t ax = 0; ax < node->arguments()->size(); ax++) {
      auto arg = (cdk::expression_node *)node->arguments()->node(ax);
      if (arg->type() == ftype->input(ax))
        continue;
      if (ftype->input(ax)->name() == cdk::TYPE_DOUBLE &&
          arg->is_typed(cdk::TYPE_INT))
        continue;
      throw std::string("type mismatch for argument " +
                        std::to_string(ax + 1) + " of '" + node->name() +
                        "'");
    }
  } else {
    throw std::string("number of arguments in call (" +
                      std::to_string(node->arguments()->size()) +
                      ") must match definition (" +
                      std::to_string(ftype->input_length()) + ")");
  }
}

void udf::type_checker::do_function_node(udf::function_node *const node,
                                         int lvl) {
  const auto name = node->identifier();

  auto symbol = _symtab.find(name);
  if (symbol == nullptr) {
    symbol = udf::make_symbol(false, node->access_specifier(), node->type(),
                              name, false, true);
    if (!_symtab.insert(name, symbol)) {
      throw std::string("internal error: failed to insert new function symbol '" +
                        name + "'");
    }
    _parent->set_new_symbol(symbol);
  }

  _symtab.push();

  if (node->arguments()) {
    node->arguments()->accept(this, lvl + 2);
  }

  if (node->body()) {
    node->body()->accept(this, lvl + 2);
  }

  _symtab.pop();
}

void udf::type_checker::do_size_of_node(udf::size_of_node *const node,
                                        int lvl) {
  ASSERT_UNSPEC;
  node->argument()->accept(this, lvl + 2);
  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
}

//---------------------------------------------------------------------------
//          TENSOR NODES
//---------------------------------------------------------------------------

void udf::type_checker::check_tensor_elements(cdk::sequence_node *sequence,
                                              int lvl) {
  for (cdk::basic_node *element_node : sequence->nodes()) {
    if (auto inner_sequence =
            dynamic_cast<cdk::sequence_node *>(element_node)) {
      check_tensor_elements(inner_sequence, lvl);
    } else if (auto expr =
                   dynamic_cast<cdk::expression_node *>(element_node)) {
      expr->accept(this, lvl + 2);
      if (!expr->is_typed(cdk::TYPE_INT) &&
          !expr->is_typed(cdk::TYPE_DOUBLE)) {
        throw std::string(
            "tensor elements must be of type integer or double");
      }
    } else {
      throw std::string("invalid node type in tensor literal");
    }
  }
}

std::vector<size_t>
udf::type_checker::infer_tensor_dims(cdk::sequence_node *sequence) {
  std::vector<size_t> dims;
  cdk::sequence_node *current_sequence = sequence;

  while (current_sequence != nullptr && current_sequence->size() > 0) {
    dims.push_back(current_sequence->size());
    current_sequence =
        dynamic_cast<cdk::sequence_node *>(current_sequence->node(0));
  }
  return dims;
}

void udf::type_checker::do_tensor_node(udf::tensor_node *const node,
                                       int lvl) {
  ASSERT_UNSPEC;
  std::vector<size_t> dims = infer_tensor_dims(node->content());
  check_tensor_elements(node->content(), lvl);
  node->type(cdk::tensor_type::create(dims));
}

void udf::type_checker::do_tensor_index_node(
    udf::tensor_index_node *const node, int lvl) {
  ASSERT_UNSPEC;

  node->operand()->accept(this, lvl + 2);
  auto tensor_type = cdk::tensor_type::cast(node->operand()->type());
  if (tensor_type == nullptr) {
    throw std::string("indexing target must be a tensor");
  }

  node->index()->accept(this, lvl + 2);
  if (node->index()->size() != tensor_type->n_dims()) {
    throw std::string("incorrect number of indices for tensor rank");
  }
  for (size_t i = 0; i < node->index()->size(); ++i) {
    auto index_expr = (cdk::expression_node *)node->index()->node(i);
    if (!index_expr->is_typed(cdk::TYPE_INT)) {
      throw std::string("tensor indices must be integer expressions");
    }
  }

  // Tensor element access always yields a scalar double value.
  node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
}

void udf::type_checker::do_tensor_reshape_node(
    udf::tensor_reshape_node *const node, int lvl) {
  ASSERT_UNSPEC;

  node->operand()->accept(this, lvl + 2);
  auto original_type = cdk::tensor_type::cast(node->operand()->type());
  if (original_type == nullptr) {
    throw std::string("reshape target must be a tensor");
  }

  std::vector<size_t> new_dims;
  size_t new_capacity = 1;
  for (cdk::basic_node *dim_node : node->dims()->nodes()) {
    auto literal = dynamic_cast<cdk::integer_node *>(dim_node);
    if (!literal || literal->value() <= 0) {
      throw std::string(
          "reshape dimensions must be positive integer literals");
    }
    new_dims.push_back(literal->value());
    new_capacity *= literal->value();
  }

  if (original_type->size() != new_capacity) {
    throw std::string("reshape changes tensor capacity, which is not allowed");
  }

  node->type(cdk::tensor_type::create(new_dims));
}

void udf::type_checker::do_tensor_contract_node(
    udf::tensor_contract_node *const node, int lvl) {
  ASSERT_UNSPEC;

  node->left()->accept(this, lvl + 2);
  auto left_type = cdk::tensor_type::cast(node->left()->type());
  if (left_type == nullptr)
    throw std::string("left operand of contraction must be a tensor");

  node->right()->accept(this, lvl + 2);
  auto right_type = cdk::tensor_type::cast(node->right()->type());
  if (right_type == nullptr)
    throw std::string("right operand of contraction must be a tensor");

  if (left_type->n_dims() == 0 || right_type->n_dims() == 0)
    throw std::string("cannot contract zero-rank tensors");

  // The trailing dimension of the left operand must match the leading one of
  // the right operand.
  if (left_type->dim(left_type->n_dims() - 1) != right_type->dim(0)) {
    throw std::string(
        "last dimension of the first tensor must match the first dimension of "
        "the second for contraction");
  }

  // The resulting shape keeps the outer dimensions of the left operand,
  // followed by the non-contracted inner dimensions of the right operand.
  std::vector<size_t> result_dims = left_type->dims();
  result_dims.pop_back(); // Remove the contracted dimension.

  std::vector<size_t> right_dims_vec = right_type->dims();
  if (right_dims_vec.size() > 1) {
    result_dims.insert(result_dims.end(), right_dims_vec.begin() + 1,
                       right_dims_vec.end());
  }

  node->type(cdk::tensor_type::create(result_dims));
}

void udf::type_checker::do_tensor_rank_node(
    udf::tensor_rank_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->operand()->accept(this, lvl + 2);
  if (!node->operand()->is_typed(cdk::TYPE_TENSOR)) {
    throw std::string("operand of 'rank' operator must be a tensor");
  }
  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
}

void udf::type_checker::do_tensor_dim_node(udf::tensor_dim_node *const node,
                                           int lvl) {
  ASSERT_UNSPEC;
  node->operand()->accept(this, lvl + 2);
  if (!node->operand()->is_typed(cdk::TYPE_TENSOR)) {
    throw std::string(
        "operand of 'dim' or 'dims' operator must be a tensor");
  }

  if (node->index()) {
    // `t.dim(i)` returns the size of a single dimension.
    node->index()->accept(this, lvl + 2);
    if (!node->index()->is_typed(cdk::TYPE_INT)) {
      throw std::string(
          "tensor dimension index must be an integer expression");
    }
    node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  } else {
    // `t.dims` returns a pointer to the runtime dimension array.
    node->type(cdk::reference_type::create(
        4, cdk::primitive_type::create(4, cdk::TYPE_INT)));
  }
}

void udf::type_checker::do_tensor_cap_node(udf::tensor_cap_node *const node,
                                           int lvl) {
  ASSERT_UNSPEC;
  node->operand()->accept(this, lvl + 2);
  if (!node->operand()->is_typed(cdk::TYPE_TENSOR)) {
    throw std::string("operand of 'capacity' operator must be a tensor");
  }
  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
}

#include <string>
#include "targets/xml_writer.h"
#include "targets/type_checker.h"
#include ".auto/all_nodes.h"

//---------------------------------------------------------------------------
// BASIC AST NODES
//---------------------------------------------------------------------------

void udf::xml_writer::do_sequence_node(cdk::sequence_node *const node,
                                        int lvl) {
  // processes a sequence of nodes
  os() << std::string(lvl, ' ') << "<sequence_node size='" << node->size()
       << "'>" << std::endl;
  for (size_t i = 0; i < node->size(); i++)
    node->node(i)->accept(this, lvl + 2);
  closeTag(node, lvl);
}

void udf::xml_writer::do_nil_node(cdk::nil_node *const node, int lvl) {
  // handles a nil/empty node
  openTag(node, lvl);
  closeTag(node, lvl);
}

void udf::xml_writer::do_data_node(cdk::data_node *const node, int lvl) {
  // a generic data node, not used in this grammar
}

//---------------------------------------------------------------------------
// LITERAL NODES
//---------------------------------------------------------------------------

void udf::xml_writer::do_null_node(udf::null_node *node, int lvl) {
  // handles a null pointer literal
  openTag(node, lvl);
  closeTag(node, lvl);
}

void udf::xml_writer::do_integer_node(cdk::integer_node *const node, int lvl) {
  // processes an integer literal
  process_literal(node, lvl);
}

void udf::xml_writer::do_double_node(cdk::double_node *const node, int lvl) {
  // processes a double literal
  process_literal(node, lvl);
}

void udf::xml_writer::do_string_node(cdk::string_node *const node, int lvl) {
  // processes a string literal
  process_literal(node, lvl);
}

void udf::xml_writer::do_tensor_node(udf::tensor_node *const node, int lvl) {
  // processes a tensor literal and its content
  openTag(node, lvl);
  cdk::sequence_node *content = node->content();
  if (content) {
    for (size_t i = 0; i < content->size(); ++i) {
      cdk::basic_node *element = content->node(i);
      if (element) {
        element->accept(this, lvl + 2);
      }
    }
  }
  closeTag(node, lvl);
}

//---------------------------------------------------------------------------
// UNARY OPERATIONS
//---------------------------------------------------------------------------

void udf::xml_writer::do_unary_operation(cdk::unary_operation_node *const node,
                                         int lvl) {
  // helper for all unary operations
  openTag(node, lvl);
  node->argument()->accept(this, lvl + 2);
  closeTag(node, lvl);
}

void udf::xml_writer::do_unary_minus_node(cdk::unary_minus_node *const node,
                                          int lvl) {
  do_unary_operation(node, lvl);
}

void udf::xml_writer::do_unary_plus_node(cdk::unary_plus_node *const node,
                                         int lvl) {
  do_unary_operation(node, lvl);
}

void udf::xml_writer::do_not_node(cdk::not_node *const node, int lvl) {
  do_unary_operation(node, lvl);
}

void udf::xml_writer::do_unary_position_node(
    udf::unary_position_node *const node, int lvl) {
  // handles the address-of operator
  node->argument()->accept(this, lvl + 2);
  // todo do_unary_operation(node, lvl);
}

void udf::xml_writer::do_size_of_node(udf::size_of_node *node, int lvl) {
  // handles the sizeof operator
  openTag(node, lvl);
  node->argument()->accept(this, lvl + 2);
  closeTag(node, lvl);
}

//---------------------------------------------------------------------------
// BINARY OPERATIONS
//---------------------------------------------------------------------------

void udf::xml_writer::do_binary_operation(
    cdk::binary_operation_node *const node, int lvl) {
  // helper for all binary operations
  openTag(node, lvl);
  node->left()->accept(this, lvl + 2);
  node->right()->accept(this, lvl + 2);
  closeTag(node, lvl);
}

void udf::xml_writer::do_and_node(cdk::and_node *const node, int lvl) {
  do_binary_operation(node, lvl);
}

void udf::xml_writer::do_or_node(cdk::or_node *const node, int lvl) {
  do_binary_operation(node, lvl);
}

void udf::xml_writer::do_add_node(cdk::add_node *const node, int lvl) {
  do_binary_operation(node, lvl);
}

void udf::xml_writer::do_sub_node(cdk::sub_node *const node, int lvl) {
  do_binary_operation(node, lvl);
}

void udf::xml_writer::do_mul_node(cdk::mul_node *const node, int lvl) {
  do_binary_operation(node, lvl);
}

void udf::xml_writer::do_div_node(cdk::div_node *const node, int lvl) {
  do_binary_operation(node, lvl);
}

void udf::xml_writer::do_mod_node(cdk::mod_node *const node, int lvl) {
  do_binary_operation(node, lvl);
}

void udf::xml_writer::do_lt_node(cdk::lt_node *const node, int lvl) {
  do_binary_operation(node, lvl);
}

void udf::xml_writer::do_le_node(cdk::le_node *const node, int lvl) {
  do_binary_operation(node, lvl);
}

void udf::xml_writer::do_ge_node(cdk::ge_node *const node, int lvl) {
  do_binary_operation(node, lvl);
}

void udf::xml_writer::do_gt_node(cdk::gt_node *const node, int lvl) {
  do_binary_operation(node, lvl);
}

void udf::xml_writer::do_eq_node(cdk::eq_node *const node, int lvl) {
  do_binary_operation(node, lvl);
}

void udf::xml_writer::do_ne_node(cdk::ne_node *const node, int lvl) {
  do_binary_operation(node, lvl);
}

//---------------------------------------------------------------------------
// VARIABLES AND ASSIGNMENTS
//---------------------------------------------------------------------------

void udf::xml_writer::do_variable_node(cdk::variable_node *const node,
                                       int lvl) {
  // handles a variable usage
  single_line_tag(os(), node->label(), node->name(), lvl);
}

void udf::xml_writer::do_rvalue_node(cdk::rvalue_node *const node, int lvl) {
  // handles reading a value from an l-value
  openTag(node, lvl);
  node->lvalue()->accept(this, lvl + 2);
  closeTag(node, lvl);
}

void udf::xml_writer::do_assignment_node(cdk::assignment_node *const node,
                                         int lvl) {
  // handles an assignment operation
  openTag(node, lvl);
  node->lvalue()->accept(this, lvl + 2);
  reset_new_symbol();
  node->rvalue()->accept(this, lvl + 2);
  closeTag(node, lvl);
}

void udf::xml_writer::do_auto_node(udf::auto_node *const node, int lvl) {
  // handles an 'auto' type definition
  openTag(node, lvl);
  node->value()->accept(this, lvl + 2);
  closeTag(node, lvl);
}

void udf::xml_writer::do_var_definition_node(
    udf::var_definition_node *const node, int lvl) {
  // handles a variable declaration/definition
  openTag(node, lvl);
  single_line_tag(os(), "access_specifier",
                  std::to_string(node->access_specifier()), lvl + 2);
  single_line_tag(os(), "identifier", node->name(), lvl + 2);
  openTag("initializer", lvl + 2);
  if (node->value()) {
    node->value()->accept(this, lvl + 4);
  }
  closeTag("initializer", lvl + 2);
  closeTag(node, lvl);
}

//---------------------------------------------------------------------------
// CONTROL FLOW NODES
//---------------------------------------------------------------------------

void udf::xml_writer::do_if_node(udf::if_node *const node, int lvl) {
  // processes an if statement
  openTag(node, lvl);
  openTag("condition", lvl + 2);
  node->condition()->accept(this, lvl + 4);
  closeTag("condition", lvl + 2);
  openTag("then", lvl + 2);
  node->block()->accept(this, lvl + 4);
  closeTag("then", lvl + 2);
  closeTag(node, lvl);
}

void udf::xml_writer::do_if_else_node(udf::if_else_node *const node, int lvl) {
  // processes an if-else statement
  openTag(node, lvl);
  openTag("condition", lvl + 2);
  node->condition()->accept(this, lvl + 4);
  closeTag("condition", lvl + 2);
  openTag("then", lvl + 2);
  node->thenblock()->accept(this, lvl + 4);
  closeTag("then", lvl + 2);
  openTag("else", lvl + 2);
  node->elseblock()->accept(this, lvl + 4);
  closeTag("else", lvl + 2);
  closeTag(node, lvl);
}

void udf::xml_writer::do_for_node(udf::for_node *const node, int lvl) {
  // processes a for loop
  openTag(node, lvl);
  openTag("initialization", lvl + 2);
  if (node->init()) {
    node->init()->accept(this, lvl + 4);
  }
  closeTag("initialization", lvl + 2);
  openTag("condition", lvl + 2);
  if (node->condition()) {
    node->condition()->accept(this, lvl + 4);
  }
  closeTag("condition", lvl + 2);
  openTag("update", lvl + 2);
  if (node->update()) {
    node->update()->accept(this, lvl + 4);
  }
  closeTag("update", lvl + 2);
  openTag("body", lvl + 2);
  if (node->block()) {
    node->block()->accept(this, lvl + 4);
  }
  closeTag("body", lvl + 2);
  closeTag(node, lvl);
}

void udf::xml_writer::do_break_node(udf::break_node *const node, int lvl) {
  // handles a break statement
  openTag(node, lvl);
  closeTag(node, lvl);
}

void udf::xml_writer::do_continue_node(udf::continue_node *const node,
                                       int lvl) {
  // handles a continue statement
  openTag(node, lvl);
  closeTag(node, lvl);
}

void udf::xml_writer::do_return_node(udf::return_node *const node, int lvl) {
  // handles a return statement
  openTag(node, lvl);
  if (node->return_value()) {
    node->return_value()->accept(this, lvl + 2);
  }
  closeTag(node, lvl);
}

//---------------------------------------------------------------------------
// PROGRAM STRUCTURE NODES
//---------------------------------------------------------------------------

void udf::xml_writer::do_program_node(udf::program_node *const node, int lvl) {
  // handles the root of the ast
  openTag(node, lvl);
  node->statements()->accept(this, lvl + 2);
  closeTag(node, lvl);
}

void udf::xml_writer::do_code_block_node(udf::code_block_node *const node,
                                         int lvl) {
  // handles a block of code with declarations and statements
  openTag(node, lvl);
  openTag("declarations", lvl + 2);
  if (node->declarations()) {
    node->declarations()->accept(this, lvl + 4);
  }
  closeTag("declarations", lvl + 2);
  openTag("statements", lvl + 2);
  if (node->statements()) {
    node->statements()->accept(this, lvl + 4);
  }
  closeTag("statements", lvl + 2);
  closeTag(node, lvl);
}

void udf::xml_writer::do_evaluation_node(udf::evaluation_node *const node,
                                         int lvl) {
  // handles an expression used as a statement
  openTag(node, lvl);
  node->argument()->accept(this, lvl + 2);
  closeTag(node, lvl);
}

//---------------------------------------------------------------------------
// FUNCTION-RELATED NODES
//---------------------------------------------------------------------------

void udf::xml_writer::do_function_node(udf::function_node *const node,
                                       int lvl) {
  // handles a function definition
  openTag(node, lvl);
  single_line_tag(os(), "access_specifier",
                  std::to_string(node->access_specifier()), lvl + 2);
  single_line_tag(os(), "identifier", node->identifier(), lvl + 2);
  openTag("arguments", lvl + 2);
  if (node->arguments()) {
    node->arguments()->accept(this, lvl + 4);
  }
  closeTag("arguments", lvl + 2);
  openTag("body", lvl + 2);
  if (node->body()) {
    node->body()->accept(this, lvl + 4);
  }
  closeTag("body", lvl + 2);
  closeTag(node, lvl);
}

void udf::xml_writer::do_function_call_node(
    udf::function_call_node *const node, int lvl) {
  // handles a function call
  openTag(node, lvl);
  single_line_tag(os(), "name", node->name(), lvl + 2);
  openTag("arguments", lvl + 2);
  if (node->arguments()) {
    node->arguments()->accept(this, lvl + 4);
  }
  closeTag("arguments", lvl + 2);
  closeTag(node, lvl);
}

//---------------------------------------------------------------------------
// MEMORY OPERATIONS
//---------------------------------------------------------------------------

void udf::xml_writer::do_memory_allocation_node(
    udf::memory_allocation_node *const node, int lvl) {
  // handles dynamic memory allocation
  openTag(node, lvl);
  node->size()->accept(this, lvl + 2);
  closeTag(node, lvl);
}

void udf::xml_writer::do_element_access_node(
    udf::element_access_node *const node, int lvl) {
  // handles array/pointer element access
  openTag(node, lvl);
  openTag("base", lvl + 2);
  node->base()->accept(this, lvl + 4);
  closeTag("base", lvl + 2);
  openTag("index", lvl + 2);
  node->index()->accept(this, lvl + 4);
  closeTag("index", lvl + 2);
  closeTag(node, lvl);
}

//---------------------------------------------------------------------------
// INPUT/OUTPUT OPERATIONS
//---------------------------------------------------------------------------

void udf::xml_writer::do_input_node(udf::input_node *const node, int lvl) {
  // handles reading input
  openTag(node, lvl);
  closeTag(node, lvl);
}

void udf::xml_writer::do_write_node(udf::write_node *const node, int lvl) {
  // handles printing output
  openTag(node, lvl);
  single_line_tag(os(), "newline", node->new_line() ? "true" : "false",
                  lvl + 2);
  openTag("arguments", lvl + 2);
  node->arguments()->accept(this, lvl + 4);
  closeTag("arguments", lvl + 2);
  closeTag(node, lvl);
}

//---------------------------------------------------------------------------
// TENSOR-SPECIFIC OPERATIONS
//---------------------------------------------------------------------------

void udf::xml_writer::do_tensor_contract_node(
    udf::tensor_contract_node *const node, int lvl) {
  // handles tensor contraction (matrix multiplication)
  do_binary_operation(node, lvl);
}

void udf::xml_writer::do_tensor_dim_node(udf::tensor_dim_node *const node,
                                         int lvl) {
  // handles the 'dim' or 'dims' property access
  openTag(node, lvl);
  node->operand()->accept(this, lvl + 2);
  closeTag(node, lvl);
}

void udf::xml_writer::do_tensor_rank_node(udf::tensor_rank_node *const node,
                                          int lvl) {
  // handles the 'rank' property access
  openTag(node, lvl);
  node->operand()->accept(this, lvl + 2);
  closeTag(node, lvl);
}

void udf::xml_writer::do_tensor_cap_node(udf::tensor_cap_node *const node,
                                         int lvl) {
  // handles the 'capacity' property access
  openTag(node, lvl);
  node->operand()->accept(this, lvl + 2);
  closeTag(node, lvl);
}

void udf::xml_writer::do_tensor_index_node(udf::tensor_index_node *const node,
                                           int lvl) {
  // handles indexing into a tensor
  openTag(node, lvl);
  openTag("operand", lvl + 2);
  node->operand()->accept(this, lvl + 4);
  closeTag("operand", lvl + 2);
  openTag("indexes", lvl + 2);
  node->index()->accept(this, lvl + 4);
  closeTag("indexes", lvl + 2);
  closeTag(node, lvl);
}

void udf::xml_writer::do_tensor_reshape_node(
    udf::tensor_reshape_node *const node, int lvl) {
  // handles reshaping a tensor
  openTag(node, lvl);
  openTag("operand", lvl + 2);
  node->operand()->accept(this, lvl + 4);
  closeTag("operand", lvl + 2);
  openTag("dimensions", lvl + 2);
  node->dims()->accept(this, lvl + 4);
  closeTag("dimensions", lvl + 2);
  closeTag(node, lvl);
}
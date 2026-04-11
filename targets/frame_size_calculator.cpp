#include "targets/frame_size_calculator.h"
#include ".auto/all_nodes.h"

udf::frame_size_calculator::~frame_size_calculator() { os().flush(); }

//===========================================================================
// SIMPLE NODES
//===========================================================================

void udf::frame_size_calculator::do_data_node(cdk::data_node *const node,
                                              int lvl) {}
void udf::frame_size_calculator::do_nil_node(cdk::nil_node *const node,
                                             int lvl) {}
void udf::frame_size_calculator::do_unary_position_node(
    udf::unary_position_node *const node, int lvl) {}
void udf::frame_size_calculator::do_add_node(cdk::add_node *const node,
                                             int lvl) {}
void udf::frame_size_calculator::do_and_node(cdk::and_node *const node,
                                             int lvl) {}
void udf::frame_size_calculator::do_assignment_node(
    cdk::assignment_node *const node, int lvl) {}
void udf::frame_size_calculator::do_div_node(cdk::div_node *const node,
                                             int lvl) {}
void udf::frame_size_calculator::do_double_node(cdk::double_node *const node,
                                                int lvl) {}
void udf::frame_size_calculator::do_eq_node(cdk::eq_node *const node,
                                            int lvl) {}
void udf::frame_size_calculator::do_ge_node(cdk::ge_node *const node,
                                            int lvl) {}
void udf::frame_size_calculator::do_gt_node(cdk::gt_node *const node,
                                            int lvl) {}
void udf::frame_size_calculator::do_variable_node(
    cdk::variable_node *const node, int lvl) {}
void udf::frame_size_calculator::do_integer_node(cdk::integer_node *const node,
                                                 int lvl) {}
void udf::frame_size_calculator::do_le_node(cdk::le_node *const node,
                                            int lvl) {}
void udf::frame_size_calculator::do_lt_node(cdk::lt_node *const node,
                                            int lvl) {}
void udf::frame_size_calculator::do_mod_node(cdk::mod_node *const node,
                                             int lvl) {}
void udf::frame_size_calculator::do_mul_node(cdk::mul_node *const node,
                                             int lvl) {}
void udf::frame_size_calculator::do_ne_node(cdk::ne_node *const node,
                                            int lvl) {}
void udf::frame_size_calculator::do_unary_minus_node(
    cdk::unary_minus_node *const node, int lvl) {}
void udf::frame_size_calculator::do_unary_plus_node(
    cdk::unary_plus_node *const node, int lvl) {}
void udf::frame_size_calculator::do_not_node(cdk::not_node *const node,
                                             int lvl) {}
void udf::frame_size_calculator::do_or_node(cdk::or_node *const node,
                                            int lvl) {}
void udf::frame_size_calculator::do_rvalue_node(cdk::rvalue_node *const node,
                                                int lvl) {}
void udf::frame_size_calculator::do_string_node(cdk::string_node *const node,
                                                int lvl) {}
void udf::frame_size_calculator::do_sub_node(cdk::sub_node *const node,
                                             int lvl) {}
void udf::frame_size_calculator::do_write_node(udf::write_node *const node,
                                               int lvl) {}
void udf::frame_size_calculator::do_input_node(udf::input_node *const node,
                                               int lvl) {}
void udf::frame_size_calculator::do_function_call_node(
    udf::function_call_node *const node, int lvl) {}
void udf::frame_size_calculator::do_element_access_node(
    udf::element_access_node *const node, int lvl) {}
void udf::frame_size_calculator::do_null_node(udf::null_node *const node,
                                              int lvl) {}
void udf::frame_size_calculator::do_memory_allocation_node(
    udf::memory_allocation_node *const node, int lvl) {}
void udf::frame_size_calculator::do_break_node(udf::break_node *const node,
                                               int lvl) {}
void udf::frame_size_calculator::do_continue_node(
    udf::continue_node *const node, int lvl) {}
void udf::frame_size_calculator::do_return_node(udf::return_node *const node,
                                                int lvl) {}
void udf::frame_size_calculator::do_auto_node(udf::auto_node *const node,
                                              int lvl) {}
void udf::frame_size_calculator::do_size_of_node(udf::size_of_node *const node,
                                                 int lvl) {}
void udf::frame_size_calculator::do_tensor_node(udf::tensor_node *const node,
                                                int lvl) {}
void udf::frame_size_calculator::do_tensor_index_node(
    udf::tensor_index_node *const node, int lvl) {}
void udf::frame_size_calculator::do_tensor_reshape_node(
    udf::tensor_reshape_node *const node, int lvl) {}
void udf::frame_size_calculator::do_tensor_contract_node(
    udf::tensor_contract_node *const node, int lvl) {}
void udf::frame_size_calculator::do_tensor_rank_node(
    udf::tensor_rank_node *const node, int lvl) {}
void udf::frame_size_calculator::do_tensor_dim_node(
    udf::tensor_dim_node *const node, int lvl) {}
void udf::frame_size_calculator::do_tensor_cap_node(
    udf::tensor_cap_node *const node, int lvl) {}

void udf::frame_size_calculator::do_evaluation_node(
    udf::evaluation_node *const node, int lvl) {
  // Must visit the argument, as it may contain declarations.
  node->argument()->accept(this, lvl + 2);
}

//===========================================================================
// COMPOSITE / STRUCTURAL NODES
//===========================================================================

void udf::frame_size_calculator::do_sequence_node(
    cdk::sequence_node *const node, int lvl) {
  for (size_t i = 0; i < node->size(); i++) {
    cdk::basic_node *n = node->node(i);
    if (n == nullptr) break;
    n->accept(this, lvl + 2);
  }
}

void udf::frame_size_calculator::do_code_block_node(
    udf::code_block_node *const node, int lvl) {
  if (node->declarations())
    node->declarations()->accept(this, lvl + 2);
  if (node->statements())
    node->statements()->accept(this, lvl + 2);
}

void udf::frame_size_calculator::do_for_node(udf::for_node *const node,
                                             int lvl) {
  if (node->init())
    node->init()->accept(this, lvl + 2);
  node->block()->accept(this, lvl + 2);
}

void udf::frame_size_calculator::do_if_node(udf::if_node *const node,
                                            int lvl) {
  node->block()->accept(this, lvl + 2);
}

void udf::frame_size_calculator::do_if_else_node(
    udf::if_else_node *const node, int lvl) {
  node->thenblock()->accept(this, lvl + 2);
  if (node->elseblock())
    node->elseblock()->accept(this, lvl + 2);
}

//===========================================================================
// THE CORE NODE (VARIABLE DEFINITION)
//===========================================================================

void udf::frame_size_calculator::do_var_definition_node(
    udf::var_definition_node *const node, int lvl) {
  // Add the new local variable's size to the total.
  _localsize += node->type()->size();
}

//===========================================================================
// TOP-LEVEL SCOPE NODES
//===========================================================================

void udf::frame_size_calculator::do_function_node(
    udf::function_node *const node, int lvl) {
  if (node->body())
    node->body()->accept(this, lvl + 2);
}

void udf::frame_size_calculator::do_program_node(
    udf::program_node *const node, int lvl) {
  if (node->statements())
    node->statements()->accept(this, lvl + 2);
}
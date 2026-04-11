#include "targets/postfix_writer.h"
#include ".auto/all_nodes.h"
#include "targets/frame_size_calculator.h"
#include "targets/type_checker.h"
#include <cdk/types/types.h>
#include <functional>
#include <sstream>

#include "udf_parser.tab.h"

//---------------------------------------------------------------------------
// BASIC TRAVERSAL
//---------------------------------------------------------------------------
void udf::postfix_writer::do_data_node(cdk::data_node *const node, int lvl) {}
void udf::postfix_writer::do_nil_node(cdk::nil_node *const node, int lvl) {}
void udf::postfix_writer::do_unary_position_node(
    udf::unary_position_node *const node, int lvl) {
  ASSERT_SAFE;
  node->argument()->accept(this, lvl);
}

//---------------------------------------------------------------------------
// GENERAL AND STRUCTURAL NODES
//---------------------------------------------------------------------------
void udf::postfix_writer::do_sequence_node(cdk::sequence_node *const node,
                                           int lvl) {
  for (size_t i = 0; i < node->size(); i++) {
    node->node(i)->accept(this, lvl);
  }
}

void udf::postfix_writer::do_program_node(udf::program_node *const node,
                                          int lvl) {
  ASSERT_SAFE;

  auto statements = dynamic_cast<cdk::sequence_node *>(node->statements());

  // Emit a valid empty entry point even when the program has no declarations.
  if (statements == nullptr) {
    _pf.TEXT();
    _pf.ALIGN();
    _pf.GLOBAL("_main", _pf.FUNC());
    _pf.LABEL("_main");
    _pf.ENTER(0);
    _pf.INT(0);
    _pf.STFVAL32();
    _pf.LEAVE();
    _pf.RET();
    return;
  }

  // Emit all top-level declarations and function definitions first.
  for (size_t i = 0; i < statements->size(); i++) {
    auto decl = statements->node(i);
    decl->accept(this, lvl);
  }

  // Emit the synthetic runtime entry point.
  _pf.TEXT();
  _pf.ALIGN();
  _pf.GLOBAL("_main", _pf.FUNC());
  _pf.LABEL("_main");
  _pf.ENTER(0); // The synthetic main frame does not allocate locals.

  // Initialize runtime memory support before invoking the user entry point.
  _pendingFunctionDeclarations.insert("mem_init");
  _pf.CALL("mem_init");

  // Call the user-defined `udf` function, which acts as the program entry point.
  auto udf_symbol = _symtab.find("udf");
  if (udf_symbol == nullptr || !udf_symbol->isFunction()) {
    error(0, "FATAL: function 'udf' not defined.");
    _pf.INT(-1); // Return a failure code if the user entry point is missing.
    _pf.STFVAL32();
    _pf.LEAVE();
    _pf.RET();
  } else {
    _pf.CALL("udf");
    _pf.LDFVAL32(); // Load the return value produced by `udf`.
    _pf.STFVAL32(); // Expose it as the return value of `_main`.
    _pf.LEAVE();
    _pf.RET();
  }

  // Declare every external symbol referenced during code generation.
  for (const std::string &s : _pendingFunctionDeclarations)
    _pf.EXTERN(s);
}

void udf::postfix_writer::do_code_block_node(udf::code_block_node *const node,
                                             int lvl) {
  _symtab.push(); // Enter a new lexical scope.
  if (node->declarations())
    node->declarations()->accept(this, lvl + 2);
  if (node->statements())
    node->statements()->accept(this, lvl + 2);
  _symtab.pop(); // Leave the lexical scope.
}

//---------------------------------------------------------------------------
// CONTROL FLOW
//---------------------------------------------------------------------------
void udf::postfix_writer::do_return_node(udf::return_node *const node,
                                         int lvl) {
  ASSERT_SAFE;
  _returnAcknowledged = true;

  if (_functions.empty()) {
    error(node->lineno(), "'return' outside function");
    return;
  }

  auto function = _functions.top();
  auto ftype = cdk::functional_type::cast(function->type());
  if (ftype == nullptr) {
    error(node->lineno(), "internal compiler error: function symbol is not "
                         "of functional type.");
    return;
  }

  auto return_type = ftype->output(0);

  // Non-void functions must materialize a value in the proper return register.
  if (return_type->name() != cdk::TYPE_VOID) {
    if (node->return_value() == nullptr) {
      error(node->lineno(), "non-void function must return a value.");
      return;
    }
    node->return_value()->accept(this, lvl + 2);

    // Store the value in the ABI-appropriate return register.
    if (return_type->name() == cdk::TYPE_INT ||
        return_type->name() == cdk::TYPE_STRING ||
        return_type->name() == cdk::TYPE_POINTER ||
        return_type->name() == cdk::TYPE_FUNCTIONAL ||
        return_type->name() == cdk::TYPE_TENSOR) {
      _pf.STFVAL32();
    } else if (return_type->name() == cdk::TYPE_DOUBLE) {
      if (node->return_value()->is_typed(cdk::TYPE_INT))
        _pf.I2D(); // Promote integer results when returning a double.
      _pf.STFVAL64();
    }
  }

  // Transfer control to the common function epilogue.
  _pf.JMP(function->name() + "_epilogue");
}

void udf::postfix_writer::do_break_node(udf::break_node *const node,
                                        int lvl) {
  if (!_forEnd.empty()) {
    _pf.JMP(mklbl(_forEnd.back())); // Jump to the end of the active loop.
  } else {
    error(node->lineno(), "'break' outside 'for'");
  }
}

void udf::postfix_writer::do_continue_node(udf::continue_node *const node,
                                           int lvl) {
  if (!_forUpdate.empty()) {
    // Jump directly to the update section of the active loop.
    _pf.JMP(mklbl(_forUpdate.back()));
  } else {
    error(node->lineno(), "'continue' outside 'for'");
  }
}

void udf::postfix_writer::do_for_node(udf::for_node *const node, int lvl) {
  ASSERT_SAFE;
  int condLbl = ++_lbl;
  int updateLbl = ++_lbl;
  int endLbl = ++_lbl;

  _forUpdate.push_back(updateLbl);
  _forEnd.push_back(endLbl);

  if (node->init()) {
    node->init()->accept(this, lvl + 2);
  }

  // Condition check.
  _pf.ALIGN();
  _pf.LABEL(mklbl(condLbl));
  if (node->condition() && node->condition()->size() > 0) {
    node->condition()->accept(this, lvl + 2);
    _pf.JZ(mklbl(endLbl)); // Exit the loop when the condition is false.
  }

  // Loop body.
  node->block()->accept(this, lvl + 2);

  // Update section.
  _pf.ALIGN();
  _pf.LABEL(mklbl(updateLbl));
  if (node->update()) {
    node->update()->accept(this, lvl + 2);
  }

  _pf.JMP(mklbl(condLbl)); // Repeat from the condition check.

  // Loop exit label.
  _pf.ALIGN();
  _pf.LABEL(mklbl(endLbl));

  _forUpdate.pop_back();
  _forEnd.pop_back();
}

void udf::postfix_writer::do_if_node(udf::if_node *const node, int lvl) {
  ASSERT_SAFE;
  int lbl_end;
  node->condition()->accept(this, lvl);
  _pf.JZ(mklbl(lbl_end = ++_lbl)); // Skip the block when the condition is false.
  node->block()->accept(this, lvl + 2);
  _pf.LABEL(mklbl(lbl_end));
}

void udf::postfix_writer::do_if_else_node(udf::if_else_node *const node,
                                          int lvl) {
  ASSERT_SAFE;
  int lbl_else, lbl_end;
  node->condition()->accept(this, lvl);
  _pf.JZ(mklbl(lbl_else = ++_lbl)); // Branch to the else block when needed.
  node->thenblock()->accept(this, lvl + 2);
  _pf.JMP(mklbl(lbl_end = ++_lbl)); // Skip the else block after the then branch.
  _pf.LABEL(mklbl(lbl_else));
  node->elseblock()->accept(this, lvl + 2);
  _pf.LABEL(mklbl(lbl_end));
}

//---------------------------------------------------------------------------
// VARIABLE DECLARATIONS, LITERALS, AND MEMORY
//---------------------------------------------------------------------------
void udf::postfix_writer::do_var_definition_node(
    udf::var_definition_node *const node, int lvl) {
  ASSERT_SAFE;

  auto id = node->name();
  int offset = 0, typesize = node->type()->size();
  auto symbol = _symtab.find(id);

  // Reserve stack offsets for formal parameters.
  if (_context.top() == Context::Args) {
    offset = _offset;
    _offset += typesize;
    if (symbol) {
      symbol->set_offset(offset);
    }
    return;
  }

  // Reserve storage for local variables.
  if (_context.top() == Context::Body) {
    _offset -= typesize;
    offset = _offset;
  } else {
    offset = 0; // Global symbols do not use frame-relative offsets.
  }

  if (symbol) {
    symbol->set_offset(offset);
  }

  if (_context.top() == Context::Body) {
    // Materialize the initializer directly into local storage.
    if (node->value()) {
      node->value()->accept(this, lvl);
      if (node->is_typed(cdk::TYPE_INT) ||
          node->is_typed(cdk::TYPE_STRING) ||
          node->is_typed(cdk::TYPE_POINTER) ||
          node->is_typed(cdk::TYPE_FUNCTIONAL) ||
          node->is_typed(cdk::TYPE_TENSOR)) {
        _pf.LOCAL(symbol->offset());
        _pf.STINT();
      } else if (node->is_typed(cdk::TYPE_DOUBLE)) {
        if (node->value()->is_typed(cdk::TYPE_INT))
          _pf.I2D();
        _pf.LOCAL(symbol->offset());
        _pf.STDOUBLE();
      } else {
        error(node->lineno(), "cannot initialize local variable");
      }
    } else {
      // Uninitialized local tensors still require runtime allocation.
      if (node->is_typed(cdk::TYPE_TENSOR)) {
        auto ttype = cdk::tensor_type::cast(node->type());
        auto dims = ttype->dims();

        for (int i = dims.size() - 1; i >= 0; --i) {
          _pf.INT(dims[i]);
        }
        _pf.INT(dims.size());
        _pendingFunctionDeclarations.insert("tensor_create");
        _pf.CALL("tensor_create");
        _pf.TRASH(4 * (dims.size() + 1));
        _pf.LDFVAL32();
        _pf.LOCAL(symbol->offset());
        _pf.STINT();
      }
    }
  } else { // Emit static storage for global variables.
    if (node->value() == nullptr) {
      // Emit zero-initialized storage in the BSS segment.
      _pf.BSS();
      _pf.ALIGN();
      _pf.LABEL(id);
      _pf.SALLOC(typesize);
    } else {
      // Emit initialized storage in the data segment.
      _pf.DATA();
      _pf.ALIGN();
      _pf.LABEL(id);

      if (node->is_typed(cdk::TYPE_DOUBLE) &&
          node->value()->is_typed(cdk::TYPE_INT)) {
        auto integer_literal =
            dynamic_cast<cdk::integer_node *>(node->value());
        if (integer_literal) {
          _pf.SDOUBLE(static_cast<double>(integer_literal->value()));
        } else {
          error(node->lineno(),
                "complex integer expression for global double "
                "initializer not supported");
        }
      } else if (node->is_typed(cdk::TYPE_INT) ||
                 node->is_typed(cdk::TYPE_DOUBLE) ||
                 node->is_typed(cdk::TYPE_POINTER) ||
                 node->is_typed(cdk::TYPE_TENSOR)) {
        node->value()->accept(this, lvl);
      } else if (node->is_typed(cdk::TYPE_STRING)) {
        node->value()->accept(this, lvl);
      } else {
        error(node->lineno(), "cannot initialize global variable");
      }
    }
  }
}

void udf::postfix_writer::do_auto_node(udf::auto_node *const node, int lvl) {
  node->value()->accept(this, lvl);
}

void udf::postfix_writer::do_double_node(cdk::double_node *const node,
                                         int lvl) {
  if (_context.top() == Context::Body) {
    _pf.DOUBLE(node->value()); // Push the literal onto the evaluation stack.
  } else {
    _pf.SDOUBLE(node->value()); // Emit the literal in static data.
  }
}

void udf::postfix_writer::do_integer_node(cdk::integer_node *const node,
                                          int lvl) {
  if (_context.top() == Context::Body) {
    _pf.INT(node->value()); // Push the literal onto the evaluation stack.
  } else {
    _pf.SINT(node->value()); // Emit the literal in static data.
  }
}

void udf::postfix_writer::do_string_node(cdk::string_node *const node,
                                         int lvl) {
  int lbl = ++_lbl;
  // Emit the string literal in read-only storage.
  _pf.RODATA();
  _pf.ALIGN();
  _pf.LABEL(mklbl(lbl));
  _pf.SSTRING(node->value());

  if (_context.top() == Context::Body) {
    _pf.TEXT();
    _pf.ADDR(mklbl(lbl)); // Push the literal address onto the stack.
  } else {
    _pf.DATA();
    _pf.SADDR(mklbl(lbl)); // Emit a static pointer to the literal.
  }
}

void udf::postfix_writer::do_null_node(udf::null_node *const node, int lvl) {
  ASSERT_SAFE;
  if (_context.top() == Context::Body) {
    _pf.INT(0); // Push the null pointer representation.
  } else {
    _pf.SINT(0); // Emit the null pointer representation in static data.
  }
}

void udf::postfix_writer::do_variable_node(cdk::variable_node *const node,
                                           int lvl) {
  ASSERT_SAFE;
  auto symbol = _symtab.find(node->name());
  if (symbol->global()) {
    _pf.ADDR(symbol->name()); // Load the address of the global symbol.
  } else {
    _pf.LOCAL(symbol->offset()); // Load the address of the local slot.
  }
}

void udf::postfix_writer::do_element_access_node(
    udf::element_access_node *const node, int lvl) {
  ASSERT_SAFE;
  node->base()->accept(this, lvl);  // Push the base address.
  node->index()->accept(this, lvl); // Push the element index.
  auto reftype = cdk::reference_type::cast(node->base()->type());
  _pf.INT(reftype->referenced()->size()); // Push the size of each element.
  _pf.MUL();                              // Compute the byte offset.
  _pf.ADD();                              // Add the offset to the base address.
}

void udf::postfix_writer::do_rvalue_node(cdk::rvalue_node *const node,
                                         int lvl) {
  ASSERT_SAFE;
  node->lvalue()->accept(this, lvl); // Resolve the address of the lvalue.
  if (node->type()->name() == cdk::TYPE_DOUBLE) {
    _pf.LDDOUBLE(); // Load the 64-bit value.
  } else {
    _pf.LDINT(); // Load the 32-bit value.
  }
}

void udf::postfix_writer::do_assignment_node(cdk::assignment_node *const node,
                                             int lvl) {
  ASSERT_SAFE;
  node->rvalue()->accept(this, lvl + 2); // Evaluate the assigned value.
  if (node->type()->name() == cdk::TYPE_DOUBLE &&
      node->rvalue()->is_typed(cdk::TYPE_INT)) {
    _pf.I2D(); // Promote integer results when assigning to doubles.
  }

  // Preserve the assigned value so the assignment itself yields that value.
  if (node->type()->name() == cdk::TYPE_DOUBLE) {
    _pf.DUP64();
  } else {
    _pf.DUP32();
  }

  node->lvalue()->accept(this, lvl); // Resolve the destination address.
  if (node->type()->name() == cdk::TYPE_DOUBLE) {
    _pf.STDOUBLE(); // Store the 64-bit value.
  } else {
    _pf.STINT(); // Store the 32-bit value.
  }
}

void udf::postfix_writer::do_size_of_node(udf::size_of_node *const node,
                                          int lvl) {
  ASSERT_SAFE;
  if (node->argument()->is_typed(cdk::TYPE_TENSOR)) {
    auto ttype = cdk::tensor_type::cast(node->argument()->type());
    // Tensor payloads are stored as doubles in the runtime representation.
    _pf.INT(ttype->size() * 8);
  } else {
    _pf.INT(node->argument()->type()->size());
  }
}

void udf::postfix_writer::do_memory_allocation_node(
    udf::memory_allocation_node *const node, int lvl) {
  ASSERT_SAFE;
  node->size()->accept(this, lvl); // Push the requested element count.
  auto reftype = cdk::reference_type::cast(node->type());
  _pf.INT(reftype->referenced()->size()); // Push the size of one element.
  _pf.MUL();                              // Compute the total allocation size.
  _pendingFunctionDeclarations.insert("mem_alloc");
  _pf.CALL("mem_alloc"); // Request heap storage from the runtime.
  _pf.TRASH(4);
  _pf.LDFVAL32(); // Load the returned pointer.
}

//---------------------------------------------------------------------------
// I/O OPERATIONS
//---------------------------------------------------------------------------
void udf::postfix_writer::do_input_node(udf::input_node *const node,
                                        int lvl) {
  ASSERT_SAFE;
  if (node->is_typed(cdk::TYPE_DOUBLE)) {
    _pendingFunctionDeclarations.insert("readd");
    _pf.CALL("readd");
    _pf.LDFVAL64();
  } else if (node->is_typed(cdk::TYPE_INT)) {
    _pendingFunctionDeclarations.insert("readi");
    _pf.CALL("readi");
    _pf.LDFVAL32();
  } else {
    error(node->lineno(), "cannot read type");
  }
}

void udf::postfix_writer::do_write_node(udf::write_node *const node,
                                        int lvl) {
  ASSERT_SAFE;
  for (size_t ix = 0; ix < node->arguments()->size(); ix++) {
    auto child = (cdk::expression_node *)node->arguments()->node(ix);
    child->accept(this, lvl);
    if (child->is_typed(cdk::TYPE_INT)) {
      _pendingFunctionDeclarations.insert("printi");
      _pf.CALL("printi");
      _pf.TRASH(4);
    } else if (child->is_typed(cdk::TYPE_DOUBLE)) {
      _pendingFunctionDeclarations.insert("printd");
      _pf.CALL("printd");
      _pf.TRASH(8);
    } else if (child->is_typed(cdk::TYPE_STRING)) {
      _pendingFunctionDeclarations.insert("prints");
      _pf.CALL("prints");
      _pf.TRASH(4);
    } else if (child->is_typed(cdk::TYPE_TENSOR)) {
      _pendingFunctionDeclarations.insert("tensor_print");
      _pf.CALL("tensor_print");
      _pf.TRASH(4);
    } else {
      error(node->lineno(), "cannot print expression of unknown type");
    }
  }
  if (node->new_line()) {
    _pendingFunctionDeclarations.insert("println");
    _pf.CALL("println");
  }
}

void udf::postfix_writer::do_evaluation_node(udf::evaluation_node *const node,
                                             int lvl) {
  ASSERT_SAFE;
  node->argument()->accept(this, lvl);
  _pf.TRASH(node->argument()->type()->size()); // Discard the expression value.
}

//---------------------------------------------------------------------------
// UNARY AND BINARY OPERATORS
//---------------------------------------------------------------------------
void udf::postfix_writer::do_unary_minus_node(
    cdk::unary_minus_node *const node, int lvl) {
  ASSERT_SAFE;
  node->argument()->accept(this, lvl);
  if (node->is_typed(cdk::TYPE_DOUBLE))
    _pf.DNEG();
  else
    _pf.NEG();
}

void udf::postfix_writer::do_unary_plus_node(
    cdk::unary_plus_node *const node, int lvl) {
  ASSERT_SAFE;
  node->argument()->accept(this, lvl); // Unary plus preserves the operand.
}

void udf::postfix_writer::do_not_node(cdk::not_node *const node, int lvl) {
  ASSERT_SAFE;
  node->argument()->accept(this, lvl + 2);
  _pf.INT(0);
  _pf.EQ(); // Logical negation is compiled as `x == 0`.
}

void udf::postfix_writer::do_add_node(cdk::add_node *const node, int lvl) {
  ASSERT_SAFE;

  // Tensor plus tensor is delegated to the runtime helper.
  if (node->left()->is_typed(cdk::TYPE_TENSOR) &&
      node->right()->is_typed(cdk::TYPE_TENSOR)) {
    node->right()->accept(this, lvl + 2);
    node->left()->accept(this, lvl + 2);
    _pendingFunctionDeclarations.insert("tensor_add");
    _pf.CALL("tensor_add");
    _pf.TRASH(8);
    _pf.LDFVAL32();
    return;
  }

  // Tensor plus scalar is also handled by the runtime.
  bool left_is_tensor = node->left()->is_typed(cdk::TYPE_TENSOR);
  bool right_is_tensor = node->right()->is_typed(cdk::TYPE_TENSOR);
  bool left_is_scalar = node->left()->is_typed(cdk::TYPE_INT) ||
                        node->left()->is_typed(cdk::TYPE_DOUBLE);
  bool right_is_scalar = node->right()->is_typed(cdk::TYPE_INT) ||
                         node->right()->is_typed(cdk::TYPE_DOUBLE);

  if ((left_is_scalar && right_is_tensor) ||
      (left_is_tensor && right_is_scalar)) {
    cdk::expression_node *scalar_node =
        left_is_scalar ? node->left() : node->right();
    scalar_node->accept(this, lvl + 2);
    if (scalar_node->is_typed(cdk::TYPE_INT)) {
      _pf.I2D();
    }
    cdk::expression_node *tensor_node =
        left_is_tensor ? node->left() : node->right();
    tensor_node->accept(this, lvl + 2);
    _pendingFunctionDeclarations.insert("tensor_add_scalar");
    _pf.CALL("tensor_add_scalar");
    _pf.TRASH(12);
    _pf.LDFVAL32();
    return;
  }

  // Fall back to native arithmetic and pointer offsetting.
  node->left()->accept(this, lvl + 2);
  if (node->is_typed(cdk::TYPE_POINTER) &&
      node->left()->is_typed(cdk::TYPE_INT)) {
    auto reftype = cdk::reference_type::cast(node->right()->type());
    _pf.INT(reftype->referenced()->size());
    _pf.MUL();
  } else if (node->is_typed(cdk::TYPE_DOUBLE) &&
             node->left()->is_typed(cdk::TYPE_INT)) {
    _pf.I2D();
  }

  node->right()->accept(this, lvl + 2);
  if (node->is_typed(cdk::TYPE_POINTER) &&
      node->right()->is_typed(cdk::TYPE_INT)) {
    auto reftype = cdk::reference_type::cast(node->left()->type());
    _pf.INT(reftype->referenced()->size());
    _pf.MUL();
  } else if (node->is_typed(cdk::TYPE_DOUBLE) &&
             node->right()->is_typed(cdk::TYPE_INT)) {
    _pf.I2D();
  }

  if (node->is_typed(cdk::TYPE_DOUBLE))
    _pf.DADD();
  else
    _pf.ADD();
}

void udf::postfix_writer::do_sub_node(cdk::sub_node *const node, int lvl) {
  ASSERT_SAFE;

  // Tensor minus tensor is delegated to the runtime helper.
  if (node->left()->is_typed(cdk::TYPE_TENSOR) &&
      node->right()->is_typed(cdk::TYPE_TENSOR)) {
    node->right()->accept(this, lvl + 2);
    node->left()->accept(this, lvl + 2);
    _pendingFunctionDeclarations.insert("tensor_sub");
    _pf.CALL("tensor_sub");
    _pf.TRASH(8);
    _pf.LDFVAL32();
    return;
  }

  // Tensor minus scalar is delegated to the runtime helper.
  bool left_is_tensor = node->left()->is_typed(cdk::TYPE_TENSOR);
  bool right_is_scalar = node->right()->is_typed(cdk::TYPE_INT) ||
                         node->right()->is_typed(cdk::TYPE_DOUBLE);

  if (left_is_tensor && right_is_scalar) {
    node->right()->accept(this, lvl + 2);
    if (node->right()->is_typed(cdk::TYPE_INT)) {
      _pf.I2D();
    }
    node->left()->accept(this, lvl + 2);
    _pendingFunctionDeclarations.insert("tensor_sub_scalar");
    _pf.CALL("tensor_sub_scalar");
    _pf.TRASH(12);
    _pf.LDFVAL32();
    return;
  }

  // Fall back to native arithmetic and pointer offsetting.
  node->left()->accept(this, lvl + 2);
  if (node->is_typed(cdk::TYPE_DOUBLE) &&
      node->left()->is_typed(cdk::TYPE_INT))
    _pf.I2D();

  node->right()->accept(this, lvl + 2);
  if (node->is_typed(cdk::TYPE_POINTER) &&
      node->right()->is_typed(cdk::TYPE_INT)) {
    auto reftype = cdk::reference_type::cast(node->left()->type());
    _pf.INT(reftype->referenced()->size());
    _pf.MUL();
  } else if (node->is_typed(cdk::TYPE_DOUBLE) &&
             node->right()->is_typed(cdk::TYPE_INT)) {
    _pf.I2D();
  }

  if (node->is_typed(cdk::TYPE_DOUBLE)) {
    _pf.DSUB();
  } else {
    _pf.SUB();
  }

  // Pointer subtraction yields the element distance, not raw bytes.
  if (node->is_typed(cdk::TYPE_INT) &&
      node->left()->is_typed(cdk::TYPE_POINTER) &&
      node->right()->is_typed(cdk::TYPE_POINTER)) {
    auto reftype = cdk::reference_type::cast(node->left()->type());
    _pf.INT(reftype->referenced()->size());
    _pf.DIV();
  }
}

void udf::postfix_writer::do_mul_node(cdk::mul_node *const node, int lvl) {
  ASSERT_SAFE;

  // Tensor multiplication is delegated to the runtime helper.
  if (node->left()->is_typed(cdk::TYPE_TENSOR) &&
      node->right()->is_typed(cdk::TYPE_TENSOR)) {
    node->right()->accept(this, lvl + 2);
    node->left()->accept(this, lvl + 2);
    _pendingFunctionDeclarations.insert("tensor_mul");
    _pf.CALL("tensor_mul");
    _pf.TRASH(8);
    _pf.LDFVAL32();
    return;
  }

  // Tensor-scalar multiplication is delegated to the runtime helper.
  bool left_is_tensor = node->left()->is_typed(cdk::TYPE_TENSOR);
  bool right_is_tensor = node->right()->is_typed(cdk::TYPE_TENSOR);
  bool left_is_scalar = node->left()->is_typed(cdk::TYPE_INT) ||
                        node->left()->is_typed(cdk::TYPE_DOUBLE);
  bool right_is_scalar = node->right()->is_typed(cdk::TYPE_INT) ||
                         node->right()->is_typed(cdk::TYPE_DOUBLE);

  if ((left_is_scalar && right_is_tensor) ||
      (left_is_tensor && right_is_scalar)) {
    cdk::expression_node *scalar_node =
        left_is_scalar ? node->left() : node->right();
    scalar_node->accept(this, lvl + 2);
    if (scalar_node->is_typed(cdk::TYPE_INT)) {
      _pf.I2D();
    }
    cdk::expression_node *tensor_node =
        left_is_tensor ? node->left() : node->right();
    tensor_node->accept(this, lvl + 2);
    _pendingFunctionDeclarations.insert("tensor_mul_scalar");
    _pf.CALL("tensor_mul_scalar");
    _pf.TRASH(12);
    _pf.LDFVAL32();
    return;
  }

  // Fall back to native scalar multiplication.
  node->left()->accept(this, lvl + 2);
  if (node->is_typed(cdk::TYPE_DOUBLE) &&
      node->left()->is_typed(cdk::TYPE_INT))
    _pf.I2D();
  node->right()->accept(this, lvl + 2);
  if (node->is_typed(cdk::TYPE_DOUBLE) &&
      node->right()->is_typed(cdk::TYPE_INT))
    _pf.I2D();
  if (node->is_typed(cdk::TYPE_DOUBLE))
    _pf.DMUL();
  else
    _pf.MUL();
}
void udf::postfix_writer::do_div_node(cdk::div_node *const node, int lvl) {
  ASSERT_SAFE;

  // Tensor division is delegated to the runtime helper.
  if (node->left()->is_typed(cdk::TYPE_TENSOR) &&
      node->right()->is_typed(cdk::TYPE_TENSOR)) {
    node->right()->accept(this, lvl + 2);
    node->left()->accept(this, lvl + 2);
    _pendingFunctionDeclarations.insert("tensor_div");
    _pf.CALL("tensor_div");
    _pf.TRASH(8);
    _pf.LDFVAL32();
    return;
  }

  // Tensor-scalar division is delegated to the runtime helper.
  bool left_is_tensor = node->left()->is_typed(cdk::TYPE_TENSOR);
  bool right_is_scalar = node->right()->is_typed(cdk::TYPE_INT) ||
                         node->right()->is_typed(cdk::TYPE_DOUBLE);

  if (left_is_tensor && right_is_scalar) {
    node->right()->accept(this, lvl + 2);
    if (node->right()->is_typed(cdk::TYPE_INT)) {
      _pf.I2D();
    }
    node->left()->accept(this, lvl + 2);
    _pendingFunctionDeclarations.insert("tensor_div_scalar");
    _pf.CALL("tensor_div_scalar");
    _pf.TRASH(12);
    _pf.LDFVAL32();
    return;
  }

  // Fall back to native scalar division.
  node->left()->accept(this, lvl + 2);
  if (node->is_typed(cdk::TYPE_DOUBLE) &&
      node->left()->is_typed(cdk::TYPE_INT))
    _pf.I2D();
  node->right()->accept(this, lvl + 2);
  if (node->is_typed(cdk::TYPE_DOUBLE) &&
      node->right()->is_typed(cdk::TYPE_INT))
    _pf.I2D();
  if (node->is_typed(cdk::TYPE_DOUBLE))
    _pf.DDIV();
  else
    _pf.DIV();
}

void udf::postfix_writer::do_mod_node(cdk::mod_node *const node, int lvl) {
  ASSERT_SAFE;
  node->left()->accept(this, lvl + 2);
  node->right()->accept(this, lvl + 2);
  _pf.MOD();
}
void udf::postfix_writer::do_lt_node(cdk::lt_node *const node, int lvl) {
  ASSERT_SAFE;
  node->left()->accept(this, lvl + 2);
  if (node->left()->is_typed(cdk::TYPE_INT) &&
      node->right()->is_typed(cdk::TYPE_DOUBLE))
    _pf.I2D();
  node->right()->accept(this, lvl + 2);
  if (node->right()->is_typed(cdk::TYPE_INT) &&
      node->left()->is_typed(cdk::TYPE_DOUBLE))
    _pf.I2D();
  _pf.LT();
}
void udf::postfix_writer::do_le_node(cdk::le_node *const node, int lvl) {
  ASSERT_SAFE;
  node->left()->accept(this, lvl + 2);
  if (node->left()->is_typed(cdk::TYPE_INT) &&
      node->right()->is_typed(cdk::TYPE_DOUBLE))
    _pf.I2D();
  node->right()->accept(this, lvl + 2);
  if (node->right()->is_typed(cdk::TYPE_INT) &&
      node->left()->is_typed(cdk::TYPE_DOUBLE))
    _pf.I2D();
  _pf.LE();
}
void udf::postfix_writer::do_ge_node(cdk::ge_node *const node, int lvl) {
  ASSERT_SAFE;
  node->left()->accept(this, lvl + 2);
  if (node->left()->is_typed(cdk::TYPE_INT) &&
      node->right()->is_typed(cdk::TYPE_DOUBLE))
    _pf.I2D();
  node->right()->accept(this, lvl + 2);
  if (node->right()->is_typed(cdk::TYPE_INT) &&
      node->left()->is_typed(cdk::TYPE_DOUBLE))
    _pf.I2D();
  _pf.GE();
}
void udf::postfix_writer::do_gt_node(cdk::gt_node *const node, int lvl) {
  ASSERT_SAFE;
  node->left()->accept(this, lvl + 2);
  if (node->left()->is_typed(cdk::TYPE_INT) &&
      node->right()->is_typed(cdk::TYPE_DOUBLE))
    _pf.I2D();
  node->right()->accept(this, lvl + 2);
  if (node->right()->is_typed(cdk::TYPE_INT) &&
      node->left()->is_typed(cdk::TYPE_DOUBLE))
    _pf.I2D();
  _pf.GT();
}
void udf::postfix_writer::do_ne_node(cdk::ne_node *const node, int lvl) {
  ASSERT_SAFE;
  if (node->left()->is_typed(cdk::TYPE_TENSOR) &&
      node->right()->is_typed(cdk::TYPE_TENSOR)) {
    node->left()->accept(this, lvl + 2);
    node->right()->accept(this, lvl + 2);
    _pendingFunctionDeclarations.insert("tensor_equals");
    _pf.CALL("tensor_equals");
    _pf.TRASH(8);
    _pf.LDFVAL64();
    _pf.DOUBLE(0);
    _pf.EQ(); // Tensor inequality is true when the runtime equality returns zero.
    return;
  }
  node->left()->accept(this, lvl + 2);
  if (node->left()->is_typed(cdk::TYPE_INT) &&
      node->right()->is_typed(cdk::TYPE_DOUBLE))
    _pf.I2D();
  node->right()->accept(this, lvl + 2);
  if (node->right()->is_typed(cdk::TYPE_INT) &&
      node->left()->is_typed(cdk::TYPE_DOUBLE))
    _pf.I2D();
  _pf.NE();
}

void udf::postfix_writer::do_eq_node(cdk::eq_node *const node, int lvl) {
  ASSERT_SAFE;
  if (node->left()->is_typed(cdk::TYPE_TENSOR) &&
      node->right()->is_typed(cdk::TYPE_TENSOR)) {
    node->left()->accept(this, lvl + 2);
    node->right()->accept(this, lvl + 2);
    _pendingFunctionDeclarations.insert("tensor_equals");
    _pf.CALL("tensor_equals");
    _pf.TRASH(8);
    _pf.LDFVAL64(); // The runtime already returns the final boolean value.
    return;
  }
  node->left()->accept(this, lvl + 2);
  if (node->left()->is_typed(cdk::TYPE_INT) &&
      node->right()->is_typed(cdk::TYPE_DOUBLE))
    _pf.I2D();
  node->right()->accept(this, lvl + 2);
  if (node->right()->is_typed(cdk::TYPE_INT) &&
      node->left()->is_typed(cdk::TYPE_DOUBLE))
    _pf.I2D();
  _pf.EQ();
}
void udf::postfix_writer::do_and_node(cdk::and_node *const node, int lvl) {
  ASSERT_SAFE;
  int lbl = ++_lbl;
  node->left()->accept(this, lvl + 2);
  _pf.DUP32();
  _pf.JZ(mklbl(lbl)); // Short-circuit when the left operand is already false.
  node->right()->accept(this, lvl + 2);
  _pf.AND();
  _pf.ALIGN();
  _pf.LABEL(mklbl(lbl));
}
void udf::postfix_writer::do_or_node(cdk::or_node *const node, int lvl) {
  ASSERT_SAFE;
  int lbl = ++_lbl;
  node->left()->accept(this, lvl + 2);
  _pf.DUP32();
  _pf.JNZ(mklbl(lbl)); // Short-circuit when the left operand is already true.
  node->right()->accept(this, lvl + 2);
  _pf.OR();
  _pf.ALIGN();
  _pf.LABEL(mklbl(lbl));
}

//---------------------------------------------------------------------------
// FUNCTIONS
//---------------------------------------------------------------------------
void udf::postfix_writer::do_function_call_node(
    udf::function_call_node *const node, int lvl) {
  ASSERT_SAFE;
  auto symbol = _symtab.find(node->name());
  auto ftype = cdk::functional_type::cast(symbol->type());

  size_t argsSize = 0;
  if (node->arguments()->size() > 0) {
    // Push arguments in reverse order to match the target calling convention.
    for (int ax = node->arguments()->size() - 1; ax >= 0; ax--) {
      auto arg = (cdk::expression_node *)node->arguments()->node(ax);
      arg->accept(this, lvl + 2);
      // Apply the implicit promotions expected by the callee signature.
      if (ftype->input(ax)->name() == cdk::TYPE_DOUBLE &&
          arg->is_typed(cdk::TYPE_INT)) {
        _pf.I2D();
      }
      argsSize += ftype->input(ax)->size();
    }
  }

  // Mark unresolved callees so the assembler emits the proper externs.
  if (_defined_functions.find(node->name()) == _defined_functions.end()) {
    _pendingFunctionDeclarations.insert(node->name());
  }

  _pf.CALL(node->name());

  if (argsSize != 0) {
    _pf.TRASH(argsSize); // Release the argument area after the call.
  }

  // Materialize the return value from the ABI-defined return register.
  if (node->is_typed(cdk::TYPE_INT) ||
      node->is_typed(cdk::TYPE_POINTER) ||
      node->is_typed(cdk::TYPE_FUNCTIONAL) ||
      node->is_typed(cdk::TYPE_STRING) ||
      node->is_typed(cdk::TYPE_TENSOR)) {
    _pf.LDFVAL32();
  } else if (node->is_typed(cdk::TYPE_DOUBLE)) {
    _pf.LDFVAL64();
  }
}

void udf::postfix_writer::do_function_node(udf::function_node *const node,
                                           int lvl) {
  ASSERT_SAFE;
  const auto &id = node->identifier();

  if (node->body() == nullptr) {
    return; // Pure declarations do not emit code.
  }

  if (_defined_functions.count(id)) {
    return; // Ignore repeated visits to the same definition.
  }

  _defined_functions.insert(id);
  _pendingFunctionDeclarations.erase(id);

  auto symbol = _symtab.find(id);
  if (!symbol) {
    symbol = udf::make_symbol(false, node->access_specifier(), node->type(),
                              node->identifier(), true, true);
    _symtab.insert(id, symbol);
  }

  _functions.push(symbol);
  reset_new_symbol();

  // Save the current frame state before entering the function scope.
  _offsets.push(_offset);
  _offset = 8; // Arguments begin above the saved frame pointer and return address.
  _symtab.push();

  if (node->arguments()) {
    _context.push(Context::Args);
    node->arguments()->accept(this, 0);
    _context.pop();
  }

  // Function prologue.
  _pf.TEXT();
  _pf.ALIGN();
  if (node->access_specifier() == tPUBLIC)
    _pf.GLOBAL(symbol->name(), _pf.FUNC());
  _pf.LABEL(symbol->name());

  // Compute the stack space required for local storage.
  frame_size_calculator lsc(_compiler, _symtab, _functions);
  node->accept(&lsc, lvl);
  _pf.ENTER(lsc.localsize());

  // Function body.
  _context.push(Context::Body);
  _offset = 0; // Local variables receive negative frame offsets.

  _returnAcknowledged = false;
  if (node->body()) {
    node->body()->accept(this, lvl + 4);
  }

  // Function epilogue.
  _pf.ALIGN();
  _pf.LABEL(symbol->name() + "_epilogue");

  // The language entry point defaults to returning zero when omitted.
  if (node->identifier() == "udf" && !_returnAcknowledged) {
    _pf.INT(0);
    _pf.STFVAL32();
  }
  // Every other non-void function must explicitly return a value.
  else if (cdk::functional_type::cast(node->type())->output(0)->name() !=
               cdk::TYPE_VOID &&
           !_returnAcknowledged) {
    error(node->lineno(), "missing return statement in function '" +
                              node->identifier() + "'");
  }

  _pf.LEAVE();
  _pf.RET();

  // Restore the previous compilation context.
  _context.pop();
  _symtab.pop();
  _offset = _offsets.top();
  _offsets.pop();
  _functions.pop();
}

//---------------------------------------------------------------------------
// TENSOR OPERATIONS
//---------------------------------------------------------------------------
void udf::postfix_writer::do_tensor_node(udf::tensor_node *const node,
                                         int lvl) {
  ASSERT_SAFE;
  auto ttype = cdk::tensor_type::cast(node->type());
  auto dims = ttype->dims();

  // Allocate an empty tensor with the declared shape.
  for (int i = dims.size() - 1; i >= 0; --i) {
    _pf.INT(dims[i]);
  }
  _pf.INT(dims.size());
  _pendingFunctionDeclarations.insert("tensor_create");
  _pf.CALL("tensor_create");
  _pf.TRASH(4 * (dims.size() + 1));
  _pf.LDFVAL32();

  size_t linear_offset = 0;

  // Traverse nested literal sequences in row-major order.
  std::function<void(cdk::sequence_node *)> process_level =
      [&](cdk::sequence_node *current_sequence) {
        for (size_t i = 0; i < current_sequence->size(); ++i) {
          auto element = current_sequence->node(i);
          auto next_sequence = dynamic_cast<cdk::sequence_node *>(element);

          if (next_sequence) {
            process_level(next_sequence); // Descend into the next nesting level.
          } else {
            // Store each scalar literal into the allocated tensor.
            auto expr = dynamic_cast<cdk::expression_node *>(element);
            if (!expr) {
              error(node->lineno(), "Invalid element in tensor literal.");
              return;
            }

            _pf.DUP32(); // Preserve the tensor pointer across runtime calls.
            _pf.INT(linear_offset);
            expr->accept(this, lvl);
            if (expr->is_typed(cdk::TYPE_INT)) {
              _pf.I2D();
            }
            _pendingFunctionDeclarations.insert("tensor_put");
            _pf.CALL("tensor_put");
            _pf.TRASH(16);
            linear_offset++;
          }
        }
      };

  process_level(node->content());
}

void udf::postfix_writer::do_tensor_index_node(
    udf::tensor_index_node *const node, int lvl) {
  ASSERT_SAFE;

  // The parser stores the index list in reverse, so forward iteration emits
  // the runtime arguments in the correct order.
  for (size_t i = 0; i < node->index()->size(); ++i) {
    auto index_expr = (cdk::expression_node *)node->index()->node(i);
    index_expr->accept(this, lvl);
  }

  node->operand()->accept(this, lvl); // Push the tensor handle.
  _pendingFunctionDeclarations.insert("tensor_getptr");
  _pf.CALL("tensor_getptr");
  _pf.TRASH(4 * (node->index()->size() + 1));
  _pf.LDFVAL32(); // Load the pointer to the selected element.
}

void udf::postfix_writer::do_tensor_reshape_node(
    udf::tensor_reshape_node *const node, int lvl) {
  ASSERT_SAFE;
  // Push the new dimensions before the tensor operand.
  for (int i = node->dims()->size() - 1; i >= 0; --i) {
    auto dim_expr = (cdk::expression_node *)node->dims()->node(i);
    dim_expr->accept(this, lvl);
  }
  _pf.INT(node->dims()->size());
  node->operand()->accept(this, lvl); // Push the tensor handle.
  _pendingFunctionDeclarations.insert("tensor_reshape");
  _pf.CALL("tensor_reshape");
  _pf.TRASH(4 * (node->dims()->size() + 2));
  _pf.LDFVAL32(); // Load the reshaped tensor handle.
}

void udf::postfix_writer::do_tensor_contract_node(
    udf::tensor_contract_node *const node, int lvl) {
  ASSERT_SAFE;
  node->right()->accept(this, lvl);
  node->left()->accept(this, lvl);
  _pendingFunctionDeclarations.insert("tensor_matmul");
  _pf.CALL("tensor_matmul");
  _pf.TRASH(8);
  _pf.LDFVAL32();
}

void udf::postfix_writer::do_tensor_rank_node(
    udf::tensor_rank_node *const node, int lvl) {
  ASSERT_SAFE;
  node->operand()->accept(this, lvl);
  _pendingFunctionDeclarations.insert("tensor_get_n_dims");
  _pf.CALL("tensor_get_n_dims");
  _pf.TRASH(4);
  _pf.LDFVAL32();
}

void udf::postfix_writer::do_tensor_dim_node(
    udf::tensor_dim_node *const node, int lvl) {
  ASSERT_SAFE;
  if (node->index()) {
    // Query the size of a specific dimension.
    node->index()->accept(this, lvl);
    node->operand()->accept(this, lvl);
    _pendingFunctionDeclarations.insert("tensor_get_dim_size");
    _pf.CALL("tensor_get_dim_size");
    _pf.TRASH(8);
    _pf.LDFVAL32();
  } else {
    // Query the runtime-owned array with every dimension size.
    node->operand()->accept(this, lvl);
    _pendingFunctionDeclarations.insert("tensor_get_dims");
    _pf.CALL("tensor_get_dims");
    _pf.TRASH(4);
    _pf.LDFVAL32();
  }
}

void udf::postfix_writer::do_tensor_cap_node(
    udf::tensor_cap_node *const node, int lvl) {
  ASSERT_SAFE;
  node->operand()->accept(this, lvl);
  _pendingFunctionDeclarations.insert("tensor_size");
  _pf.CALL("tensor_size"); // Query the total number of stored elements.
  _pf.TRASH(4);
  _pf.LDFVAL32();
}

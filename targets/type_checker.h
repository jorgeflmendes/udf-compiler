#ifndef __UDF_TARGET_TYPE_CHECKER_H__
#define __UDF_TARGET_TYPE_CHECKER_H__

#include "targets/basic_ast_visitor.h"
#include "targets/symbol.h"
#include <cdk/symbol_table.h>

namespace udf {

/**
 * This visitor traverses the AST to perform type checking.
 */
class type_checker : public virtual basic_ast_visitor {
  cdk::symbol_table<udf::symbol> &_symtab; // the symbol table
  std::shared_ptr<udf::symbol> _function;  // the current function being checked
  basic_ast_visitor *_parent; // the parent visitor

public:
  /**
   * the constructor
   */
  type_checker(std::shared_ptr<cdk::compiler> compiler,
               cdk::symbol_table<udf::symbol> &symtab,
               std::shared_ptr<udf::symbol> func,
               basic_ast_visitor *parent)
      : basic_ast_visitor(compiler), _symtab(symtab), _function(func),
        _parent(parent) {}

public:
  /**
   * the destructor
   */
  ~type_checker();

protected:
  // process binary operations with integer-only operands
  void do_IntExpression(cdk::binary_operation_node *const node,
                            int lvl);
  // process binary operations with pointer, integer, or double operands
  void do_PointerIntDoubleExpression(cdk::binary_operation_node *const node, int lvl);
  // process binary operations with integer or double operands
  void do_IntDoubleExpression(cdk::binary_operation_node *const node, int lvl);
  // process scalar logical operations (>, <, >=, <=)
  void do_NumericLogicalExpression(cdk::binary_operation_node *const node,
                                  int lvl);
  // process boolean logical operations (and, or)
  void do_BooleanLogicalExpression(cdk::binary_operation_node *const node,
                                   int lvl);
  // process general equality operations (==, !=)
  void do_GeneralLogicalExpression(cdk::binary_operation_node *const node,
                                   int lvl);
  // recursively check that all tensor literal elements are numeric
  void check_tensor_elements(cdk::sequence_node *sequence, int lvl);
  // recursively infer tensor dimensions from a literal
  std::vector<size_t> infer_tensor_dims(cdk::sequence_node *sequence);

public:
// do not edit these lines
#define __IN_VISITOR_HEADER__
#include ".auto/visitor_decls.h" // automatically generated
#undef __IN_VISITOR_HEADER__
  // do not edit these lines: end
};

//---------------------------------------------------------------------------
// HELPER MACRO FOR TYPE CHECKING
//---------------------------------------------------------------------------

#define CHECK_TYPES(compiler, symtab, function, node)                          \
  {                                                                            \
    try {                                                                      \
      udf::type_checker checker(compiler, symtab, function, this);             \
      (node)->accept(&checker, 0);                                             \
    } catch (const std::string &problem) {                                     \
      std::cerr << (node)->lineno() << ": " << problem << std::endl;           \
      return;                                                                  \
    }                                                                          \
  }

#define ASSERT_SAFE                                                            \
  CHECK_TYPES(_compiler, _symtab,                                              \
              (_functions.size() > 0 ? _functions.top() : nullptr), node)

} // namespace udf

#endif
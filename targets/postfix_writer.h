#ifndef __UDF_TARGET_POSTFIX_WRITER_H__
#define __UDF_TARGET_POSTFIX_WRITER_H__

#include "targets/basic_ast_visitor.h"
#include "targets/symbol.h"
#include <cdk/emitters/basic_postfix_emitter.h>
#include <cdk/symbol_table.h>
#include <set>
#include <sstream>
#include <stack>

namespace udf {

/**
 * This visitor generates postfix assembly code from the AST.
 */
class postfix_writer : public basic_ast_visitor {
  cdk::symbol_table<udf::symbol> &_symtab;
  cdk::basic_postfix_emitter &_pf;
  int _lbl; // label counter for generating unique labels

  // functions that need an EXTERN declaration (runtime and user-defined)
  std::set<std::string> _pendingFunctionDeclarations;

  // tracks the current processing context (global, function args, or body)
  enum class Context { Global, Args, Body };
  std::stack<Context> _context; // stack for managing nested contexts

  bool _returnAcknowledged; // flag to check if a return statement was found

  std::vector<int> _forUpdate, _forEnd; // for-loop labels for break/continue
  
  std::stack<std::shared_ptr<udf::symbol>> _functions;

  int _offset; // current frame offset for locals and arguments
  std::stack<int> _offsets; // save/restore offsets when entering/leaving functions

public:
  postfix_writer(std::shared_ptr<cdk::compiler> compiler,
                 cdk::symbol_table<udf::symbol> &symtab,
                 cdk::basic_postfix_emitter &pf)
      : basic_ast_visitor(compiler), _symtab(symtab), _pf(pf), _lbl(0),
        _returnAcknowledged(false), _offset(0) {
    _context.push(Context::Global);
  }

public:
  ~postfix_writer() { os().flush(); }

private:
  // keeps track of functions defined in the current compilation unit
  std::set<std::string> _defined_functions;

  // creates a label string from an integer
  std::string mklbl(int lbl) {
    std::ostringstream oss;
    oss << "L" << lbl;
    return oss.str();
  }

  // processes the body of a function node
  void do_function_node_body(udf::function_node *b, int a);

  void error(int lineno, std::string s) {
    std::cerr << "error: " << lineno << ": " << s << std::endl;
  }

public:
// do not edit these lines
#define __IN_VISITOR_HEADER__
#include ".auto/visitor_decls.h" // automatically generated
#undef __IN_VISITOR_HEADER__
  // do not edit these lines: end
};

} // namespace udf

#endif
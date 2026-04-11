#ifndef __UDF_TARGETS_SYMBOL_H__
#define __UDF_TARGETS_SYMBOL_H__

#include <cdk/types/basic_type.h>
#include <memory>
#include <string>

namespace udf {

/**
 * Represents an entry in the symbol table.
 * It stores information about variables and functions.
 */
class symbol {
  std::string _name; // identifier
  int _value;        // stores the value for integer constants

  int _qualifier; // qualifiers: public, forward, "private" (i.e., none)
  std::shared_ptr<cdk::basic_type> _type; // type (type id + type size)
  bool _initialized; // is it initialized
  int _offset = 0; // 0 means global variable/function, otherwise it's a local/arg offset
  bool _function;  // true for functions, false for variables
  bool _forward = false; // is it a forward declaration

public:
  /**
   * the constructor
   */
  symbol(bool constant, int qualifier,
         std::shared_ptr<cdk::basic_type> type, const std::string &name,
         bool initialized, bool function, bool forward = false)
      : _name(name), _value(0), _qualifier(qualifier),
        _type(type), _initialized(initialized), _function(function),
        _forward(forward) {}

  /**
   * the destructor
   */
  ~symbol() {}

  const std::string &name() const { return _name; }
  int value() const { return _value; }
  int value(int v) { return _value = v; }

  int qualifier() const { return _qualifier; }

  std::shared_ptr<cdk::basic_type> type() const { return _type; }
  void set_type(std::shared_ptr<cdk::basic_type> t) { _type = t; }
  bool is_typed(cdk::typename_type name) const {
    return _type->name() == name;
  }

  const std::string &identifier() const { return name(); }
  bool initialized() const { return _initialized; }
  int offset() const { return _offset; }
  void set_offset(int offset) { _offset = offset; }
  bool isFunction() const { return _function; }

  bool global() const { return _offset == 0; }
  bool isVariable() const { return !_function; }

  bool forward() const { return _forward; }
};

/**
 * helper function to create a shared pointer to a new symbol
 */
inline auto make_symbol(bool constant, int qualifier,
                        std::shared_ptr<cdk::basic_type> type,
                        const std::string &name, bool initialized,
                        bool function, bool forward = false) {
  return std::make_shared<symbol>(constant, qualifier, type, name,
                                  initialized, function, forward);
}

} // namespace udf

#endif
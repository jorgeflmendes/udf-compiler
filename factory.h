#pragma once

#include <memory>
#include <cdk/yy_factory.h>
#include "udf_scanner.h"

namespace udf {

  /**
   * This class implements the compiler factory for the udf compiler.
   */
  class factory: public cdk::yy_factory<udf_scanner> {
    /**
     * This object is automatically registered by the constructor in the
     * superclass' language registry.
     */
    static factory _self;

  protected:
    /**
     * @param language name of the language handled by this factory (see .cpp file)
     */
    factory(const std::string &language = "udf") :
        cdk::yy_factory<udf_scanner>(language) {
    }

  };

} // udf

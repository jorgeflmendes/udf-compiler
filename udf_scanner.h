#pragma once

#undef yyFlexLexer
#define yyFlexLexer udf_scanner_FlexLexer
#include <FlexLexer.h>

typedef udf_scanner_FlexLexer udf_scanner;

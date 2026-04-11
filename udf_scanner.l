%option c++ prefix="udf_scanner_" outfile="udf_scanner.cpp"
%option stack noyywrap yylineno 8bit debug

%{
#include <string>
#include <sstream>
#include <cdk/ast/sequence_node.h>
#include <cdk/ast/expression_node.h>
#include <cdk/ast/lvalue_node.h>
#include "udf_parser.tab.h"

#define yyerror LexerError
%}

%x STRING_STATE
%x COMMENT_STATE
%x HEX_STATE

%%
yydebug=1; set_debug(1);

"//".*$                               ; /* ignore comments */

"/*"                                  yy_push_state(COMMENT_STATE);
<COMMENT_STATE>"/*"                   yy_push_state(COMMENT_STATE);
<COMMENT_STATE>"*/"                   yy_pop_state();
<COMMENT_STATE>.|"\n"                 ;

/* Keywords */
"int"                                 return tINT;
"real"                                return tREAL;
"string"                              return tSTRING_TYPE;
"void"                                return tVOID;
"tensor"                              return tTENSOR;
"ptr"                                 return tPTR;
"auto"                                return tAUTO;

"public"                              return tPUBLIC;
"forward"                             return tFORWARD;

"if"                                  return tIF;
"elif"                                return tELIF;
"else"                                return tELSE;
"for"                                 return tFOR;
"break"                               return tBREAK;
"continue"                            return tCONTINUE;
"return"                              return tRETURN;

"write"                               return tWRITE;
"writeln"                             return tWRITELN;
"input"                               return tINPUT;

"nullptr"                             return tNULLPTR;
"sizeof"                              return tSIZEOF;
"objects"                             return tOBJECTS;

"capacity"                            return tCAPACITY;
"rank"                                return tRANK;
"dims"                                return tDIMS;
"dim"                                 return tDIM;
"reshape"                             return tRESHAPE;

/* Multi-character operators */
">="                                  return tGE;
"<="                                  return tLE;
"=="                                  return tEQ;
"!="                                  return tNE;
"&&"                                  return tAND;
"||"                                  return tOR;
"**"                                  return tCONTRACT;

/* Identifiers */
[A-Za-z][A-Za-z0-9_]*                {
                                        yylval.s = new std::string(yytext);
                                        return tIDENTIFIER;
                                      }

/* String literals */
\"                                    {
                                        yy_push_state(STRING_STATE);
                                        yylval.s = new std::string("");
                                      }

<STRING_STATE>\"                      {
                                        yy_pop_state();
                                        return tSTRING;
                                      }

<STRING_STATE>\\\"                    *yylval.s += '"';
<STRING_STATE>\\n                     *yylval.s += '\n';
<STRING_STATE>\\t                     *yylval.s += '\t';
<STRING_STATE>\\r                     *yylval.s += '\r';
<STRING_STATE>\\\\                    *yylval.s += '\\';

<STRING_STATE>\\[0-9a-fA-F]{1,2}      {
                                        char c = (char)strtol(yytext+1, nullptr, 16);
                                        *yylval.s += c;
                                      }

<STRING_STATE>\\.                     *yylval.s += yytext[1];
<STRING_STATE>[^\\\"\n\0]+            *yylval.s += yytext;
<STRING_STATE>\n                      yyerror("newline in string");
<STRING_STATE>\0                      yyerror("null character in string");

/* Hexadecimal integer literals */
"0x"                                  yy_push_state(HEX_STATE);

<HEX_STATE>[0-9a-fA-F]+               {
                                        yylval.i = strtol(yytext, nullptr, 16);
                                        yy_pop_state();
                                        return tINTEGER;
                                      }

<HEX_STATE>.                          yyerror("invalid hexadecimal digit");

/* Decimal numeric literals */
[0-9]*\.[0-9]+([eE][-+]?[0-9]+)?      {
                                        yylval.d = strtod(yytext, nullptr);
                                        return tREAL_LITERAL;
                                      }

[0-9]+[eE][-+]?[0-9]+                 {
                                        yylval.d = strtod(yytext, nullptr);
                                        return tREAL_LITERAL;
                                      }

[0-9]+                                {
                                        yylval.i = strtol(yytext, nullptr, 10);
                                        return tINTEGER;
                                      }

/* Single-character punctuation and operators */
[-()<>=+*/%;{},.:!?~@\[\]]            return *yytext;

[ \t\n\r]+                            ; /* ignore whitespace */

.                                     yyerror("Unknown character");

%%

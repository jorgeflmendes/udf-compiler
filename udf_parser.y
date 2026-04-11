%{
//-- don't change *any* of these: if you do, you'll break the compiler.
#include <algorithm>
#include <memory>
#include <stack>
#include <cstring>
#include <cdk/compiler.h>
#include <cdk/types/types.h>
#include ".auto/all_nodes.h"
#define LINE                         compiler->scanner()->lineno()
#define yylex()                      compiler->scanner()->scan()
#define yyerror(compiler, s)         compiler->scanner()->error(s)
//-- don't change *any* of these --- END!

// Extract tensor dimensions from a sequence of integer literal nodes.
static std::vector<size_t> extract_dims(cdk::sequence_node *seq) {
  std::vector<size_t> dims;
  if (!seq) {
    return dims;
  }

  std::stack<cdk::basic_node *> nodes_to_process;
  nodes_to_process.push(seq);

  while (!nodes_to_process.empty()) {
    cdk::basic_node *current_node = nodes_to_process.top();
    nodes_to_process.pop();

    if (!current_node) continue;

    if (auto *int_node = dynamic_cast<cdk::integer_node *>(current_node)) {
      // Integer nodes directly contribute one dimension.
      dims.push_back(static_cast<size_t>(int_node->value()));
    } else if (auto *seq_node =
                   dynamic_cast<cdk::sequence_node *>(current_node)) {
      // Preserve the original order while traversing nested sequences.
      for (int i = seq_node->size() - 1; i >= 0; --i) {
        nodes_to_process.push(seq_node->node(i));
      }
    } else {
      std::cerr << "Warning: Unexpected node type found during "
                   "dimension extraction."
                << std::endl;
    }
  }
  // The traversal order already matches the tensor dimension order.
  return dims;
}

// Build the functional type associated with a function declaration.
static std::shared_ptr<cdk::functional_type>
make_function_type(std::shared_ptr<cdk::basic_type> return_type,
                   cdk::sequence_node *params) {
  std::vector<std::shared_ptr<cdk::basic_type>> input_types;
  if (params) {
    for (size_t i = 0; i < params->size(); ++i) {
      auto param_def =
          dynamic_cast<udf::var_definition_node *>(params->node(i));
      if (param_def) {
        input_types.push_back(param_def->type());
      }
    }
  }
  return cdk::functional_type::create(input_types, return_type);
}

%}

%parse-param {std::shared_ptr<cdk::compiler> compiler}

// Value types carried by grammar symbols.
%union {
  //--- don't change *any* of these: if you do, you'll break the compiler.
  YYSTYPE() : type(cdk::primitive_type::create(0, cdk::TYPE_VOID)) {}
  ~YYSTYPE() {}
  YYSTYPE(const YYSTYPE &other) { *this = other; }
  YYSTYPE &operator=(const YYSTYPE &other) {
    type = other.type;
    return *this;
  }

  std::shared_ptr<cdk::basic_type> type;
  //-- don't change *any* of these --- END!

  int                   i;
  double                d;
  std::string          *s;
  cdk::basic_node      *node;
  cdk::sequence_node   *sequence;
  cdk::expression_node *expression;
  cdk::lvalue_node     *lvalue;
};

// token definitions for literals
%token <i> tINTEGER
%token <d> tREAL_LITERAL
%token <s> tIDENTIFIER tSTRING

// token definitions for keywords
%token tINT tREAL tSTRING_TYPE tVOID tTENSOR tPTR tAUTO
%token tPUBLIC tFORWARD
%token tIF tELIF tELSE tFOR tBREAK tCONTINUE tRETURN
%token tWRITE tWRITELN tINPUT
%token tNULLPTR tSIZEOF tOBJECTS
%token tCAPACITY tRANK tDIMS tDIM tRESHAPE

// token definitions for operators
%token tGE tLE tEQ tNE tAND tOR tCONTRACT

// operator precedence and associativity, from lowest to highest
%nonassoc tIFX
%nonassoc tELSE tELIF
%right '='
%left tOR
%left tAND
%right tUNARY '~' '?'
%left tEQ tNE
%left tGE tLE
%left '>' '<'
%left '+' '-'
%left '*' '/' '%'
%left tCONTRACT
%left '.' '@'
%left '[' '('
%nonassoc TYPE_BRACKETS

// type declarations for non-terminal symbols
%type <node> program declaration variable_declaration function_declaration
%type <node> statement code_block
%type <node> break_statement continue_statement return_statement
%type <node> for_statement if_statement elif_chain tensor_item
%type <sequence> declaration_list parameter_list_opt parameter_list
%type <sequence> expression_list_opt expression_list tensor_dimensions
%type <sequence> tensor_content tensor_elements declarations statements
%type <sequence> variable_list
%type <expression> expression scalar_expression tensor_expression
%type <expression> variable_initializer_opt variable_initializer
%type <lvalue> lvalue
%type <type> data_type
%type <s> string_literals

%%

//-- PROGRAM STRUCTURE
program
    : declaration_list
        { compiler->ast(new udf::program_node(LINE, $1)); }
    ;

declaration_list
    : /* empty */
        { $$ = new cdk::sequence_node(LINE); }
    | declaration_list declaration
        { $$ = new cdk::sequence_node(LINE, $2, $1); }
    ;

declaration
    : variable_declaration ';'
        { $$ = $1; }
    | function_declaration
        { $$ = $1; }
    ;

//-- DATA TYPES
data_type
    : tINT
        { $$ = cdk::primitive_type::create(4, cdk::TYPE_INT); }
    | tREAL
        { $$ = cdk::primitive_type::create(8, cdk::TYPE_DOUBLE); }
    | tSTRING_TYPE
        { $$ = cdk::primitive_type::create(4, cdk::TYPE_STRING); }
    | tVOID
        { $$ = cdk::primitive_type::create(0, cdk::TYPE_VOID); }
    | tTENSOR '<' tensor_dimensions '>' %prec TYPE_BRACKETS
        {
          std::vector<size_t> dims = extract_dims($3);
          $$ = cdk::tensor_type::create(dims);
        }
    | tPTR '<' data_type '>' %prec TYPE_BRACKETS
        { $$ = cdk::reference_type::create(4, $3); }
    | tPTR '<' tAUTO '>' %prec TYPE_BRACKETS
        { $$ = cdk::reference_type::create(4, nullptr); }
    ;

tensor_dimensions
    : tINTEGER
        { $$ = new cdk::sequence_node(LINE, new cdk::integer_node(LINE, $1)); }
    | tensor_dimensions ',' tINTEGER
        { $$ = new cdk::sequence_node(LINE, new cdk::integer_node(LINE, $3), $1); }
    ;

//-- VARIABLE AND FUNCTION DECLARATIONS
variable_declaration
    : data_type tIDENTIFIER variable_initializer_opt
        { $$ = new udf::var_definition_node(LINE, 0, *$2, $1, $3); delete $2; }
    | tPUBLIC data_type tIDENTIFIER variable_initializer_opt
        { $$ = new udf::var_definition_node(LINE, 1, *$3, $2, $4); delete $3; }
    | tFORWARD data_type tIDENTIFIER variable_initializer_opt
        { $$ = new udf::var_definition_node(LINE, 2, *$3, $2, $4); delete $3; }
    | tAUTO tIDENTIFIER variable_initializer
        { $$ = new udf::var_definition_node(LINE, 0, *$2, cdk::primitive_type::create(0, cdk::TYPE_UNSPEC), $3); delete $2; }
    | tPUBLIC tAUTO tIDENTIFIER variable_initializer
        { $$ = new udf::var_definition_node(LINE, 1, *$3, cdk::primitive_type::create(0, cdk::TYPE_UNSPEC), $4); delete $3; }
    ;

variable_initializer
    : '=' expression      { $$ = $2; }
    ;

variable_initializer_opt
    : /* empty */         { $$ = nullptr; }
    | variable_initializer      { $$ = $1; }
    ;

function_declaration
    : data_type tIDENTIFIER '(' parameter_list_opt ')' code_block
        {
          auto ftype = make_function_type($1, $4);
          $$ = new udf::function_node(LINE, 0, ftype, *$2, $4, dynamic_cast<udf::code_block_node*>($6));
          delete $2;
        }
    | data_type tIDENTIFIER '(' parameter_list_opt ')'
        {
          auto ftype = make_function_type($1, $4);
          $$ = new udf::function_node(LINE, 0, ftype, *$2, $4, nullptr);
          delete $2;
        }
    | tPUBLIC data_type tIDENTIFIER '(' parameter_list_opt ')' code_block
        {
          auto ftype = make_function_type($2, $5);
          $$ = new udf::function_node(LINE, 1, ftype, *$3, $5, dynamic_cast<udf::code_block_node*>($7));
          delete $3;
        }
    | tPUBLIC data_type tIDENTIFIER '(' parameter_list_opt ')'
        {
          auto ftype = make_function_type($2, $5);
          $$ = new udf::function_node(LINE, 1, ftype, *$3, $5, nullptr);
          delete $3;
        }
    | tFORWARD data_type tIDENTIFIER '(' parameter_list_opt ')' code_block
        {
          auto ftype = make_function_type($2, $5);
          $$ = new udf::function_node(LINE, 2, ftype, *$3, $5, dynamic_cast<udf::code_block_node*>($7));
          delete $3;
        }
    | tFORWARD data_type tIDENTIFIER '(' parameter_list_opt ')'
        {
          auto ftype = make_function_type($2, $5);
          $$ = new udf::function_node(LINE, 2, ftype, *$3, $5, nullptr);
          delete $3;
        }
    | tAUTO tIDENTIFIER '(' parameter_list_opt ')' code_block
        {
          auto ftype = make_function_type(cdk::primitive_type::create(0, cdk::TYPE_UNSPEC), $4);
          $$ = new udf::function_node(LINE, 0, ftype, *$2, $4, dynamic_cast<udf::code_block_node*>($6));
          delete $2;
        }
    | tPUBLIC tAUTO tIDENTIFIER '(' parameter_list_opt ')' code_block
        {
          auto ftype = make_function_type(cdk::primitive_type::create(0, cdk::TYPE_UNSPEC), $5);
          $$ = new udf::function_node(LINE, 1, ftype, *$3, $5, dynamic_cast<udf::code_block_node*>($7));
          delete $3;
        }
    | tFORWARD tAUTO tIDENTIFIER '(' parameter_list_opt ')' code_block
        {
          auto ftype = make_function_type(cdk::primitive_type::create(0, cdk::TYPE_UNSPEC), $5);
          $$ = new udf::function_node(LINE, 2, ftype, *$3, $5, dynamic_cast<udf::code_block_node*>($7));
          delete $3;
        }
    ;

//-- FUNCTION PARAMETERS
parameter_list_opt
    : /* empty */            { $$ = new cdk::sequence_node(LINE); }
    | parameter_list         { $$ = $1; }
    ;

parameter_list
    : variable_declaration
        { $$ = new cdk::sequence_node(LINE, $1); }
    | parameter_list ',' variable_declaration
        { $$ = new cdk::sequence_node(LINE, $3, $1); }
    ;

//-- CODE BLOCKS AND STATEMENTS
code_block
    : '{' declarations statements '}'
        { $$ = new udf::code_block_node(LINE, $2, $3); }
    ;

declarations
    : /* empty */
        { $$ = new cdk::sequence_node(LINE); }
    | declarations variable_declaration ';'
        { $$ = new cdk::sequence_node(LINE, $2, $1); }
    ;

statements
    : /* empty */
        { $$ = new cdk::sequence_node(LINE); }
    | statements statement
        { $$ = new cdk::sequence_node(LINE, $2, $1); }
    ;

statement
    : expression ';'
        { $$ = new udf::evaluation_node(LINE, $1); }
    | tWRITE expression_list_opt ';'
        { $$ = new udf::write_node(LINE, $2, false); }
    | tWRITELN expression_list_opt ';'
        { $$ = new udf::write_node(LINE, $2, true); }
    | break_statement
        { $$ = $1; }
    | continue_statement
        { $$ = $1; }
    | return_statement
        { $$ = $1; }
    | if_statement
        { $$ = $1; }
    | for_statement
        { $$ = $1; }
    | code_block
        { $$ = $1; }
    ;

//-- CONTROL FLOW STATEMENTS
break_statement
    : tBREAK
        { $$ = new udf::break_node(LINE); }
    ;

continue_statement
    : tCONTINUE
        { $$ = new udf::continue_node(LINE); }
    ;

return_statement
    : tRETURN ';'
        { $$ = new udf::return_node(LINE); }
    | tRETURN expression ';'
        { $$ = new udf::return_node(LINE, $2); }
    ;

variable_list
    : variable_declaration
        { $$ = new cdk::sequence_node(LINE, $1); }
    | variable_list ',' variable_declaration
        { $$ = new cdk::sequence_node(LINE, $3, $1); }
    ;

for_statement
    : tFOR '(' variable_list ';' expression_list_opt ';' expression_list_opt ')' statement
        { $$ = new udf::for_node(LINE, $3, $5, $7, $9); }
    | tFOR '(' expression_list_opt ';' expression_list_opt ';' expression_list_opt ')' statement
        { $$ = new udf::for_node(LINE, $3, $5, $7, $9); }
    ;

if_statement
    : tIF '(' expression ')' statement %prec tIFX
        { $$ = new udf::if_node(LINE, $3, $5); }
    | tIF '(' expression ')' statement elif_chain
        { $$ = new udf::if_else_node(LINE, $3, $5, $6); }
    | tIF '(' expression ')' statement tELSE statement
        { $$ = new udf::if_else_node(LINE, $3, $5, $7); }
    ;

elif_chain
    : tELIF '(' expression ')' statement %prec tIFX
        { $$ = new udf::if_node(LINE, $3, $5); }
    | tELIF '(' expression ')' statement elif_chain
        { $$ = new udf::if_else_node(LINE, $3, $5, $6); }
    | tELIF '(' expression ')' statement tELSE statement
        { $$ = new udf::if_else_node(LINE, $3, $5, $7); }
    ;

//-- EXPRESSIONS
expression
    : scalar_expression          { $$ = $1; }
    | tensor_expression          { $$ = $1; }
    ;

scalar_expression
    : tINTEGER
        { $$ = new cdk::integer_node(LINE, $1); }
    | tREAL_LITERAL
        { $$ = new cdk::double_node(LINE, $1); }
    | string_literals
        { $$ = new cdk::string_node(LINE, $1); }
    | tNULLPTR
        { $$ = new udf::null_node(LINE); }
    | tINPUT
        { $$ = dynamic_cast<cdk::expression_node*>(new udf::input_node(LINE)); }
    | '-' expression %prec tUNARY
        { $$ = new cdk::unary_minus_node(LINE, $2); }
    | '+' expression %prec tUNARY
        { $$ = new cdk::unary_plus_node(LINE, $2); }
    | '~' expression %prec tUNARY
        { $$ = new cdk::not_node(LINE, $2); }
    | lvalue '?' %prec tUNARY
        { $$ = new udf::unary_position_node(LINE, $1); }
    | expression '+' expression
        { $$ = new cdk::add_node(LINE, $1, $3); }
    | expression '-' expression
        { $$ = new cdk::sub_node(LINE, $1, $3); }
    | expression '*' expression
        { $$ = new cdk::mul_node(LINE, $1, $3); }
    | expression '/' expression
        { $$ = new cdk::div_node(LINE, $1, $3); }
    | expression '%' expression
        { $$ = new cdk::mod_node(LINE, $1, $3); }
    | expression tCONTRACT expression
        { $$ = new udf::tensor_contract_node(LINE, $1, $3); }
    | expression '<' expression
        { $$ = new cdk::lt_node(LINE, $1, $3); }
    | expression '>' expression
        { $$ = new cdk::gt_node(LINE, $1, $3); }
    | expression tGE expression
        { $$ = new cdk::ge_node(LINE, $1, $3); }
    | expression tLE expression
        { $$ = new cdk::le_node(LINE, $1, $3); }
    | expression tNE expression
        { $$ = new cdk::ne_node(LINE, $1, $3); }
    | expression tEQ expression
        { $$ = new cdk::eq_node(LINE, $1, $3); }
    | expression tAND expression
        { $$ = new cdk::and_node(LINE, $1, $3); }
    | expression tOR expression
        { $$ = new cdk::or_node(LINE, $1, $3); }
    | tOBJECTS '(' expression ')'
        { $$ = new udf::memory_allocation_node(LINE, $3); }
    | tSIZEOF '(' expression ')'
        { $$ = new udf::size_of_node(LINE, $3); }
    | expression '.' tCAPACITY
        { $$ = new udf::tensor_cap_node(LINE, $1); }
    | expression '.' tRANK
        { $$ = new udf::tensor_rank_node(LINE, $1); }
    | expression '.' tDIMS
        { $$ = new udf::tensor_dim_node(LINE, $1); }
    | expression '.' tDIM '(' expression ')'
        { $$ = new udf::tensor_dim_node(LINE, $1, $5); }
    | expression '.' tRESHAPE '(' expression_list ')'
        { $$ = new udf::tensor_reshape_node(LINE, $1, $5); }
    | '(' expression ')'
        { $$ = $2; }
    | lvalue
        { $$ = new cdk::rvalue_node(LINE, $1); }
    | lvalue '=' expression
        { $$ = new cdk::assignment_node(LINE, $1, $3); }
    | tIDENTIFIER '(' expression_list_opt ')'
        { $$ = new udf::function_call_node(LINE, *$1, $3); delete $1; }
    ;

tensor_expression
    : '[' tensor_content ']'
        { $$ = new udf::tensor_node(LINE, $2); }
    ;

//-- LVALUES (expressions that can appear on the left of an assignment)
lvalue
    : tIDENTIFIER
        { $$ = new cdk::variable_node(LINE, *$1); delete $1; }
    | expression '@' '(' expression_list ')'
        { $$ = new udf::tensor_index_node(LINE, $1, $4); }
    | expression '[' expression ']'
        { $$ = new udf::element_access_node(LINE, $1, $3); }
    ;

//-- EXPRESSION AND TENSOR LISTS
expression_list_opt
    : /* empty */               { $$ = new cdk::sequence_node(LINE); }
    | expression_list           { $$ = $1; }
    ;

expression_list
    : expression
        { $$ = new cdk::sequence_node(LINE, $1); }
    | expression_list ',' expression
        { $$ = new cdk::sequence_node(LINE, $3, $1); }
    ;

tensor_content
    : /* empty */                { $$ = new cdk::sequence_node(LINE); }
    | tensor_elements            { $$ = $1; }
    ;

tensor_elements
    : tensor_item
        { $$ = new cdk::sequence_node(LINE, $1); }
    | tensor_elements ',' tensor_item
        { $$ = new cdk::sequence_node(LINE, $3, $1); }
    ;

tensor_item
    : scalar_expression          { $$ = $1; }
    | '[' tensor_content ']'     { $$ = $2; }
    ;

//-- AUXILIARY RULES
string_literals
    : tSTRING
        { $$ = $1; }
    | string_literals tSTRING
        {
          *$1 += *$2;
          delete $2;
          $$ = $1;
        }
    ;

%%

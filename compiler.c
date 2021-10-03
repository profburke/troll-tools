#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"

static void advance(void);
static void errorAt(Token* token, const char* message);
static void error(const char* message);
static void errorAtCurrent(const char* message);
static void consume(TokenType type, const char* message);
static void emitByte(uint8_t byte);
static void endCompiler(void);
static void emitReturn(void);
static void emitBytes(uint8_t byte1, uint8_t byte2);
static void expression(void);
static void integer(void);
static void emitConstant(Value value);
static uint8_t makeConstant(Value value);
static void grouping(void);
static void unary(void);
static void dieroll(void);
static void question(void);
static void string(void);
static void pair(void);
static void pairSelector(void);

typedef struct {
  Token current;
  Token previous;
  bool hadError;
  bool panicMode;
} Parser;

typedef enum {
  PREC_NONE,
  PREC_SEMICOLON, 
  PREC_ELSE, // else, while, until, do
  PREC_CONCAT, // <|, |>, etc,
  PREC_RANGE, // ..
  PREC_DROP, // drop, keep, pick, --
  PREC_UNION, // U, &
  PREC_TERM,
  PREC_FACTOR,
  PREC_UNARY_MINUS,
  PREC_AGGREGATE, // choose, count, sum, ..., %1, %2
  PREC_RELATIONAL,
  PREC_MULTIDIE, // infix D, infix Z, #, ~
  PREC_DIE,
  PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)(void);

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

Parser parser;
Chunk* compilingChunk;

static void parsePrecedence(Precedence precedence);

static Chunk* currentChunk() {
  return compilingChunk;
}

ParseRule rules[];

static ParseRule* getRule(TokenType type) {
  return &rules[type];
}

bool compile(const char* source, Chunk *chunk) {
  initScanner(source);
  compilingChunk = chunk;
  
  parser.panicMode = false;
  parser.hadError = false;
  
  advance();
  expression();
  consume(TOKEN_EOF, "Expected end of expression.");
  endCompiler();
  
  return !parser.hadError;
}

static void endCompiler() {
  emitReturn();
}

static void advance() {
  parser.previous = parser.current;

  for (;;) {
    parser.current = scanToken();
    if (parser.current.type != TOKEN_ERROR) { break; }

    errorAtCurrent(parser.current.start);
  }
}

static void errorAtCurrent(const char* message) {
  errorAt(&parser.current, message);
}

static void error(const char* message) {
  errorAt(&parser.previous, message);
}

static void errorAt(Token* token, const char* message) {
  if (parser.panicMode) { return; }
  parser.panicMode = true;
  
  fprintf(stderr, "[line %d] Error", token->line);

  if (token->type == TOKEN_EOF) {
    fprintf(stderr, " at end");
  } else if (token->type == TOKEN_ERROR) {
    // Nothing
  } else {
    fprintf(stderr, " at '%.*s'", token->length, token->start);
  }

  fprintf(stderr, ": %s\n", message);
  parser.hadError = true;
}

static void consume(TokenType type, const char* message) {
  if (parser.current.type == type) {
    advance();
    return;
  }

  errorAtCurrent(message);
}

static void emitByte(uint8_t byte) {
  writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2) {
  emitByte(byte1);
  emitByte(byte2);
}

static void emitReturn() {
  emitByte(OP_RETURN);
}

static void integer() {
  int value = atoi(parser.previous.start);
  emitConstant(INTEGER_VAL(value));
}

static void emitConstant(Value value) {
  emitBytes(OP_CONSTANT, makeConstant(value));
}

static uint8_t makeConstant(Value value) {
  int constant = addConstant(currentChunk(), value);
  if (constant > UINT8_MAX) {
    error("Too many constants in one chunk.");
    return 0;
  }

  return (uint8_t)constant;
}

static void grouping() {
  expression();
  consume(TOKEN_RPAREN, "Expect ')' after expression.");
}

static void unary() {
  TokenType operatorType = parser.previous.type;

  parsePrecedence(PREC_UNARY_MINUS);

  switch (operatorType) {
  case TOKEN_MINUS: emitByte(OP_NEGATE); break;
  default: return;
  }
}

static void dieroll() {
    TokenType operatorType = parser.previous.type;

    // TODO: correct precedence?
    parsePrecedence(PREC_DIE);

    switch (operatorType) {
    case TOKEN_DIE: emitByte(OP_DIE); break;
    case TOKEN_ZERO_DIE: emitByte(OP_ZERO_DIE); break;
    default: return;
    }
}

static void binary() {
  TokenType operatorType = parser.previous.type;
  ParseRule* rule = getRule(operatorType);
  parsePrecedence((Precedence)(rule->precedence + 1));

  switch (operatorType) {
  case TOKEN_PLUS: emitByte(OP_ADD); break;
  case TOKEN_MINUS: emitByte(OP_SUBTRACT); break;
  case TOKEN_MOD: emitByte(OP_MOD); break;
  case TOKEN_TIMES: emitByte(OP_MULTIPLY); break;
  case TOKEN_DIVIDE: emitByte(OP_DIVIDE); break;
  case TOKEN_HCONC: emitByte(OP_HCONC); break;
  case TOKEN_VCONCL: emitByte(OP_VCONCL); break;
  case TOKEN_VCONCR: emitByte(OP_VCONCR); break;
  case TOKEN_VCONCC: emitByte(OP_VCONCC); break;
  case TOKEN_DIE: emitByte(OP_MDIE); break;
  case TOKEN_ZERO_DIE: emitByte(OP_MZDIE); break;
  case TOKEN_UNION: emitByte(OP_UNION); break;
  case TOKEN_DOT_DOT: emitByte(OP_RANGE); break;
  default: return;
  }
}

static void question() {
  // already consumed the '?'
  consume(TOKEN_REAL, "Expect number in range (0, 1.0) after '?'.");
  double value = strtod(parser.previous.start, NULL);
  // TODO: check that 0 < value < 1
  emitConstant(REAL_VAL(value));
  emitByte(OP_QUESTION);
}

static void pair() {
  expression();
  consume(TOKEN_COMMA, "Pair expressions must be separated by ','.");
  expression();
  consume(TOKEN_RBRACK, "Pair must be closed with a ']'.");
  emitByte(OP_MKPAIR);
}

static void pairSelector() {
  TokenType operatorType = parser.previous.type;

  parsePrecedence(PREC_AGGREGATE); // TODO: I don't think this is correct

  switch (operatorType) {
  case TOKEN_FIRST: emitByte(OP_FIRST); break;
  case TOKEN_SECOND: emitByte(OP_SECOND); break;
  default: return;
  }
}

static void string() {
  // TODO: pull embedded CONC operators out of string...
  emitConstant(OBJ_VAL(copyString(parser.previous.start + 1, parser.previous.length - 2)));
}

static void expression() {
  parsePrecedence(PREC_CONCAT);
}

static void parsePrecedence(Precedence precedence) {
  advance();
  ParseFn prefixRule = getRule(parser.previous.type) -> prefix;
  if (prefixRule == NULL) {
    error("Expect expression.");
    return;
  }

  prefixRule();

  while (precedence <= getRule(parser.current.type)->precedence) {
    advance();
    ParseFn infixRule = getRule(parser.previous.type)->infix;
    infixRule();
  }
}

ParseRule rules[] = {
  [TOKEN_DIE]           = {dieroll, binary, PREC_MULTIDIE},
  [TOKEN_ZERO_DIE]      = {dieroll, binary, PREC_MULTIDIE},
  [TOKEN_UNION]         = {NULL, binary, PREC_UNION},
  [TOKEN_PLUS]          = {NULL, binary, PREC_TERM},
  [TOKEN_TIMES]         = {NULL, binary, PREC_FACTOR},
  [TOKEN_DIVIDE]        = {NULL, binary, PREC_FACTOR},
  [TOKEN_LPAREN]        = {grouping, NULL, PREC_NONE},
  [TOKEN_RPAREN]        = {NULL, NULL, PREC_NONE},
  [TOKEN_COMMA]         = {NULL, NULL, PREC_NONE},
  [TOKEN_SEMICOLON]     = {NULL, NULL, PREC_NONE},
  [TOKEN_LBRACE]        = {NULL, NULL, PREC_NONE},
  [TOKEN_RBRACE]        = {NULL, NULL, PREC_NONE},
  [TOKEN_TILDE]         = {NULL, NULL, PREC_NONE},
  [TOKEN_BANG]          = {NULL, NULL, PREC_NONE},
  [TOKEN_AND]           = {NULL, NULL, PREC_NONE},
  [TOKEN_HASH]          = {NULL, NULL, PREC_NONE},
  [TOKEN_QUESTION]      = {question, NULL, PREC_NONE}, // TODO: we don't really need precedence, do we?
  [TOKEN_SAMPLE]        = {NULL, NULL, PREC_NONE},
  [TOKEN_LBRACK]        = {pair, NULL, PREC_NONE},
  [TOKEN_RBRACK]        = {NULL, NULL, PREC_NONE},
  [TOKEN_MINUS]         = {unary, binary, PREC_TERM},
  [TOKEN_SET_MINUS]     = {NULL, NULL, PREC_NONE},
  [TOKEN_ASSIGN]        = {NULL, NULL, PREC_NONE},
  [TOKEN_EQ]            = {NULL, NULL, PREC_NONE},
  [TOKEN_NEQ]           = {NULL, NULL, PREC_NONE},
  [TOKEN_LT]            = {NULL, NULL, PREC_NONE},
  [TOKEN_GT]            = {NULL, NULL, PREC_NONE},
  [TOKEN_LE]            = {NULL, NULL, PREC_NONE},
  [TOKEN_GE]            = {NULL, NULL, PREC_NONE},
  [TOKEN_DOT_DOT]       = {NULL, binary, PREC_RANGE},
  [TOKEN_HCONC]         = {NULL, binary, PREC_CONCAT},
  [TOKEN_VCONCL]        = {NULL, binary, PREC_CONCAT},
  [TOKEN_VCONCR]        = {NULL, binary, PREC_CONCAT},
  [TOKEN_VCONCC]        = {NULL, binary, PREC_CONCAT},
  [TOKEN_FIRST]         = {pairSelector, NULL, PREC_NONE},
  [TOKEN_SECOND]        = {pairSelector, NULL, PREC_NONE},
  [TOKEN_INTEGER]       = {integer, NULL, PREC_NONE},
  [TOKEN_REAL]          = {NULL, NULL, PREC_NONE},
  [TOKEN_IDENTIFIER]    = {NULL, NULL, PREC_NONE},
  [TOKEN_STRING]        = {string, NULL, PREC_NONE},
  [TOKEN_SUM]           = {NULL, NULL, PREC_NONE},
  [TOKEN_SGN]           = {NULL, NULL, PREC_NONE},
  [TOKEN_MOD]           = {NULL, binary, PREC_FACTOR},
  [TOKEN_LEAST]         = {NULL, NULL, PREC_NONE},
  [TOKEN_LARGEST]       = {NULL, NULL, PREC_NONE},
  [TOKEN_COUNT]         = {NULL, NULL, PREC_NONE},
  [TOKEN_DROP]          = {NULL, NULL, PREC_NONE},
  [TOKEN_KEEP]          = {NULL, NULL, PREC_NONE},
  [TOKEN_PICK]          = {NULL, NULL, PREC_NONE},
  [TOKEN_MEDIAN]        = {NULL, NULL, PREC_NONE},
  [TOKEN_IN]            = {NULL, NULL, PREC_NONE},
  [TOKEN_REPEAT]        = {NULL, NULL, PREC_NONE},
  [TOKEN_ACCUMULATE]    = {NULL, NULL, PREC_NONE},
  [TOKEN_WHILE]         = {NULL, NULL, PREC_NONE},
  [TOKEN_UNTIL]         = {NULL, NULL, PREC_NONE},
  [TOKEN_FOREACH]       = {NULL, NULL, PREC_NONE},
  [TOKEN_DO]            = {NULL, NULL, PREC_NONE},
  [TOKEN_IF]            = {NULL, NULL, PREC_NONE},
  [TOKEN_THEN]          = {NULL, NULL, PREC_NONE},
  [TOKEN_ELSE]          = {NULL, NULL, PREC_NONE},
  [TOKEN_MIN]           = {NULL, NULL, PREC_NONE},
  [TOKEN_MAX]           = {NULL, NULL, PREC_NONE},
  [TOKEN_MINIMAL]       = {NULL, NULL, PREC_NONE},
  [TOKEN_MAXIMAL]       = {NULL, NULL, PREC_NONE},
  [TOKEN_CHOOSE]        = {NULL, NULL, PREC_NONE},
  [TOKEN_DIFFERENT]     = {NULL, NULL, PREC_NONE},
  [TOKEN_FUNCTION]      = {NULL, NULL, PREC_NONE},
  [TOKEN_CALL]          = {NULL, NULL, PREC_NONE},
  [TOKEN_COMPOSITIONAL] = {NULL, NULL, PREC_NONE},
  [TOKEN_ERROR]         = {NULL, NULL, PREC_NONE},
  [TOKEN_EOF]           = {NULL, NULL, PREC_NONE},
};

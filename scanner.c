#include <stdio.h>
#include <string.h>

#include "common.h"
#include "scanner.h"

static bool isAtEnd(void);
static Token makeToken(TokenType type);
static Token errorToken(const char* message);
static char advance(void);
static bool match(char expected);
static void skipWhitespace(void);
static char peek(void);
static char peekNext(void);
static Token string(void);
static bool isDigit(char c);
static bool isNonzeroDigit(char c);
static bool isAlpha(char c);
static Token realOrZero(void);
static Token integer(void);
static Token identifier(void);
static TokenType identifierType(void);
static TokenType checkKeyword(int start, int length, const char* rest, TokenType type);

typedef struct {
  const char* start;
  const char* current;
  int line;
} Scanner;

Scanner scanner;

void initScanner(const char* source) {
  scanner.start = source;
  scanner.current = source;
  scanner.line = 1;
}

Token scanToken(void) {
  skipWhitespace();
  scanner.start = scanner.current;

  if (isAtEnd()) { return makeToken(TOKEN_EOF); }

  char c = advance();
  if (isNonzeroDigit(c)) { return integer(); }
  if (isAlpha(c)) { return identifier(); }
  
  switch (c) {
  case '0': return realOrZero();
  case '@': return makeToken(TOKEN_UNION);
  case '+': return makeToken(TOKEN_PLUS);
  case '*': return makeToken(TOKEN_TIMES);
  case '/': return makeToken(TOKEN_DIVIDE);
  case '(': return makeToken(TOKEN_LPAREN);
  case ')': return makeToken(TOKEN_RPAREN);
  case ',': return makeToken(TOKEN_COMMA);
  case ';': return makeToken(TOKEN_SEMICOLON);
  case '{': return makeToken(TOKEN_LBRACE);
  case '}': return makeToken(TOKEN_RBRACE);
  case '~': return makeToken(TOKEN_TILDE);
  case '!': return makeToken(TOKEN_BANG);
  case '&': return makeToken(TOKEN_AND);
  case '#': return makeToken(TOKEN_HASH);
  case '?': return makeToken(TOKEN_QUESTION);
  case '\'': return makeToken(TOKEN_SAMPLE);
  case '[': return makeToken(TOKEN_LBRACK);
  case ']': return makeToken(TOKEN_RBRACK);
    
  case ':':
    if (match('=')) {
      return makeToken(TOKEN_ASSIGN);
    } else {
      return errorToken("':' must be followed by '='.");
    }
  case '-':
    return makeToken(match('-') ? TOKEN_SET_MINUS : TOKEN_MINUS);
  case '>':
    return makeToken(match('=') ? TOKEN_GE : TOKEN_GT);
  case '<':
    if (match('=')) {
      return makeToken(TOKEN_LE);
    } else if (match('>')) {
      return makeToken(TOKEN_VCONCC);
    } else if (match('|')) {
      return makeToken(TOKEN_VCONCR);
    } else {
      return makeToken(TOKEN_LT);
    }
  case '|':
    if (match('|')) {
      return makeToken(TOKEN_HCONC);
    } else if (match('>')) {
      return makeToken(TOKEN_VCONCL);
    } else {
      return errorToken("'|' must be followed by either '|' or '>'.");
    }
  case '%':
    if (match('1')) {
      return makeToken(TOKEN_FIRST);
    } else if (match('2')) {
      return makeToken(TOKEN_SECOND);
    } else {
      return errorToken("'%' must be followed by either '1' or '2'.");
    }
  case '=':
    if (match('/') && match('=')) {
      return makeToken(TOKEN_NEQ);
    } else {
      return makeToken(TOKEN_EQ);
    }
  case '.':
    if (match('.')) {
      return makeToken(TOKEN_DOT_DOT);
    } else {
      return errorToken("'.' must be followed by a second '.'.");
    }

  case '"': return string();
  }
  
  return errorToken("Unexpected character.");
}

static Token string() {
  while (peek() != '"' && peek() != '\n' && !isAtEnd()) {
    advance();
  }

  if (isAtEnd() || peek() == '\n') {
    return errorToken("Unexpected character.");
  }

  advance(); // past the closing "
  return makeToken(TOKEN_STRING);
}

static bool isAtEnd() {
  return *scanner.current == '\0';
}

static Token makeToken(TokenType type) {
  Token token;

  token.type = type;
  token.start = scanner.start;
  token.length = (int)(scanner.current - scanner.start);
  token.line = scanner.line;

  return token;
}

static Token errorToken(const char* message) {
  Token token;
  
  token.type = TOKEN_ERROR;
  token.start = message;
  token.length = (int)strlen(message);
  token.line = scanner.line;

  return token;
}

static char advance() {
  scanner.current++;
  return scanner.current[-1];
}

static bool match(char expected) {
  if (isAtEnd()) { return false; }
  if (*scanner.current != expected) { return false; }
  scanner.current++;
  return true;
}

static void skipWhitespace() {
  for (;;) {
    char c = peek();
    switch (c) {
    case ' ':
    case '\r':
    case '\t':
      advance();
      break;
    case '\n':
      scanner.line++;
      advance();
      break;
    case '/':
      if (peekNext() == '/') {
        while (peek() != '\n' && !isAtEnd()) { advance(); }
      } else {
        return;
      }
      break;
    default:
      return;
    }
  }
}

static char peek() {
  return *scanner.current;
}

static char peekNext() {
  if (isAtEnd()) { return '\0'; }
  return scanner.current[1];
}

static bool isDigit(char c) {
  return c >= '0' && c <= '9';
}

static bool isNonzeroDigit(char c) {
  return c >= '1' && c <= '9';
}

static bool isAlpha(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static Token realOrZero() {
  if (peek() == '.') {
    advance();
    while (isDigit(peek())) { advance(); }

    return makeToken(TOKEN_REAL);
  } else {
    return makeToken(TOKEN_INTEGER);
  }
}

static Token integer() {
  while (isDigit(peek())) { advance(); }

  return makeToken(TOKEN_INTEGER);
}

static Token identifier() {
  while (isAlpha(peek())) { advance(); }

  return makeToken(identifierType());
}

static TokenType identifierType() {
  switch (scanner.start[0]) {
  case 'D': return checkKeyword(1, 0, "", TOKEN_DIE);
  case 'U': return checkKeyword(1, 0, "", TOKEN_UNION);
  case 'Z': return checkKeyword(1, 0, "", TOKEN_ZERO_DIE);
  case 'a': return checkKeyword(2, 9, "ccumulate", TOKEN_ACCUMULATE);
  case 'c':
    if (scanner.current - scanner.start > 1) {
      switch (scanner.start[1]) {
      case 'a': return checkKeyword(2, 2, "ll", TOKEN_CALL);
      case 'h': return checkKeyword(2, 4, "oose", TOKEN_CHOOSE);
      case 'o':
        if (scanner.current - scanner.start > 2) {
          switch (scanner.start[2]) {
          case 'm': return checkKeyword(3, 10, "positional", TOKEN_COMPOSITIONAL);
          case 'u': return checkKeyword(3, 2, "nt", TOKEN_COUNT);
          }
        }
        break;
      }
    }
    break;
  case 'd':
    if (scanner.current - scanner.start > 1) {
      switch (scanner.start[1]) {
      case 'i': return checkKeyword(2, 7, "fferent", TOKEN_DIFFERENT);
      case 'o': return checkKeyword(2, 0, "", TOKEN_DO);
      case 'r': return checkKeyword(2, 2, "op", TOKEN_DROP);
      }
    } else {
      return TOKEN_DIE;
    }
  case 'e': return checkKeyword(1, 3, "lse", TOKEN_ELSE);
  case 'f':
    if (scanner.current - scanner.start > 1) {
      switch (scanner.start[1]) {
      case 'o': return checkKeyword(2, 5, "reach", TOKEN_FOREACH);
      case 'u': return checkKeyword(2, 6, "nction", TOKEN_FUNCTION);
      }
    }
    break;
  case 'i':
    if (scanner.current - scanner.start > 1) {
      switch (scanner.start[1]) {
      case 'f': return checkKeyword(2, 0, "", TOKEN_IF);
      case 'n': return checkKeyword(2, 0, "", TOKEN_IN);
      }
    }
    break;
  case 'k': return checkKeyword(1, 3, "eep", TOKEN_KEEP);
  case 'l':
    if (scanner.current - scanner.start > 1) {
      switch (scanner.start[1]) {
      case 'a': return checkKeyword(2, 5, "rgest", TOKEN_LARGEST);
      case 'e': return checkKeyword(2, 3, "ast", TOKEN_LEAST);
      }
    }
    break;
  case 'm':
    if (scanner.current - scanner.start > 1) {
      switch (scanner.start[1]) {
      case 'a':
        if (scanner.current - scanner.start > 2 && scanner.start[2] == 'x') {
          if (scanner.current - scanner.start == 3) {
            return TOKEN_MAX;
          } else {
            return checkKeyword(3, 4, "imal", TOKEN_MAXIMAL);
          }
        }
        break;
      case 'e': return checkKeyword(2, 4, "dian", TOKEN_MEDIAN);
      case 'i':
        if (scanner.current - scanner.start > 2 && scanner.start[2] == 'n') {
          if (scanner.current - scanner.start == 3) {
            return TOKEN_MIN;
          } else {
            return checkKeyword(3, 4, "imal", TOKEN_MINIMAL);
          }
        }
        break;
      case 'o': return checkKeyword(2, 1, "d", TOKEN_MOD);
      }
    }
    break;
  case 'p': return checkKeyword(1, 3, "ick", TOKEN_PICK);
  case 'r': return checkKeyword(1, 5, "epeat", TOKEN_REPEAT);
  case 's':
    if (scanner.current - scanner.start > 1) {
      switch (scanner.start[1]) {
      case 'g': return checkKeyword(2, 1, "n", TOKEN_SGN);
      case 'u': return checkKeyword(2, 1, "m", TOKEN_SUM);
      }
    }
    break;
  case 't': return checkKeyword(1, 3, "hen", TOKEN_THEN);
  case 'u': return checkKeyword(1, 4, "ntil", TOKEN_UNTIL);
  case 'w': return checkKeyword(1, 4, "hile", TOKEN_WHILE);
  case 'z': return checkKeyword(1, 0, "", TOKEN_ZERO_DIE);
  }
  
  return TOKEN_IDENTIFIER;
}

static TokenType checkKeyword(int start, int length, const char* rest, TokenType type) {
  if (scanner.current - scanner.start == start + length
      && memcmp(scanner.start + start, rest, length) == 0) {
    return type;
  }
  
  return TOKEN_IDENTIFIER;
}

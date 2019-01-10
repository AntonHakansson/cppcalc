#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>
#include <stdlib.h>

#define PI 3.141592653589
#define ArrayCount(array) (sizeof((array))/sizeof((array)[0]))

#ifdef ENABLE_ASSERTIONS
#define ASSERT(expr) assert(expr)
#else
#define ASSERT(expr)
#endif

typedef int8_t u8;
typedef double r64;
typedef uint32_t bool32;

#include "profiling.h"

// NOTE(Hakan): OperatorType, precedence, isRightAssociative
#define LIST_OPERATORS                          \
  HANDLE_OPERATOR(Add, 2, 0)                    \
  HANDLE_OPERATOR(Sub, 2, 0)                    \
  HANDLE_OPERATOR(Mul, 3, 0)                    \
  HANDLE_OPERATOR(Div, 3, 0)                    \
  HANDLE_OPERATOR(Pow, 4, 1)                    \
  HANDLE_OPERATOR(Sin, 5, 0)                    \
  HANDLE_OPERATOR(Cos, 5, 0)                    \
  HANDLE_OPERATOR(Tan, 5, 0)                    \
  HANDLE_OPERATOR(Max, 5, 0)                    \
  HANDLE_OPERATOR(Min, 5, 0)

#define HANDLE_OPERATOR(type, precedence, associativity) Token_Op ## type,

enum TokenType {
  Token_Unknown,

  Token_OpStart,
  LIST_OPERATORS
  Token_OpEnd,

  Token_OpenParen,
  Token_CloseParen,
  Token_Comma,

  Token_Number,
  Token_Identifier,

  Token_EndOfStream,
  Tokens_Count,
};
#undef HANDLE_OPERATOR

struct Token {
  union {
    r64 number;
    struct {
      size_t textLength;
      char *text;
    };
  };
  TokenType type;
};

struct Tokenizer {
  char *at;
  Token previousToken;
};

struct Operator {
  size_t precedence;
  bool32 isRightAssociative;
};

#define HANDLE_OPERATOR(type, precedence, associativity) {precedence, associativity},
static const Operator operatorLookup[] = {
  LIST_OPERATORS
};
#undef HANDLE_OPERATOR


inline bool32 isEndOfLine(char c) {
  return (bool32) (c == '\n' || c == '\r');
}

inline bool32 isWhitespace(char c) {
  return (bool32) (c == ' ' || c == '\t' || c == '\v' || c == '\f' || isEndOfLine(c));
}

inline bool32 isAlpha(char c) {
  return (bool32) ((c >= 'a' && c <= 'z') ||
                   (c >= 'A' && c <= 'Z'));
}

inline bool32 isDigit(char c) {
  return (bool32) (c >= '0' && c <= '9');
}

static void eatAllWhitespace(Tokenizer *tokenizer) {
  while (isWhitespace(tokenizer->at[0])) {
    ++tokenizer->at;
  }
}

static bool32 tokenEquals(Token token, const char *text) {
  char *at = const_cast<char*>(text);
  for (size_t index = 0;
       index < token.textLength;
       ++index, ++at) {
    if (*at == '\0' ||
        token.text[index] == '\0' ||
        *at != token.text[index]) {
      return 0;
    }
  }

  return (bool32) (*at == '\0');
}

static inline r64 numberTokenToValue(Token *token) {
  ASSERT(token->type == Token_Number);

  constexpr unsigned long long pow10[] = {
    1, 10, 100,
    1000, 10000, 100000,
    1000000, 10000000, 100000000,
    1000000000, 10000000000, 100000000000,
    1000000000000, 10000000000000, 100000000000000,
    1000000000000000, 10000000000000000, 100000000000000000,
    1000000000000000000, 10000000000000000000UL,
  };

  bool32 isNegative = 0;
  unsigned long long mantisa = 0;
  size_t numDigitsAfterDecimalPoint = 0;

  for (size_t textIndex = 0; textIndex < token->textLength; textIndex++) {
    char c = token->text[textIndex];
    if(isDigit(c)){
      size_t exp = token->textLength - textIndex - 1;
      ASSERT(exp <= ArrayCount(pow10));
      mantisa += (c - '0') * pow10[exp];
    }
    else if (c == '.') {
      numDigitsAfterDecimalPoint = token->textLength - textIndex - 1;
      ASSERT(numDigitsAfterDecimalPoint <= ArrayCount(pow10));
      mantisa /= 10;
    }
    else if(c == '-') {
      isNegative = true;
    }
    else if (c == '+') {
      continue;
    }
    else {
      // TODO(Hakan): Handle unexpected character
      printf("Unexpected character \'%c\' in number token %.*s\n", c, (int)token->textLength, token->text);
    }
  }

  r64 result = (r64)mantisa / (r64)pow10[numDigitsAfterDecimalPoint];
  if (isNegative) {
    result = -result;
  }

  return result;
}

static Token getToken(Tokenizer *tokenizer) {
  Token result = {};

  eatAllWhitespace(tokenizer);
  result.text = tokenizer->at;
  result.textLength = 1;

  char c = tokenizer->at[0];
  ++tokenizer->at;

  switch (c) {
    case '\0': {result.type = Token_EndOfStream;} break;

    case '(': {result.type = Token_OpenParen;} break;
    case ')': {result.type = Token_CloseParen;} break;
    case ',': {result.type = Token_Comma;} break;

    case '+': {result.type = Token_OpAdd;} break;
    case '-': {
      if ((tokenizer->previousToken.textLength == 0) && (tokenizer->previousToken.type == Token_Unknown)) {
        goto UnarySign;
      }
      else if (((tokenizer->previousToken.type < Token_OpEnd) && (tokenizer->previousToken.type > Token_OpStart)) ||
               (tokenizer->previousToken.type == Token_OpenParen) ||
               (tokenizer->previousToken.type == Token_Comma)) {
        goto UnarySign;
      }
      else {
        result.type = Token_OpSub;
      }
    } break;
    case '*': {result.type = Token_OpMul;} break;
    case '/': {result.type = Token_OpDiv;} break;
    case '^': {result.type = Token_OpPow;} break;

    // NOTE(Hakan): either a string or number
    default: {
      if (isAlpha(c)) {
        result.type = Token_Identifier;

        while (isAlpha(tokenizer->at[0]) ||
               isDigit(tokenizer->at[0]) ||
               tokenizer->at[0] == '_') {
          ++tokenizer->at;
        }

        result.textLength = (uint32_t)(tokenizer->at - result.text);

        if (tokenEquals(result, "pi")) {
          result.number = PI;
        }
        else if (tokenEquals(result, "sin")) {
          result.type = Token_OpSin;
        }
        else if (tokenEquals(result, "cos")) {
          result.type = Token_OpCos;
        }
        else if (tokenEquals(result, "tan")) {
          result.type = Token_OpTan;
        }
        else if (tokenEquals(result, "max")) {
          result.type = Token_OpMax;
        }
        else if (tokenEquals(result, "min")) {
          result.type = Token_OpMin;
        }
        else {
          break;
        }
      }
      else if(isDigit(c) || c == '-' || c == '.') {
UnarySign:
        result.type = Token_Number;

        while (isDigit(tokenizer->at[0]) ||
               tokenizer->at[0] == '.') {
          ++tokenizer->at;
        }

        result.textLength = tokenizer->at - result.text;
        result.number = numberTokenToValue(&result);
      }
      else {
        result.type = Token_Unknown;
      }
    } break;
  }

  tokenizer->previousToken = result;
  return result;
}

struct ListOfTokens {
  Token *tokens;
  size_t count;
};

ListOfTokens cStringToRTN(Tokenizer *tokenizer) {
  // NOTE(Hakan): The number of tokens can never exceed the length of the math
  // expression string
  // const size_t tokensMaxCount = strlen(tokenizer->at);
  const size_t tokensMaxCount = strlen(tokenizer->at);

  ListOfTokens result = {};
  result.tokens = (Token*)malloc(sizeof(Token) * tokensMaxCount);

  Token *operatorStack = (Token*)alloca(sizeof(Token) * tokensMaxCount);
  size_t operatorStackCount = 0;

  bool32 isParsing = true;
  while (isParsing) {
    Token token = getToken(tokenizer);
    switch (token.type) {
      case Token_EndOfStream: {
        while (operatorStackCount != 0) {
          result.tokens[result.count] = operatorStack[operatorStackCount - 1];
          result.count++;
          operatorStackCount--;
        }
        isParsing = false;
      } break;

      case Token_OpenParen: {
        operatorStack[operatorStackCount] = token;
        operatorStackCount++;
      } break;

      case Token_CloseParen: {
        while ((operatorStack[operatorStackCount - 1].type != Token_OpenParen)) {
          result.tokens[result.count] = operatorStack[operatorStackCount - 1];
          result.count++;
          operatorStackCount--;
        }
        // pop open paranthesis from operator stack
        operatorStackCount--;
      } break;

      case Token_OpAdd:
      case Token_OpSub:
      case Token_OpMul:
      case Token_OpDiv:
      case Token_OpPow:
      case Token_OpSin:
      case Token_OpCos:
      case Token_OpTan:
      case Token_OpMax:
      case Token_OpMin: {
        while (operatorStackCount > 0) {
          Token *topOp = &operatorStack[operatorStackCount - 1];
          bool32 isRightAssociative = operatorLookup[token.type - Token_OpStart].isRightAssociative;
          size_t opPrecedence = operatorLookup[token.type - Token_OpStart].precedence;
          size_t topOpPrecedence = operatorLookup[topOp->type - Token_OpStart].precedence;

          if (((topOpPrecedence > opPrecedence) || ((topOpPrecedence == opPrecedence) && (!isRightAssociative))) &&
               (topOp->type != Token_OpenParen)) {
            result.tokens[result.count] = *topOp;
            result.count++;
            operatorStackCount--;
          }
          else {
            break;
          }
        }

        operatorStack[operatorStackCount] = token;
        operatorStackCount++;
      } break;

      case Token_Identifier:
      case Token_Number: {
        result.tokens[result.count] = token;
        result.count++;
      } break;
    }
  }

#if 0
  fputs("RTN: ", stdout);
  for (size_t tokenIndex = 0; tokenIndex < result.count; tokenIndex++) {
    Token token = result.tokens[tokenIndex];
    fwrite(token.text, sizeof(char), token.textLength, stdout);
  }
  putc('\n', stdout);
#endif

  return result;
}

r64 evalExpression(Tokenizer *tokenizer) {
  ListOfTokens rtn = cStringToRTN(tokenizer);

  // NOTE(Hakan): The size of resultstack can never grow larger than the rtn
  // representaion
  r64 *resultStack = (r64*)alloca(sizeof(r64) * rtn.count);
  size_t resultStackCount = 0;

  for (size_t tokenIndex = 0; tokenIndex < rtn.count; tokenIndex++) {
    Token token = rtn.tokens[tokenIndex];
    switch (token.type) {
      case Token_OpAdd: {
        r64 operandB = resultStack[resultStackCount - 1];
        r64 operandA = resultStack[resultStackCount - 2];
        resultStack[resultStackCount - 2] = operandA + operandB;
        resultStackCount--;
      } break;
      case Token_OpSub:{
        r64 operandB = resultStack[resultStackCount - 1];
        r64 operandA = resultStack[resultStackCount - 2];
        resultStack[resultStackCount - 2] = operandA - operandB;
        resultStackCount--;
      } break;
      case Token_OpMul:{
        r64 operandB = resultStack[resultStackCount - 1];
        r64 operandA = resultStack[resultStackCount - 2];
        resultStack[resultStackCount - 2] = operandA * operandB;
        resultStackCount--;
      } break;
      case Token_OpDiv: {
        r64 operandB = resultStack[resultStackCount - 1];
        r64 operandA = resultStack[resultStackCount - 2];
        resultStack[resultStackCount - 2] = operandA / operandB;
        resultStackCount--;
      } break;
      case Token_OpPow: {
        r64 operandB = resultStack[resultStackCount - 1];
        r64 operandA = resultStack[resultStackCount - 2];
        resultStack[resultStackCount - 2] = pow(operandA, operandB);
        resultStackCount--;
      } break;
      case Token_OpSin: {
        r64 operand = resultStack[resultStackCount - 1];
        resultStack[resultStackCount - 1] = sin(operand);
      } break;
      case Token_OpCos: {
        r64 operand = resultStack[resultStackCount - 1];
        resultStack[resultStackCount - 1] = cos(operand);
      } break;
      case Token_OpTan: {
        r64 operand = resultStack[resultStackCount - 1];
        resultStack[resultStackCount - 1] = tan(operand);
      } break;
      case Token_OpMax: {
        r64 operandB = resultStack[resultStackCount - 1];
        r64 operandA = resultStack[resultStackCount - 2];
        resultStack[resultStackCount - 2] = (operandA > operandB) ? operandA : operandB;
        resultStackCount--;
      } break;
      case Token_OpMin: {
        r64 operandB = resultStack[resultStackCount - 1];
        r64 operandA = resultStack[resultStackCount - 2];
        resultStack[resultStackCount - 2] = (operandA < operandB) ? operandA : operandB;
        resultStackCount--;
      } break;

      case Token_Identifier:
      case Token_Number: {
        resultStack[resultStackCount] = token.number;
        resultStackCount++;
      } break;
    }
  }

  free(rtn.tokens);
  return resultStack[resultStackCount - 1];
}

#ifndef TEST
int main(int numArguments, char** arguments) {
  if (numArguments < 2) {
    printf("Usage: calc.exe expr");
    return 0;
  }

  const char *expr = arguments[numArguments - 1];

  Tokenizer tokenizer = {};
  tokenizer.at = const_cast<char*>(expr);

  r64 result = evalExpression(&tokenizer);
  printf("%f\n", result);

  return 0;
}
#endif

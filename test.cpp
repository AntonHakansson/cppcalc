#define TEST
#include "calc.cpp"

#include <stdlib.h>
#include <time.h>

struct StringBuilder {
  char *at;
  char text[4*1028];
};

inline static double getRandPrintFriendlyNumber(double fMin, double fMax) {
  double f = (double)rand() / RAND_MAX;
  double rand = fMin + f * (fMax - fMin);
  char output[50];
  snprintf(output, 50, "%f", rand);
  return strtod(output, nullptr);
}

inline static void stringBuilderPut(StringBuilder *expr, char character) {
  *expr->at = character;
  expr->at++;
}

inline static void stringBuilderPuts(StringBuilder *expr, char *str) {
  while (*str != '\0') {
    *expr->at = *str;
    expr->at++;
    str++;
  }
}

static void putNumberToken(StringBuilder *expr, r64 number) {
  char output[50];
  snprintf(output, 50, "%f", number);

  int numberLength = (int)strlen(output);
  // stringBuilderPut(expr, '(');
  for (int i = 0; i < numberLength; i++) {
    stringBuilderPut(expr, output[i]);
  }
  // stringBuilderPut(expr, ')');
}

static inline r64 insertGeneratedExpr(StringBuilder *expr);

static r64 insertOperatorAndNullTerminate(StringBuilder *out) {
  r64 result = 0;
  bool32 isOperandNumber = (rand() % 3) < 2;
  if (isOperandNumber) {
    result = getRandPrintFriendlyNumber(-99999.0, 99999.0);
    putNumberToken(out, result);
  }
  else {
    stringBuilderPut(out, '(');
    result = insertGeneratedExpr(out);
    stringBuilderPut(out, ')');
  }
  stringBuilderPut(out, '\0');

  return result;
}

static inline r64 insertGeneratedExpr(StringBuilder *expr) {
  r64 result = 0;
  r64 leftOperandResult = 0;
  r64 rightOperandResult = 0;

  StringBuilder leftOperand = {};
  leftOperand.at = leftOperand.text;

  StringBuilder rightOperand = {};
  rightOperand.at = rightOperand.text;

  leftOperandResult = insertOperatorAndNullTerminate(&leftOperand);
  rightOperandResult = insertOperatorAndNullTerminate(&rightOperand);

  TokenType op = (TokenType)(rand() % (Token_OpEnd - Token_OpStart));
  switch(op) {
    case Token_OpAdd: {
      stringBuilderPuts(expr, leftOperand.text);
      stringBuilderPut(expr, '+');
      stringBuilderPuts(expr, rightOperand.text);

      result = leftOperandResult + rightOperandResult;
    } break;
    case Token_OpSub: {
      stringBuilderPuts(expr, leftOperand.text);
      stringBuilderPut(expr, '-');
      stringBuilderPuts(expr, rightOperand.text);

      result = leftOperandResult - rightOperandResult;
    } break;
    case Token_OpMul: {
      stringBuilderPuts(expr, leftOperand.text);
      stringBuilderPut(expr, '*');
      stringBuilderPuts(expr, rightOperand.text);

      result = leftOperandResult * rightOperandResult;
    } break;
    case Token_OpDiv: {
      stringBuilderPuts(expr, leftOperand.text);
      stringBuilderPut(expr, '/');
      stringBuilderPuts(expr, rightOperand.text);

      result = leftOperandResult / rightOperandResult;
    } break;
    case Token_OpPow: {
      stringBuilderPuts(expr, leftOperand.text);
      stringBuilderPut(expr, '^');
      stringBuilderPuts(expr, rightOperand.text);

      result = pow(leftOperandResult, rightOperandResult);
    } break;
    case Token_OpSin: {
      stringBuilderPuts(expr, "sin(");
      stringBuilderPuts(expr, leftOperand.text);
      stringBuilderPut(expr, ')');

      result = sin(leftOperandResult);
    } break;
    case Token_OpCos: {
      stringBuilderPuts(expr, "cos(");
      stringBuilderPuts(expr, leftOperand.text);
      stringBuilderPut(expr, ')');

      result = cos(leftOperandResult);
    } break;
    case Token_OpTan: {
      stringBuilderPuts(expr, "tan(");
      stringBuilderPuts(expr, leftOperand.text);
      stringBuilderPut(expr, ')');

      result = tan(leftOperandResult);
    } break;
    case Token_OpMax: {
      stringBuilderPuts(expr, "max(");
      stringBuilderPuts(expr, leftOperand.text);
      stringBuilderPut(expr, ',');
      stringBuilderPuts(expr, rightOperand.text);
      stringBuilderPut(expr, ')');

      result = (leftOperandResult > rightOperandResult) ? leftOperandResult : rightOperandResult;
    } break;
    case Token_OpMin: {
      stringBuilderPuts(expr, "min(");
      stringBuilderPuts(expr, leftOperand.text);
      stringBuilderPut(expr, ',');
      stringBuilderPuts(expr, rightOperand.text);
      stringBuilderPut(expr, ')');

      result = (leftOperandResult <= rightOperandResult) ? leftOperandResult : rightOperandResult;
    } break;

    default: {
      stringBuilderPuts(expr, leftOperand.text);
      stringBuilderPut(expr, '+');
      stringBuilderPuts(expr, rightOperand.text);

      result = leftOperandResult + rightOperandResult;
    } break;
  }

  return result;
}

int main(int numArguments, char** arguments) {
  START_TIMEDBLOCK("test");
  srand((unsigned int)time(nullptr));

#define TEST_ExprEval 1
// #define TEST_StringToDouble 1

#if TEST_ExprEval
  {
    const int testSamples = 1000000;
    int numberFailedTests = 0;
    TimeUnit avgClockCycles = 0;

    puts("################################");
    puts("# Testing math expression eval #");
    puts("################################");
    for (int i = 0; i < testSamples; i++) {
      Tokenizer tokenizer = {};
      StringBuilder stringBuilder = {};
      stringBuilder.at = stringBuilder.text;

      r64 correctResult = insertGeneratedExpr(&stringBuilder);
      stringBuilderPut(&stringBuilder, '\0');

      tokenizer.at = stringBuilder.text;

      START_TIMEDBLOCK("EVAL");
      r64 result = evalExpression(&tokenizer);
      avgClockCycles += GET_TIMEDBLOCK("EVAL");

      if ((result - correctResult) > 0.1) {
        fputs(stringBuilder.text, stdout);
        printf(" = %f = %f\n", result, correctResult);
        numberFailedTests++;
      }
    }

    printf("## Average clock pulses %f\n", (r64)avgClockCycles / (r64)testSamples);
    const int succeddedTests = (testSamples - numberFailedTests);
    printf("## %d/%d(%.1f%%) test succeeded\n\n", succeddedTests, testSamples, 100.0*((r64)succeddedTests/(r64)testSamples));
  }
#endif

#if TEST_StringToDouble
  {
    const int testSamples = 1000000;
    int successfulTestsCount = 0;

    Token token;
    token.type = Token_Number;

    TimeUnit myAvgClockCycles = 0;
    TimeUnit stdAvgClockCycles = 0;

    puts("################################");
    puts("### Testing string to double ###");
    puts("################################");
    for (int i = 0; i < testSamples; i++) {
      StringBuilder stringBuilder = {};
      stringBuilder.at = stringBuilder.text;

      r64 number = getRandPrintFriendlyNumber(-99999.0, 99999.0);
      putNumberToken(&stringBuilder, number);
      stringBuilderPut(&stringBuilder, '\0');

      token.text = stringBuilder.text;
      token.textLength = strlen(stringBuilder.text);

      START_TIMEDBLOCK("my ");
      r64 result = numberTokenToValue(&token);
      myAvgClockCycles += GET_TIMEDBLOCK("my ");

      START_TIMEDBLOCK("std");
      r64 correctResult = atof(token.text);
      stdAvgClockCycles += GET_TIMEDBLOCK("std");

      if (result != correctResult) {
        printf("Failed to evaluate number %f != %f\n", result, correctResult);
      }
      else {
        successfulTestsCount++;
      }
    }

    printf("## Average clock cycles my: %f, std: %f\n", myAvgClockCycles / (r64)testSamples, stdAvgClockCycles / (r64)testSamples);
    printf("## %d/%d(%.1f%%) tests succeeded\n\n", successfulTestsCount, testSamples, 100*((r64)successfulTestsCount / (r64)testSamples));
  }
#endif

  DEBUG_TIMEDBLOCK("test");
  return 0;
}

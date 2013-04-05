#include <assert.h>
#include <stdio.h>

#include "stackvm.h"
#include "regvm.h"

/*
   Simple recursive fibonacci implementation, roughly equivalent to:

   int fibonacci(int arg)
   {
      if (arg < 2) {
          return arg
      }
      return fibonacci(arg-1) + fibonacci(arg-2)
   }
 */
using namespace stackvm;
const char fibonacci[] = {
  // stack: [arg]

  // 0:
  DUP,
  // stack: [arg, arg]

  // 1:
  PUSH_INT_CONST, 2,
  // stack: [arg, arg, 2]

  // 3:
  BINARY_INT_COMPARE_LT,
  // stack: [arg, (arg < 2)]

  // 4:
  JUMP_ABS_IF_TRUE, 17,
  // stack: [arg]

  // 6:
  DUP,
  // stack: [arg, arg]

  // 7:
  PUSH_INT_CONST,  1,
  // stack: [arg, arg, 1]

  // 9:
  BINARY_INT_SUBTRACT,
  // stack: [arg,  (arg - 1)

  // 10:
  CALL_INT,
  // stack: [arg, fib(arg - 1)]

  // 11:
  ROT,
  // stack: [fib(arg - 1), arg]

  // 12:
  PUSH_INT_CONST,  2,
  // stack: [fib(arg - 1), arg, 2]

  // 14:
  BINARY_INT_SUBTRACT,
  // stack: [fib(arg - 1), arg,  (arg - 2)

  // 15:
  CALL_INT,
  // stack: [fib(arg - 1), fib(arg - 1)]

  // 16:
  BINARY_INT_ADD,
  // stack: [fib(arg - 1) + fib(arg - 1)]

  // 17:
  RETURN_INT
};

int main(int argc, const char **argv)
{
  stackvm::bytecode * scode = new stackvm::bytecode(fibonacci,
                                                    sizeof(fibonacci));
  scode->disassemble(stdout);

  stackvm::vm *sv = new stackvm::vm(scode);
  //printf("sv->interpret(8) = %i\n", sv->interpret(8));

  regvm::wordcode * regcode = scode->compile_to_regvm();
  regcode->disassemble(stdout);

  regvm::vm *rv = new regvm::vm(regcode);
  printf("rv->interpret(8) = %i\n", rv->interpret(8));
}

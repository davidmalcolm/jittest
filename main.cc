/*
   Copyright 2013 David Malcolm <dmalcolm@redhat.com>
   Copyright 2013 Red Hat, Inc.

   This is free software: you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see
   <http://www.gnu.org/licenses/>.
*/

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

typedef int (*compiled_code) (int);

int main(int argc, const char **argv)
{
  stackvm::bytecode * scode = new stackvm::bytecode(fibonacci,
                                                    sizeof(fibonacci));
  scode->disassemble(stdout);

  stackvm::vm *sv = new stackvm::vm(scode);
  printf("sv->interpret(8) = %i\n", sv->interpret(8));

  regvm::wordcode * regcode = scode->compile_to_regvm();
  regcode->disassemble(stdout);

  regvm::vm *rv = new regvm::vm(regcode);
  printf("rv->interpret(8) = %i\n", rv->interpret(8));

  compiled_code code = (compiled_code)regcode->compile();
  printf("code (8) = %i\n", code (8));
}

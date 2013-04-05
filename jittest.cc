#include <assert.h>
#include <stdio.h>

enum opcode {
  DUP,
  ROT,
  PUSH_INT_CONST,
  BINARY_INT_ADD,
  BINARY_INT_SUBTRACT,
  BINARY_INT_COMPARE_LT,
  JUMP_ABS_IF_TRUE,
  CALL_INT,
  RETURN_INT,
};

class bytecode
{
public:
  bytecode(const char *bytes, int len)
    : m_bytes(bytes),
      m_len(len)
  {}

  void disassemble(FILE *out) const;

  void disassemble_at(FILE *out, int &pc) const;

  enum opcode
  fetch_opcode(int &pc) const;

  int
  fetch_arg_int(int &pc) const;

private:
  const char *m_bytes;
  int m_len;
};

class toy_vm
{
public:
  toy_vm(bytecode *code)
    : m_bytecode(code),
      m_depth(0)
  {}
  ~toy_vm() {}

  int interpret(int arg);
  void *compile();

private:
  int pop_int();
  void push_int(int);

  bool pop_bool() { return pop_int() != 0; }
  void push_bool(bool flag) { push_int(flag ? 1 : 0); }

  void debug_begin_frame(int arg);
  void debug_end_frame(int pc, int result);
  void debug_begin_opcode(int pc);
  void debug_end_opcode(int pc);

private:
  bytecode *m_bytecode;
  int m_stack[16];
  int m_depth;
};

void bytecode::disassemble(FILE *out) const
{
  int pc = 0;
  while (pc < m_len) {
    disassemble_at(out, pc);
  }
}

void bytecode::disassemble_at(FILE *out, int &pc) const
{
    fprintf(out, "[%i] : ", pc);
    enum opcode op = fetch_opcode(pc);
    switch (op) {
      case DUP:
        {
          fprintf(out, "DUP\n");
        }
        break;

      case ROT:
        {
          fprintf(out, "ROT\n");
        }
        break;

      case PUSH_INT_CONST:
        {
          fprintf(out, "PUSH_INT_CONST %i\n", fetch_arg_int(pc));
        }
        break;

      case BINARY_INT_ADD:
        {
          fprintf(out, "BINARY_INT_ADD\n");
        }
        break;

      case BINARY_INT_SUBTRACT:
        {
          fprintf(out, "BINARY_INT_SUBTRACT\n");
        }
        break;

      case BINARY_INT_COMPARE_LT:
        {
          fprintf(out, "BINARY_INT_COMPARE_LT\n");
        }
        break;

      case JUMP_ABS_IF_TRUE:
        {
          fprintf(out, "JUMP_ABS_IF_TRUE %i\n", fetch_arg_int(pc));
        }
        break;

      case CALL_INT:
        {
          fprintf(out, "CALL_INT\n");
        }
        break;

      case RETURN_INT:
        {
          fprintf(out, "RETURN_INT\n");
        }
        break;

      default:
        assert(0); // FIXME
    }
}

enum opcode
bytecode::fetch_opcode(int &pc) const
{
  return static_cast<enum opcode>(m_bytes[pc++]);
}

int
bytecode::fetch_arg_int(int &pc) const
{
  return static_cast<int>(m_bytes[pc++]);
}

int toy_vm::interpret(int input)
{
  int pc = 0;
  debug_begin_frame(input);
  push_int(input);
  while (1) {
    debug_begin_opcode(pc);
    enum opcode op = m_bytecode->fetch_opcode(pc);
    switch (op) {
      case DUP:
        {
          int top = pop_int();
          push_int(top);
          push_int(top);
        }
        break;

      case ROT:
        {
          int first = pop_int();
          int second = pop_int();
          push_int(first);
          push_int(second);
        }
        break;

      case PUSH_INT_CONST:
        {
          push_int(m_bytecode->fetch_arg_int(pc));
        }
        break;

      case BINARY_INT_ADD:
        {
          int rhs = pop_int();
          int lhs = pop_int();
          int result = lhs + rhs;
          push_int(result);
        }
        break;

      case BINARY_INT_SUBTRACT:
        {
          int rhs = pop_int();
          int lhs = pop_int();
          int result = lhs - rhs;
          push_int(result);
        }
        break;

      case BINARY_INT_COMPARE_LT:
        {
          int rhs = pop_int();
          int lhs = pop_int();
          bool result = lhs < rhs;
          push_bool(result);
        }
        break;

      case JUMP_ABS_IF_TRUE:
        {
          bool flag = pop_bool();
          int dest = m_bytecode->fetch_arg_int(pc);
          if (flag) {
            pc = dest;
          }
        }
        break;

      case CALL_INT:
        {
          int arg = pop_int();
          int result = interpret(arg); //recurse
          push_int(result);
        }
        break;

      case RETURN_INT:
        {
          int result = pop_int();
          debug_end_frame(pc, result);
          return result;
        }

      default:
        assert(0); // FIXME
      }
    debug_end_opcode(pc);
  }
}

int toy_vm::pop_int()
{
  return m_stack[--m_depth];
}

void toy_vm::push_int(int val)
{
  assert(m_depth < 16);
  m_stack[m_depth++] = val;
}

void toy_vm::debug_begin_frame(int arg)
{
  printf("begin frame: arg=%i\n", arg);
}

void toy_vm::debug_end_frame(int pc, int result)
{
  printf("end frame: result=%i\n", result);
  m_bytecode->disassemble_at(stdout, pc);
}

void toy_vm::debug_begin_opcode(int pc)
{
  printf("begin opcode: ");
  m_bytecode->disassemble_at(stdout, pc);
  printf("  stack: \n");
  for (int i = 0; i < m_depth; i++) {
    printf("    depth %i: %i\n", i, m_stack[i]);
  }
}

void toy_vm::debug_end_opcode(int pc)
{
}


void *toy_vm::compile()
{
}

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
  bytecode * code = new bytecode(fibonacci, sizeof(fibonacci));
  code->disassemble(stdout);

  toy_vm *vm = new toy_vm(code);
  printf("vm->interpret(8) = %i\n", vm->interpret(8));

  // FIXME: compile

}

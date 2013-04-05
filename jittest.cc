#include <assert.h>
#include <stdio.h>

// A simple stack-based virtual machine
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

class frame
{
public:
  frame()
    : m_depth(0)
  {}

  int pop_int();
  void push_int(int);

  bool pop_bool() { return pop_int() != 0; }
  void push_bool(bool flag) { push_int(flag ? 1 : 0); }

  void debug_stack(FILE *out) const;

private:
  int m_stack[16];
  int m_depth;
};

class toy_vm
{
public:
  toy_vm(bytecode *code)
    : m_bytecode(code)
  {}
  ~toy_vm() {}

  int interpret(int arg);
  void *compile();

private:
  void debug_begin_frame(int arg);
  void debug_end_frame(int pc, int result);
  void debug_begin_opcode(const frame &f, int pc);
  void debug_end_opcode(int pc);

private:
  bytecode *m_bytecode;
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
  frame f;
  int pc = 0;
  debug_begin_frame(input);
  f.push_int(input);
  while (1) {
    debug_begin_opcode(f, pc);
    enum opcode op = m_bytecode->fetch_opcode(pc);
    switch (op) {
      case DUP:
        {
          int top = f.pop_int();
          f.push_int(top);
          f.push_int(top);
        }
        break;

      case ROT:
        {
          int first = f.pop_int();
          int second = f.pop_int();
          f.push_int(first);
          f.push_int(second);
        }
        break;

      case PUSH_INT_CONST:
        {
          f.push_int(m_bytecode->fetch_arg_int(pc));
        }
        break;

      case BINARY_INT_ADD:
        {
          int rhs = f.pop_int();
          int lhs = f.pop_int();
          int result = lhs + rhs;
          f.push_int(result);
        }
        break;

      case BINARY_INT_SUBTRACT:
        {
          int rhs = f.pop_int();
          int lhs = f.pop_int();
          int result = lhs - rhs;
          f.push_int(result);
        }
        break;

      case BINARY_INT_COMPARE_LT:
        {
          int rhs = f.pop_int();
          int lhs = f.pop_int();
          bool result = lhs < rhs;
          f.push_bool(result);
        }
        break;

      case JUMP_ABS_IF_TRUE:
        {
          bool flag = f.pop_bool();
          int dest = m_bytecode->fetch_arg_int(pc);
          if (flag) {
            pc = dest;
          }
        }
        break;

      case CALL_INT:
        {
          int arg = f.pop_int();
          int result = interpret(arg); //recurse
          f.push_int(result);
        }
        break;

      case RETURN_INT:
        {
          int result = f.pop_int();
          debug_end_frame(pc, result);
          return result;
        }

      default:
        assert(0); // FIXME
      }
    debug_end_opcode(pc);
  }
}

int frame::pop_int()
{
  return m_stack[--m_depth];
}

void frame::push_int(int val)
{
  assert(m_depth < 16);
  m_stack[m_depth++] = val;
}

void frame::debug_stack(FILE *out) const
{
  for (int i = 0; i < m_depth; i++) {
    fprintf(out, "    depth %i: %i\n", i, m_stack[i]);
  }
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

void toy_vm::debug_begin_opcode(const frame &f, int pc)
{
  printf("begin opcode: ");
  m_bytecode->disassemble_at(stdout, pc);
  printf("  stack: \n");
  f.debug_stack(stdout);
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

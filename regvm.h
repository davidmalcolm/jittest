#include <vector>

namespace regvm {

const int NUM_REGISTERS = 4;

// A simple register-based virtual machine
enum addrmode {
  CONSTANT,
  REGISTER,
};

enum opcode {
  COPY_INT,
  BINARY_INT_ADD,
  BINARY_INT_SUBTRACT,
  BINARY_INT_COMPARE_LT,
  JUMP_ABS_IF_TRUE,
  CALL_INT,
  RETURN_INT,

  NUM_OPCODES,
};

struct input
{
  input(enum addrmode addrmode, int value)
    : m_addrmode(addrmode),
      m_value(value)
  {}

  enum addrmode m_addrmode;
  int m_value;
};

struct instr
{
  instr(enum opcode op, int output_reg, input a);

  instr(enum opcode op, int output_reg, input lhs, input rhs);

  void disassemble(FILE *out) const;

  enum opcode m_op;
  int m_output_reg;
  input m_inputA;
  input m_inputB;
};

class wordcode
{
public:
  wordcode(std::vector<instr> instrs)
    : m_instrs(instrs)
  {}

  void disassemble(FILE *out) const;

  void disassemble_at(FILE *out, int &pc) const;

  const instr&
  fetch_instr(int &pc) const;

  int
  fetch_arg_int(int &pc) const;

private:
  std::vector<instr> m_instrs;
};

class frame
{
public:
  frame();

  int eval_int(const input& in) const;
  bool eval_bool(const input& in) const { return eval_int(in) != 0; }

  int get_int_reg(int idx);
  void set_int_reg(int idx, int val);

  bool get_bool_reg(int idx) { return get_int_reg(idx) != 0; }
  void set_bool_reg(int idx ,bool flag) { set_int_reg(idx, flag ? 1 : 0); }

  void debug_registers(FILE *out) const;

private:
  int m_registers[NUM_REGISTERS];
};

class vm
{
public:
  vm(wordcode *code)
    : m_wordcode(code)
  {}
  ~vm() {}

  int interpret(int arg);
  void *compile();

private:
  void debug_begin_frame(int arg);
  void debug_end_frame(int pc, int result);
  void debug_begin_opcode(const frame &f, int pc);
  void debug_end_opcode(int pc);

private:
  wordcode *m_wordcode;
};

}; // namespace regvm

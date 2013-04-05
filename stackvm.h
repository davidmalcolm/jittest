namespace regvm {
  class wordcode;
};

namespace stackvm {

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

  regvm::wordcode *
  compile_to_regvm() const;

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

class vm
{
public:
  vm(bytecode *code)
    : m_bytecode(code)
  {}
  ~vm() {}

  int interpret(int arg);
  void *compile();

private:
  template <class T>
  typename T::return_type
  dispatch(typename T::input_type input);

  void debug_begin_frame(int arg);
  void debug_end_frame(int pc, int result);
  void debug_begin_opcode(const frame &f, int pc);
  void debug_end_opcode(int pc);

private:
  bytecode *m_bytecode;
};

}; // namespace stackvm

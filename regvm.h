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

#include <vector>

#include "location.h"

struct gcc_jit_context;

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
  instr(enum opcode op, int output_reg, input a, const location &loc);

  instr(enum opcode op, int output_reg, input lhs, input rhs, const location &loc);

  void disassemble(FILE *out) const;

  enum opcode m_op;
  int m_output_reg;
  input m_inputA;
  input m_inputB;
  /* Source location: */
  location m_loc;
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

  void *compile();

private:
  static int
  compilation_cb (struct gcc_jit_context *ctxt,
                  wordcode *code);
  void
  compilation_hook (struct gcc_jit_context *ctxt);

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

private:
  void debug_begin_frame(int arg);
  void debug_end_frame(int pc, int result);
  void debug_begin_opcode(const frame &f, int pc);
  void debug_end_opcode(int pc);

private:
  wordcode *m_wordcode;
};

}; // namespace regvm

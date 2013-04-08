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

#include "regvm.h"

using namespace regvm;

static const int num_inputs[NUM_OPCODES] = {
  1, // COPY_INT,
  2, // BINARY_INT_ADD,
  2, // BINARY_INT_SUBTRACT,
  2, // BINARY_INT_COMPARE_LT,
  1, // JUMP_ABS_IF_TRUE,
  1, // CALL_INT,
  1, // RETURN_INT,
};

instr::instr(enum opcode op, int output_reg, input a)
  : m_op(op),
    m_output_reg(output_reg),
    m_inputA(a),
    m_inputB(CONSTANT, 0)
{
}


instr::instr(enum opcode op, int output_reg, input lhs, input rhs)
  : m_op(op),
    m_output_reg(output_reg),
    m_inputA(lhs),
    m_inputB(rhs)
{
}

static void
write_assign_to_lhs(FILE *out, int output_reg)
{
  fprintf(out, "R%i = ", output_reg);
}

static void
write_rvalue(FILE *out, input in)
{
  switch (in.m_addrmode) {
  case CONSTANT:
    fprintf(out, "%i", in.m_value);
    break;

  case REGISTER:
    fprintf(out, "R%i", in.m_value);
    break;

  default:
    assert(0);
  }
}

static void
write_binary_op(FILE *out, const instr &ins, const char *sym)
{
    write_assign_to_lhs(out, ins.m_output_reg);
    write_rvalue(out, ins.m_inputA);
    fprintf(out, " %s ", sym);
    write_rvalue(out, ins.m_inputB);
    fprintf(out, ";\n");
}

void instr::disassemble(FILE *out) const
{
  switch (m_op) {
  case COPY_INT:
    write_assign_to_lhs(out, m_output_reg);
    write_rvalue(out, m_inputA);
    fprintf(out, ";\n");
    break;

  case BINARY_INT_ADD:
    write_binary_op(out, *this, "+");
    break;

  case BINARY_INT_SUBTRACT:
    write_binary_op(out, *this, "-");
    break;

  case BINARY_INT_COMPARE_LT:
    write_binary_op(out, *this, "<");
    break;

  case JUMP_ABS_IF_TRUE:
    fprintf(out, "IF (");
    write_rvalue(out, m_inputA);
    fprintf(out, ") GOTO ");
    write_rvalue(out, m_inputB);
    fprintf(out, ";\n");
    break;

  case CALL_INT:
    write_assign_to_lhs(out, m_output_reg);
    fprintf(out, "CALL(");
    write_rvalue(out, m_inputA);
    fprintf(out, ");\n");
    break;

  case RETURN_INT:
    fprintf(out, "RETURN(");
    write_rvalue(out, m_inputA);
    fprintf(out, ");\n");
    break;

  default:
    assert(0); // FIXME
  }
}

void wordcode::disassemble(FILE *out) const
{
  for (int pc = 0; pc < (int)m_instrs.size(); /* */) {
    disassemble_at(out, pc);
  }
}
void wordcode::disassemble_at(FILE *out, int &pc) const
{
  fprintf(out, "[%i] : ", pc);
  const instr& ins = fetch_instr(pc);
  ins.disassemble(out);
}

const instr&
wordcode::fetch_instr(int &pc) const
{
  return m_instrs[pc++];
}

// Experimental JIT compilation via gcc-c-api:
#if 0
#define FAKE_TYPE(X) \
  typedef struct X { \
    void *inner;     \
  } X;

FAKE_TYPE(gcc_context)
FAKE_TYPE(gcc_function)
FAKE_TYPE(gcc_basic_block)
FAKE_TYPE(gcc_tree)
FAKE_TYPE(gcc_var_decl)
FAKE_TYPE(gcc_gimple)

enum gcc_expr_code
{
  EQ_EXPR
};

extern gcc_context
gcc_context_new (void);

extern gcc_function
gcc_function_new (gcc_context ctxt);

extern gcc_basic_block
gcc_basic_block_new (gcc_context ctxt);

extern gcc_gimple
gcc_gimple_assign_new_COPY(gcc_context ctxt,
                           gcc_tree dst,
                           gcc_tree src);

extern gcc_gimple
gcc_gimple_assign_new_ADD(gcc_context ctxt,
                          gcc_tree dst,
                          gcc_tree lhs,
                          gcc_tree rhs);

extern gcc_gimple
gcc_gimple_assign_new_SUBTRACT(gcc_context ctxt,
                               gcc_tree dst,
                               gcc_tree lhs,
                               gcc_tree rhs);

extern gcc_gimple
gcc_gimple_assign_new_COMPARE_LT(gcc_context ctxt,
                                 gcc_tree dst,
                                 gcc_tree lhs,
                                 gcc_tree rhs);

extern gcc_gimple
gcc_gimple_call_new(gcc_context ctxt,
                    gcc_tree dst,
                    /* FIXME: add the function itself! */
                    gcc_tree arg);

extern gcc_gimple
gcc_gimple_cond_new(gcc_context ctxt,
                    gcc_tree lhs,
                    enum gcc_expr_code exprcode,
                    gcc_tree rhs,
                    gcc_basic_block on_true,
                    gcc_basic_block on_false);

extern gcc_gimple
gcc_gimple_jump_new(gcc_context ctxt,
                    gcc_basic_block dst);

extern gcc_gimple
gcc_gimple_return_new(gcc_context ctxt,
                      gcc_tree retval);

extern gcc_tree
gcc_integer_const_new(gcc_context ctxt,
                      int value);

class frame_compiler
{
public:
  // We will have one local per "register":
  std::vector<gcc_var_decl> m_locals;

  gcc_tree eval_int(const input& in) const;
  gcc_tree get_output_reg(const instr &ins) const;

  void set_bb(gcc_basic_block bb);
  void add_gimple(gcc_gimple stmt);
};

void *wordcode::compile()
{
  frame_compiler f;
  int pc;

  gcc_context ctxt = gcc_context_new ();
#if 0
  gcc_function fn = gcc_function_new (ctxt);
  gcc_type int_type = gcc_type_get_int (ctxt);
  gcc_function_type fntype = gcc_function_type_new (ctxt,
                                                    int_type,
                                                    int_type);
  gcc_declaration fndecl = gcc_function_decl_new(fntype, "fibonacci");
#endif

  // 1st pass: create one (empty) "basic block" per opcode:
  // (do these ever get merged by gcc?)
  std::vector<gcc_basic_block> bbs;
  for (std::vector<instr>::iterator it = m_instrs.begin();
       it != m_instrs.end();
       ++it)
    {
      gcc_basic_block bb = gcc_basic_block_new (ctxt);
      bbs.push_back(bb);
    }

  // 2nd pass: fill in gimple:
  pc = 0;
  for (pc = 0; pc < (int)m_instrs.size(); pc++)
    {
      gcc_basic_block bb = bbs[pc];
      f.set_bb(bb);
      const instr &ins = m_instrs[pc];
      ins.disassemble(stdout);

      switch (ins.m_op) {
        case COPY_INT:
        {
          gcc_tree src = f.eval_int(ins.m_inputA);
          gcc_tree dst = f.get_output_reg(ins);
          f.add_gimple(gcc_gimple_assign_new_COPY(ctxt, dst, src));
        }
        break;

      case BINARY_INT_ADD:
        {
          gcc_tree lhs = f.eval_int(ins.m_inputA);
          gcc_tree rhs = f.eval_int(ins.m_inputB);
          gcc_tree dst = f.get_output_reg(ins);
          f.add_gimple(gcc_gimple_assign_new_ADD(ctxt, dst, lhs, rhs));
        }
        break;

      case BINARY_INT_SUBTRACT:
        {
          gcc_tree lhs = f.eval_int(ins.m_inputA);
          gcc_tree rhs = f.eval_int(ins.m_inputB);
          gcc_tree dst = f.get_output_reg(ins);
          f.add_gimple(gcc_gimple_assign_new_SUBTRACT(ctxt, dst, lhs, rhs));
        }
        break;

      case BINARY_INT_COMPARE_LT:
        {
          gcc_tree lhs = f.eval_int(ins.m_inputA);
          gcc_tree rhs = f.eval_int(ins.m_inputB);
          gcc_tree dst = f.get_output_reg(ins);
          f.add_gimple(gcc_gimple_assign_new_COMPARE_LT(ctxt, dst, lhs, rhs));
        }
        break;

      case JUMP_ABS_IF_TRUE:
        {
          gcc_tree flag = f.eval_int(ins.m_inputA);
          assert(ins.m_inputB.m_addrmode == CONSTANT);
          int dest = ins.m_inputB.m_value;

          gcc_basic_block on_true = bbs[dest];
          gcc_basic_block on_false = bbs[pc + 1];

          f.add_gimple(gcc_gimple_cond_new(ctxt,
                                          flag,
                                           EQ_EXPR,
                                           gcc_integer_const_new(ctxt, 0),
                                           on_true,
                                           on_false));//falselabel
        }
        break;

      case CALL_INT:
        {
          gcc_tree arg = f.eval_int(ins.m_inputA);
          gcc_tree dst = f.get_output_reg(ins);
          f.add_gimple(gcc_gimple_call_new(ctxt, dst, arg));
        }
        break;

      case RETURN_INT:
        {
          gcc_tree result = f.eval_int(ins.m_inputA);
          f.add_gimple(gcc_gimple_return_new(ctxt, result));
        }
        break;

      default:
        assert(0); // FIXME
      }

      if (ins.m_op != RETURN_INT &&
          ins.m_op != JUMP_ABS_IF_TRUE)
        {
          gcc_basic_block nextbb = bbs[pc + 1];
          f.add_gimple(gcc_gimple_jump_new(ctxt, nextbb));
        }
    }
  return NULL;
}
#endif

int vm::interpret(int input)
{
  frame f;
  int pc = 0;
  debug_begin_frame(input);
  f.set_int_reg(0, input);
  while (1) {
    debug_begin_opcode(f, pc);
    const instr &ins = m_wordcode->fetch_instr(pc);
    switch (ins.m_op) {
      case COPY_INT:
        {
          int result = f.eval_int(ins.m_inputA);
          f.set_int_reg(ins.m_output_reg, result);
        }
        break;

      case BINARY_INT_ADD:
        {
          int lhs = f.eval_int(ins.m_inputA);
          int rhs = f.eval_int(ins.m_inputB);
          int result = lhs + rhs;
          f.set_int_reg(ins.m_output_reg, result);
        }
        break;

      case BINARY_INT_SUBTRACT:
        {
          int lhs = f.eval_int(ins.m_inputA);
          int rhs = f.eval_int(ins.m_inputB);
          int result = lhs - rhs;
          f.set_int_reg(ins.m_output_reg, result);
        }
        break;

      case BINARY_INT_COMPARE_LT:
        {
          int lhs = f.eval_int(ins.m_inputA);
          int rhs = f.eval_int(ins.m_inputB);
          bool result = lhs < rhs;
          f.set_bool_reg(ins.m_output_reg, result);
        }
        break;

      case JUMP_ABS_IF_TRUE:
        {
          bool flag = f.eval_int(ins.m_inputA);
          int dest = f.eval_int(ins.m_inputB);
          if (flag) {
            pc = dest;
          }
        }
        break;

      case CALL_INT:
        {
          int arg = f.eval_int(ins.m_inputA);
          int result = interpret(arg); //recurse
          f.set_int_reg(ins.m_output_reg, result);
        }
        break;

      case RETURN_INT:
        {
          int result = f.eval_int(ins.m_inputA);
          debug_end_frame(pc, result);
          return result;
        }
        break;

      default:
        assert(0); // FIXME
      }
    debug_end_opcode(pc);
  }
}

frame::frame()
{
  for (int i = 0; i < NUM_REGISTERS; i++) {
    m_registers[i] = 0xDEADBEEF;
  }
}

int frame::eval_int(const input& in) const
{
  switch (in.m_addrmode) {
  case CONSTANT:
    return in.m_value;
  case REGISTER:
    assert(in.m_value >= 0);
    assert(in.m_value < NUM_REGISTERS);
    return m_registers[in.m_value];
  default:
    assert(0);
  }
}

int frame::get_int_reg(int idx)
{
  assert(idx >= 0);
  assert(idx < NUM_REGISTERS);
  printf("setting reg%i as %i\n", idx, m_registers[idx]);
  return m_registers[idx];
}

void frame::set_int_reg(int idx, int val)
{
  assert(idx >= 0);
  assert(idx < NUM_REGISTERS);
  printf("setting reg%i to %i\n", idx, val);
  m_registers[idx] = val;
}

void frame::debug_registers(FILE *out) const
{
  for (int i = 0; i < NUM_REGISTERS; i++) {
    fprintf(out, "    register %i: %i\n", i, m_registers[i]);
  }
}

void vm::debug_begin_frame(int arg)
{
  printf("BEGIN FRAME: arg=%i\n", arg);
}

void vm::debug_end_frame(int pc, int result)
{
  printf("END FRAME: result=%i\n", result);
}

void vm::debug_begin_opcode(const frame &f, int pc)
{
  printf("begin opcode: ");
  m_wordcode->disassemble_at(stdout, pc);
  printf("  registers: \n");
  f.debug_registers(stdout);
}

void vm::debug_end_opcode(int pc)
{
}

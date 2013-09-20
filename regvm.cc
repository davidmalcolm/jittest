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
#include "libgccjit.h"

using namespace regvm;

static const int num_inputs[NUM_OPCODES] = {
  1, // COPY_INT,
  2, // BINARY_INT_ADD,
  2, // BINARY_INT_SUBTRACT,
  2, // BINARY_INT_COMPARE_LT,
  2, // JUMP_ABS_IF_TRUE,
  1, // CALL_INT,
  1, // RETURN_INT,
};

instr::instr(enum opcode op, int output_reg, input a)
  : m_op(op),
    m_output_reg(output_reg),
    m_inputA(a),
    m_inputB(CONSTANT, 0)
{
  assert(num_inputs[op] == 1);
}


instr::instr(enum opcode op, int output_reg, input lhs, input rhs)
  : m_op(op),
    m_output_reg(output_reg),
    m_inputA(lhs),
    m_inputB(rhs)
{
  assert(num_inputs[op] == 2);
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

// Experimental JIT compilation via libgccjit:
#if 1
class frame_compiler
{
public:
  frame_compiler(gcc_jit_context *ctxt,
                 gcc_jit_function *fn) :
    m_ctxt(ctxt),
    m_fn(fn),
    m_int_type(gcc_jit_context_get_int_type (m_ctxt))
  {
    for (int i = 0; i < NUM_REGISTERS; i++) {
      char buf[10];
      sprintf (buf, "R%i", i);
      gcc_jit_local *local =
        gcc_jit_context_new_local (m_ctxt,
                                   NULL,
                                   m_int_type,
                                   buf);
      m_locals.push_back(local);
    }
  }

  // We will have one local per "register":
  std::vector<gcc_jit_local *> m_locals;

  gcc_jit_rvalue *eval_int(const input& in) const;
  gcc_jit_lvalue *get_reg(int idx) const;
  gcc_jit_lvalue *get_output_reg(const instr &ins) const;

private:
  gcc_jit_context *m_ctxt;
  gcc_jit_function *m_fn;
  gcc_jit_type *m_int_type;
};


gcc_jit_rvalue *
frame_compiler::eval_int(const input& in) const
{
  switch (in.m_addrmode) {
  case CONSTANT:
    return
      gcc_jit_context_new_rvalue_from_int (m_ctxt,
                                           m_int_type,
                                           in.m_value);
  case REGISTER:
    assert(in.m_value >= 0);
    assert(in.m_value < NUM_REGISTERS);
    return gcc_jit_local_as_rvalue (m_locals[in.m_value]);

  default:
    assert(0);
  }
}

gcc_jit_lvalue *
frame_compiler::get_reg(int idx) const
{
  return gcc_jit_local_as_lvalue (m_locals[idx]);
}

gcc_jit_lvalue *
frame_compiler::get_output_reg(const instr &ins) const
{
  return get_reg (ins.m_output_reg);
}

void *wordcode::compile()
{
  gcc_jit_context *ctxt = gcc_jit_context_acquire ();

  gcc_jit_context_set_code_factory (ctxt,
                                    (gcc_jit_code_callback)compilation_cb,
                                    this);
  gcc_jit_result *result = gcc_jit_context_compile (ctxt);
  gcc_jit_context_release (ctxt);

  return gcc_jit_result_get_code (result,
                                  "fibonacci" /* FIXME */);
  /* FIXME: this leaks "result" */
}

int
wordcode::compilation_cb(gcc_jit_context *ctxt, wordcode *code)
{
  code->compilation_hook (ctxt);
  return 0;
}

void
wordcode::compilation_hook (struct gcc_jit_context *ctxt)
{
  int pc;

  gcc_jit_type *int_type = gcc_jit_context_get_int_type (ctxt);
  gcc_jit_param *param =
    gcc_jit_context_new_param (ctxt, NULL, int_type, "input");
  gcc_jit_function *fn =
    gcc_jit_context_new_function (ctxt,
                                  NULL,
                                  GCC_JIT_FUNCTION_EXPORTED,
                                  int_type,
                                  "fibonacci", /* FIXME */
                                  1, &param, 0);
  frame_compiler f(ctxt, fn);

  // 1st pass: create forward labels, one per opcode:
  std::vector<gcc_jit_label *> labels;
  pc = 0;
  for (std::vector<instr>::iterator it = m_instrs.begin();
       it != m_instrs.end();
       ++it, ++pc)
    {
      char buf[16];
      sprintf (buf, "instr%i", pc);
      gcc_jit_label *label = gcc_jit_function_new_forward_label (fn, buf);
      labels.push_back(label);
    }

  // Assign param to R0:
  gcc_jit_function_add_assignment (fn, NULL,
                                   f.get_reg (0),
                                   gcc_jit_param_as_rvalue (param));

  // 2nd pass: fill in instructions:
  for (pc = 0; pc < (int)m_instrs.size(); pc++)
    {
      gcc_jit_function_place_forward_label (fn, labels[pc]);

      const instr &ins = m_instrs[pc];
      ins.disassemble(stdout);

      switch (ins.m_op) {
        case COPY_INT:
        {
          gcc_jit_rvalue *src = f.eval_int(ins.m_inputA);
          gcc_jit_lvalue *dst = f.get_output_reg(ins);
          gcc_jit_function_add_assignment (fn, NULL, dst, src);
        }
        break;

      case BINARY_INT_ADD:
      case BINARY_INT_SUBTRACT:
        {
          enum gcc_jit_binary_op op
            = ((ins.m_op == BINARY_INT_ADD)
               ? GCC_JIT_BINARY_OP_PLUS : GCC_JIT_BINARY_OP_MINUS);
          gcc_jit_rvalue *lhs = f.eval_int(ins.m_inputA);
          gcc_jit_rvalue *rhs = f.eval_int(ins.m_inputB);
          gcc_jit_lvalue *dst = f.get_output_reg(ins);
          gcc_jit_function_add_assignment (
            fn, NULL, dst,
            gcc_jit_context_new_binary_op (ctxt, NULL, op,
                                           int_type,
                                           lhs, rhs));
        }
        break;

      case BINARY_INT_COMPARE_LT:
        {
          gcc_jit_rvalue *lhs = f.eval_int(ins.m_inputA);
          gcc_jit_rvalue *rhs = f.eval_int(ins.m_inputB);
          gcc_jit_lvalue *dst = f.get_output_reg(ins);
          gcc_jit_function_add_assignment (
            fn, NULL, dst,
            gcc_jit_context_new_comparison (ctxt, NULL, GCC_JIT_COMPARISON_LT,
                                            lhs, rhs));
        }
        break;

      case JUMP_ABS_IF_TRUE:
        {
          gcc_jit_rvalue *flag = f.eval_int(ins.m_inputA);
          assert(ins.m_inputB.m_addrmode == CONSTANT);
          int dest = ins.m_inputB.m_value;

          gcc_jit_label *on_true = labels[dest];
          gcc_jit_label *on_false = labels[pc + 1];

          gcc_jit_function_add_conditional (fn, NULL,
                                            flag,
                                            on_true,
                                            on_false);
        }
        break;

      case CALL_INT:
        {
          gcc_jit_rvalue *arg = f.eval_int(ins.m_inputA);
          gcc_jit_lvalue *dst = f.get_output_reg(ins);
          gcc_jit_function_add_assignment (
            fn, NULL, dst,
            gcc_jit_context_new_call (ctxt, NULL, fn,
                                      1, &arg));
        }
        break;

      case RETURN_INT:
        {
          gcc_jit_rvalue *result = f.eval_int(ins.m_inputA);
          gcc_jit_function_add_return (fn, NULL, result);
        }
        break;

      default:
        assert(0); // FIXME
      }
    }
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

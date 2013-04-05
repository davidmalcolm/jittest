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
  for (int pc = 0; pc < m_instrs.size(); /* */) {
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


void *vm::compile()
{
  return NULL;
}

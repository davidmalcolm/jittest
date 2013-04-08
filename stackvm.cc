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
#include <vector>
#include <map>

#include "stackvm.h"
#include "regvm.h"

using namespace stackvm;

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

class compilation_frame
{
public:
  compilation_frame() :
    m_depth(1) // 1 initial arg
  {}

  regvm::input get_accum() {
    return regvm::input(regvm::REGISTER,
                        regvm::NUM_REGISTERS - 1);
  }

  regvm::input pop_int();
  void push_int(regvm::input);

  regvm::input pop_bool() { return pop_int(); }
  void push_bool(regvm::input abstrval) { push_int(abstrval); }

  void add_instr(const regvm::instr&);

  int next_instr_idx() { return m_instrs.size(); }

  //private:
  int m_depth;
  std::vector<regvm::instr> m_instrs;
};

regvm::input compilation_frame::pop_int()
{
  return regvm::input(regvm::REGISTER, --m_depth);
}

void compilation_frame::push_int(regvm::input in)
{
  add_instr(regvm::instr(regvm::COPY_INT,

                         // dst:
                         m_depth++,

                         // src:
                         in));
}

void compilation_frame::add_instr(const regvm::instr& ins)
{
  printf("add_instr: [%i] ", (int)m_instrs.size());
  ins.disassemble(stdout);
  m_instrs.push_back(ins);
}

regvm::wordcode *
bytecode::compile_to_regvm() const
{
  compilation_frame f;
  int pc = 0;

  // Map from offset within src opcodes to index of first generated instr
  std::map<int, int> index_map;

  while (pc < m_len) {
    index_map.insert(std::make_pair(pc, f.next_instr_idx()));
    enum opcode op = fetch_opcode(pc);
    switch (op) {
      case DUP:
        {
          regvm::input top = f.pop_int();
          f.push_int(top);
          f.push_int(top);
        }
        break;

      case ROT:
        {
          regvm::input accum = f.get_accum();
          f.add_instr(regvm::instr(regvm::COPY_INT,

                                   // dst:
                                   accum.m_value,

                                   //src:
                                   regvm::input(regvm::REGISTER,
                                                f.m_depth - 1)));

          f.add_instr(regvm::instr(regvm::COPY_INT,

                                   // dst:
                                   f.m_depth - 1,

                                   //src:
                                   regvm::input(regvm::REGISTER,
                                                f.m_depth - 2)));
          f.add_instr(regvm::instr(regvm::COPY_INT,

                                   // dst:
                                   f.m_depth - 2,

                                   //src:
                                   accum));
        }
        break;

      case PUSH_INT_CONST:
        {
          f.push_int(regvm::input(regvm::CONSTANT,
                                  fetch_arg_int(pc)));
        }
        break;

      case BINARY_INT_ADD:
        {
          regvm::input rhs = f.pop_int();
          regvm::input lhs = f.pop_int();
          regvm::input accum = f.get_accum();
          f.add_instr(regvm::instr(regvm::BINARY_INT_ADD,
                                   accum.m_value,
                                   lhs, rhs));
          f.push_int(accum);
        }
        break;

      case BINARY_INT_SUBTRACT:
        {
          regvm::input rhs = f.pop_int();
          regvm::input lhs = f.pop_int();
          regvm::input accum = f.get_accum();
          f.add_instr(regvm::instr(regvm::BINARY_INT_SUBTRACT,
                                   accum.m_value,
                                   lhs, rhs));
          f.push_int(accum);
        }
        break;

      case BINARY_INT_COMPARE_LT:
        {
          regvm::input rhs = f.pop_int();
          regvm::input lhs = f.pop_int();
          regvm::input accum = f.get_accum();
          f.add_instr(regvm::instr(regvm::BINARY_INT_COMPARE_LT,
                                   accum.m_value,
                                   lhs, rhs));
          f.push_bool(accum);
        }
        break;

      case JUMP_ABS_IF_TRUE:
        {
          regvm::input flag = f.pop_bool();
          int dest = fetch_arg_int(pc);
          f.add_instr(regvm::instr(regvm::JUMP_ABS_IF_TRUE,
                                   0,
                                   flag,
                                   regvm::input(regvm::CONSTANT, dest)));
          // the dest address gets patched below
        }
        break;

      case CALL_INT:
        {
          regvm::input arg = f.pop_int();
          regvm::input accum = f.get_accum();
          f.add_instr(regvm::instr(regvm::CALL_INT,
                                   accum.m_value,
                                   arg));
          f.push_int(accum);
        }
        break;

      case RETURN_INT:
        {
          regvm::input result = f.pop_int();
          f.add_instr(regvm::instr(regvm::RETURN_INT,
                                   0,
                                   result));
        }
        break;

      default:
        assert(0); // FIXME
      }
  }

  // Patch jumps (from referring to offsets in src bytecode
  // to referring to indices in generated wordcode)
  for (unsigned int i = 0; i < f.m_instrs.size(); i++) {
    regvm::instr &ins = f.m_instrs[i];
    if (regvm::JUMP_ABS_IF_TRUE == ins.m_op) {
      ins.m_inputB.m_value = index_map.find(ins.m_inputB.m_value)->second;
    }
  }

  return new regvm::wordcode(f.m_instrs);
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

int vm::interpret(int input)
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

void vm::debug_begin_frame(int arg)
{
  printf("BEGIN FRAME: arg=%i\n", arg);
}

void vm::debug_end_frame(int pc, int result)
{
  printf("END FRAME: result=%i\n", result);
  m_bytecode->disassemble_at(stdout, pc);
}

void vm::debug_begin_opcode(const frame &f, int pc)
{
  printf("begin opcode: ");
  m_bytecode->disassemble_at(stdout, pc);
  printf("  stack: \n");
  f.debug_stack(stdout);
}

void vm::debug_end_opcode(int pc)
{
}


void *vm::compile()
{
  return NULL;
}

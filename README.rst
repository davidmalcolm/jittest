This is a simple testbed intended for experiments with JIT compilation

It builds an executable containing two virtual machines: stackvm and regvm

stackvm
=======
A simple virtual machine in which each frame has a stack of integer values.
It runs bytecode programs.  The following bytecodes are supported::

  DUP
  ROT
  PUSH_INT_CONST
  BINARY_INT_ADD
  BINARY_INT_SUBTRACT
  BINARY_INT_COMPARE_LT
  JUMP_ABS_IF_TRUE
  CALL_INT
  RETURN_INT

It is intended as a (very simple) example of the kind of bytecode
interpreter seen in dynamic languages such as Python, Ruby etc

stackvm programs can be interpreted, disassembled, and compiled to regvm
programs.


regvm
=====
A simple virtual machine in which each frame has a set of integer
"registers" (i.e. numbered local variables).

Operations have one or two inputs, and zero or one outputs.

Inputs are integers plus and addressing modes: either constant integer, or
register index.

The following operations are supported::

  COPY_INT
  BINARY_INT_ADD
  BINARY_INT_SUBTRACT
  BINARY_INT_COMPARE_LT
  JUMP_ABS_IF_TRUE
  CALL_INT
  RETURN_INT

It is intended as a simpler VM from which to generate machine code.

regvm programs can be interpreted and disassembled.

The main program creates a simple recursive Fibonacci program using stackvm,
interprets it for one input, then compiles it to regvm, and interprets that
again for one input.

The aim is to support just-in-time compilation of these programs to
machine code, though this aspect is still a work-in-progress.

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

Here's what a simple recursive Fibonacci program looks like in stackvm
bytecode:

  [0] : DUP
  [1] : PUSH_INT_CONST 2
  [3] : BINARY_INT_COMPARE_LT
  [4] : JUMP_ABS_IF_TRUE 17
  [6] : DUP
  [7] : PUSH_INT_CONST 1
  [9] : BINARY_INT_SUBTRACT
  [10] : CALL_INT
  [11] : ROT
  [12] : PUSH_INT_CONST 2
  [14] : BINARY_INT_SUBTRACT
  [15] : CALL_INT
  [16] : BINARY_INT_ADD
  [17] : RETURN_INT


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

Here's what the Fibonacci program looks like after it's been compiled to
regvm code:

  [0] : R0 = R0;
  [1] : R1 = R0;
  [2] : R2 = 2;
  [3] : R3 = R1 < R2;
  [4] : R1 = R3;
  [5] : IF (R1) GOTO 23;
  [6] : R0 = R0;
  [7] : R1 = R0;
  [8] : R2 = 1;
  [9] : R3 = R1 - R2;
  [10] : R1 = R3;
  [11] : R3 = CALL(R1);
  [12] : R1 = R3;
  [13] : R3 = R1;
  [14] : R1 = R0;
  [15] : R0 = R3;
  [16] : R2 = 2;
  [17] : R3 = R1 - R2;
  [18] : R1 = R3;
  [19] : R3 = CALL(R1);
  [20] : R1 = R3;
  [21] : R3 = R0 + R1;
  [22] : R0 = R3;
  [23] : RETURN(R0);

(clearly plenty of optimizations are possible on this code).

The aim is to support just-in-time compilation of these programs to
machine code, though this aspect is still a work-in-progress.

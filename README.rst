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
bytecode::

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
regvm code.  Note that no optimization happens at this stage - it simply
unrolls the stack manipulation into a set of "registers", where R0 is the
bottom of the stack, R1 directly above it etc - so there's plenty of
redundancy here::

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

This can be interpreted (by regvm.cc:vm::interpret) or compiled (by
regvm.cc:wordcode::compile).

The compiler uses my experimental libgccjit.so API for GCC:
http://gcc.gnu.org/ml/gcc-patches/2013-10/msg00228.html

One of GCC's internal representations is called "gimple".  A dump of the
initial gimple representation of the code can be seen by setting::

  gcc_jit_context_set_bool_option (ctxt,
                                   GCC_JIT_BOOL_OPTION_DUMP_INITIAL_GIMPLE,
                                   1);

in `wordcode::compile` giving the following gimple dump, which closely
resembles the regvm dump above (no optimization has happened yet.  There
is a label at every instruction, most of which are redundant as they aren't
jump targets::

  fibonacci (signed int input)
  {
    <unnamed type> D.83;
    <unnamed type> D.84;
    signed int D.85;

    R0 = input;
    instr0:
    R0 = R0;
    instr1:
    R1 = R0;
    instr2:
    R2 = 2;
    instr3:
    D.83 = R1 < R2;
    R3 = (signed int) D.83;
    instr4:
    R1 = R3;
    instr5:
    D.84 = (<unnamed type>) R1;
    if (D.84 != 0) goto instr23; else goto instr6;
    instr6:
    R0 = R0;
    instr7:
    R1 = R0;
    instr8:
    R2 = 1;
    instr9:
    R3 = R1 - R2;
    instr10:
    R1 = R3;
    instr11:
    R3 = fibonacci (R1);
    instr12:
    R1 = R3;
    instr13:
    R3 = R1;
    instr14:
    R1 = R0;
    instr15:
    R0 = R3;
    instr16:
    R2 = 2;
    instr17:
    R3 = R1 - R2;
    instr18:
    R1 = R3;
    instr19:
    R3 = fibonacci (R1);
    instr20:
    R1 = R3;
    instr21:
    R3 = R0 + R1;
    instr22:
    R0 = R3;
    instr23:
    D.85 = R0;
    return D.85;
  }

You can see the various optimization steps that gcc_jit_compile performs
by setting::

  gcc_jit_context_set_bool_option (ctxt,
                                   GCC_JIT_BOOL_OPTION_KEEP_INTERMEDIATES,
                                   1);
  gcc_jit_context_set_bool_option (ctxt,
                                   GCC_JIT_BOOL_OPTION_DUMP_EVERYTHING,
                                   1);

and reviewing the contents of the temporary directory it builds.

For example, after optimizing, the gimple becomes::

  ;; Function fibonacci (fibonacci, funcdef_no=0, decl_uid=53, symbol_order=0)
  
  Removing basic block 8
  Removing basic block 9
  fibonacci (signed int input)
  {
    unsigned int _2;
    signed int _3;
    signed int add_acc_5;
    unsigned int _8;
    signed int acc_tmp_14;
    signed int add_acc_15;
    signed int add_acc_16;
    unsigned int _17;
    signed int add_acc_20;
    signed int _21;
    signed int _22;
    unsigned int _23;
    unsigned int _24;
    signed int _25;
  
    <bb 2>:
    if (input_4(D) <= 1)
      goto <bb 7> (instr23);
    else
      goto <bb 3>;
  
    <bb 3>:
    goto <bb 5> (instr6);
  
    <bb 4>:
  
    # input_7 = PHI <input_12(4), input_4(D)(3)>
    # add_acc_20 = PHI <add_acc_15(4), 0(3)>
  instr6:
    _23 = (unsigned int) input_7;
    _24 = _23 + 4294967295;
    _25 = (signed int) _24;
    R0_11 = fibonacci (_25);
    input_12 = input_7 + -2;
    add_acc_15 = add_acc_20 + R0_11;
    if (input_12 <= 1)
      goto <bb 6>;
    else
      goto <bb 4>;
  
    <bb 6>:
    # add_acc_16 = PHI <add_acc_15(5)>
    _3 = input_4(D) + -2;
    _17 = (unsigned int) input_4(D);
    _8 = _17 + 4294967294;
    _2 = _8 >> 1;
    _21 = (signed int) _2;
    _22 = _21 * -2;
    input_13 = _3 + _22;
  
    # input_1 = PHI <input_13(6), input_4(D)(2)>
    # add_acc_5 = PHI <add_acc_16(6), 0(2)>
  instr23:
    acc_tmp_14 = add_acc_5 + input_1;
    return acc_tmp_14;
  
  }

Note how it's optimized away tail-recursion for one of the recursive
invocations.

The gimple representation is then converted to RTL (Register Transfer
Language).  In its initial state this looks like::

  ;;
  ;; Full RTL generated for this function:
  ;;
  (note 6 0 12 NOTE_INSN_DELETED)
  (note 12 6 7 2 [bb 2] NOTE_INSN_BASIC_BLOCK)
  (insn 7 12 8 2 (set (reg/v:SI 102 [ input ])
          (reg:SI 5 di [ input ])) -1
       (nil))
  (note 8 7 14 2 NOTE_INSN_FUNCTION_BEG)
  (insn 14 8 15 2 (set (reg:CCGC 17 flags)
          (compare:CCGC (reg/v:SI 102 [ input ])
              (const_int 1 [0x1]))) -1
       (nil))
  (jump_insn 15 14 16 2 (set (pc)
          (if_then_else (le (reg:CCGC 17 flags)
                  (const_int 0 [0]))
              (label_ref:DI 56)
              (pc))) 616 {*jcc_1}
       (int_list:REG_BR_PROB 900 (nil))
   -> 56)
  (note 16 15 9 4 [bb 4] NOTE_INSN_BASIC_BLOCK)
  (insn 9 16 10 4 (set (reg/v:SI 90 [ input ])
          (reg/v:SI 102 [ input ])) -1
       (nil))
  (insn 10 9 17 4 (set (reg:SI 94 [ D.99 ])
          (const_int 0 [0])) -1
       (nil))
  (jump_insn 17 10 18 4 (set (pc)
          (label_ref 20)) -1
       (nil)
   -> 20)
  (barrier 18 17 28)
  (code_label 28 18 19 5 4 "" [1 uses])
  (note 19 28 20 5 [bb 5] NOTE_INSN_BASIC_BLOCK)
  (code_label 20 19 21 6 3 ("instr6") [1 uses])
  (note 21 20 22 6 [bb 6] NOTE_INSN_BASIC_BLOCK)
  (insn 22 21 23 6 (parallel [
              (set (reg:SI 103 [ D.98 ])
                  (plus:SI (reg/v:SI 90 [ input ])
                      (const_int -1 [0xffffffffffffffff])))
              (clobber (reg:CC 17 flags))
          ]) -1
       (nil))
  (insn 23 22 24 6 (set (reg:SI 5 di)
          (reg:SI 103 [ D.98 ])) -1
       (nil))
  (call_insn 24 23 25 6 (set (reg:SI 0 ax)
          (call (mem:QI (symbol_ref:DI ("fibonacci") [flags 0x1]  <function_decl 0x7f8664784500 fibonacci>) [0 fibonacci S1 A8])
              (const_int 0 [0]))) -1
       (expr_list:REG_EH_REGION (const_int 0 [0])
          (nil))
      (expr_list:REG_BR_PRED (use (reg:SI 5 di))
          (nil)))
  (insn 25 24 26 6 (set (reg/v:SI 92 [ R0 ])
          (reg:SI 0 ax)) -1
       (nil))
  (insn 26 25 27 6 (parallel [
              (set (reg/v:SI 90 [ input ])
                  (plus:SI (reg/v:SI 90 [ input ])
                      (const_int -2 [0xfffffffffffffffe])))
              (clobber (reg:CC 17 flags))
          ]) -1
       (nil))
  (insn 27 26 29 6 (parallel [
              (set (reg:SI 94 [ D.99 ])
                  (plus:SI (reg:SI 94 [ D.99 ])
                      (reg/v:SI 92 [ R0 ])))
              (clobber (reg:CC 17 flags))
          ]) -1
       (nil))
  (insn 29 27 30 6 (set (reg:CCGC 17 flags)
          (compare:CCGC (reg/v:SI 90 [ input ])
              (const_int 1 [0x1]))) -1
       (nil))
  (jump_insn 30 29 31 6 (set (pc)
          (if_then_else (gt (reg:CCGC 17 flags)
                  (const_int 0 [0]))
              (label_ref 28)
              (pc))) -1
       (int_list:REG_BR_PROB 9100 (nil))
   -> 28)
  (note 31 30 32 7 [bb 7] NOTE_INSN_BASIC_BLOCK)
  (insn 32 31 33 7 (parallel [
              (set (reg:SI 89 [ D.99 ])
                  (plus:SI (reg/v:SI 102 [ input ])
                      (const_int -2 [0xfffffffffffffffe])))
              (clobber (reg:CC 17 flags))
          ]) -1
       (nil))
  (insn 33 32 34 7 (parallel [
              (set (reg:SI 104 [ D.98 ])
                  (plus:SI (reg/v:SI 102 [ input ])
                      (const_int -2 [0xfffffffffffffffe])))
              (clobber (reg:CC 17 flags))
          ]) -1
       (nil))
  (insn 34 33 35 7 (parallel [
              (set (reg:SI 105 [ D.98 ])
                  (lshiftrt:SI (reg:SI 104 [ D.98 ])
                      (const_int 1 [0x1])))
              (clobber (reg:CC 17 flags))
          ]) -1
       (nil))
  (insn 35 34 36 7 (set (reg:SI 106)
          (const_int 0 [0])) -1
       (nil))
  (insn 36 35 37 7 (parallel [
              (set (reg:SI 107)
                  (minus:SI (reg:SI 106)
                      (reg:SI 105 [ D.98 ])))
              (clobber (reg:CC 17 flags))
          ]) -1
       (expr_list:REG_EQUAL (mult:SI (reg:SI 105 [ D.98 ])
              (const_int -1 [0xffffffffffffffff]))
          (nil)))
  (insn 37 36 38 7 (parallel [
              (set (reg:SI 108)
                  (ashift:SI (reg:SI 107)
                      (const_int 1 [0x1])))
              (clobber (reg:CC 17 flags))
          ]) -1
       (nil))
  (insn 38 37 39 7 (set (reg:SI 107)
          (reg:SI 108)) -1
       (expr_list:REG_EQUAL (mult:SI (reg:SI 105 [ D.98 ])
              (const_int -2 [0xfffffffffffffffe]))
          (nil)))
  (insn 39 38 40 7 (set (reg:SI 97 [ D.99 ])
          (reg:SI 107)) -1
       (nil))
  (insn 40 39 53 7 (parallel [
              (set (reg/v:SI 102 [ input ])
                  (plus:SI (reg:SI 89 [ D.99 ])
                      (reg:SI 97 [ D.99 ])))
              (clobber (reg:CC 17 flags))
          ]) -1
       (nil))
  (jump_insn 53 40 54 7 (set (pc)
          (label_ref 41)) -1
       (nil)
   -> 41)
  (barrier 54 53 56)
  (code_label 56 54 55 8 5 "" [1 uses])
  (note 55 56 11 8 [bb 8] NOTE_INSN_BASIC_BLOCK)
  (insn 11 55 41 8 (set (reg:SI 94 [ D.99 ])
          (const_int 0 [0])) -1
       (nil))
  (code_label 41 11 42 9 2 ("instr23") [1 uses])
  (note 42 41 43 9 [bb 9] NOTE_INSN_BASIC_BLOCK)
  (insn 43 42 44 9 (parallel [
              (set (reg:SI 109 [ D.99 ])
                  (plus:SI (reg:SI 94 [ D.99 ])
                      (reg/v:SI 102 [ input ])))
              (clobber (reg:CC 17 flags))
          ]) -1
       (nil))
  (insn 44 43 48 9 (set (reg:SI 101 [ <retval> ])
          (reg:SI 109 [ D.99 ])) -1
       (nil))
  (insn 48 44 51 9 (set (reg/i:SI 0 ax)
          (reg:SI 101 [ <retval> ])) -1
       (nil))
  (insn 51 48 0 9 (use (reg/i:SI 0 ax)) -1
       (nil))

This goes through numerous optimizations and transformations (e.g. register
allocation, instruction selection), before reaching code.

Currently libgccjit.so goes through an intermediate stage of writing
its generate code to disk as assembler, and then converting that to
machine code.  This generated assembler can be seen by setting::

  gcc_jit_context_set_bool_option (ctxt,
                                   GCC_JIT_BOOL_OPTION_KEEP_INTERMEDIATES,
                                   1);

and reviewing the files::

  	.file	"fake.c"
  	.text
  	.p2align 4,,15
  	.globl	fibonacci
  	.type	fibonacci, @function
  fibonacci:
  .LFB0:
  	.cfi_startproc
  	cmpl	$1, %edi
  	pushq	%r12
  	.cfi_def_cfa_offset 16
  	.cfi_offset 12, -16
  	movl	%edi, %r12d
  	pushq	%rbp
  	.cfi_def_cfa_offset 24
  	.cfi_offset 6, -24
  	pushq	%rbx
  	.cfi_def_cfa_offset 32
  	.cfi_offset 3, -32
  	jle	.L5
  	movl	%edi, %ebx
  	xorl	%ebp, %ebp
  	.p2align 4,,10
  	.p2align 3
  .L3:
  	leal	-1(%rbx), %edi
  	subl	$2, %ebx
  	call	fibonacci@PLT
  	addl	%eax, %ebp
  	cmpl	$1, %ebx
  	jg	.L3
  	andl	$1, %r12d
  .L2:
  	leal	0(%rbp,%r12), %eax
  	popq	%rbx
  	.cfi_remember_state
  	.cfi_def_cfa_offset 24
  	popq	%rbp
  	.cfi_def_cfa_offset 16
  	popq	%r12
  	.cfi_def_cfa_offset 8
  	ret
  .L5:
  	.cfi_restore_state
  	xorl	%ebp, %ebp
  	jmp	.L2
  	.cfi_endproc
  .LFE0:
  	.size	fibonacci, .-fibonacci
  	.ident	"GCC: (GNU) 4.9.0 20131004 (experimental)"
  	.section	.note.GNU-stack,"",@progbits

This code is then injected into the process, and run (and calculates the
correct result!).

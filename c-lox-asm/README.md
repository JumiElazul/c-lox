## c-lox-asm

Everything in this file subject to change.

c-lox can also be written in bytecode directly.  Every c-lox-asm script
must have the following:

1. Constant section.
A constant section, which is marked as `_const:`.  This first
phase of parsing will loop over all the constants in this section,
and add them to the constant table.

```
section const:
.const 5  // Adds 5 to the constant table
.const 3  // Adds 3 to the constant table
```

2. Instruction section:
The data section is marked by `section instr:`.  This is where you can
write the opcodes directly.

```
section instr:
CONST 0  // Pushes constant_table[0] (5) to the stack
CONST 1  // Pushes constant_table[1] (3) to the stack
ADD      // Pops two values off the stack and adds them, leaving 8 on the stack
```

Comments can be added with a semicolon:
```
CONST 0 ; This is a comment
```

## Opcode Mnemonics

Bytecode | Byte | Mnemonic |
OP_CONSTANT |    |  0x00 |  CONST
OP_CONSTANT_LONG |  0x01 |  CONST_LONG
OP_ADD           |  0x02 |  ADD
OP_SUBTRACT      |  0x03 |  SUB
OP_MULTIPLY      |  0x04 |  MUL
OP_DIVIDE        |  0x05 |  DIV
OP_NEGATE        |  0x06 |  NEG
OP_RETURN        |  0x07 |  RET



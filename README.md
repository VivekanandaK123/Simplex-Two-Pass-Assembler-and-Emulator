# SIMPLEX Assembler & Emulator

**CS2206 Computer Architecture — Mini Project**  
**Name:** Vivekananda Katakam | **Roll No:** 2401CS52

---

## Build

```bash
g++ -std=c++11 -Wall -W -o asm asm.cpp
g++ -std=c++11 -Wall -W -o emu emu.cpp
```

> **Note:** Written in C++11. All algorithmic choices (two-pass design, instruction table, `strtol`, etc.) follow the C89 guidance in the spec. The only C++-specific features used are `std::string`/`vector`/`map`/`set`, `std::ostringstream`/`ifstream`, default struct initialisers, and `to_string()`.

---

## Usage

### Assembler

```bash
./asm <source.asm>
```

Produces:
- `<source>.o`   — binary object file (suppressed if any errors)
- `<source>.lst` — listing file (suppressed if any errors)
- `<source>.log` — log of all errors/warnings (always produced)

### Emulator

```bash
./emu <object.o> [options]
```

Produces `<object>.trace` (always created).

**Options:**

| Flag | Description |
|------|-------------|
| `-trace` | Print each instruction as it executes (disassembled) |
| `-before` | Show register state before each instruction |
| `-after` | Show register state after each instruction |
| `-dump` | Produce memory dump (also on by default) |
| `-T <n>` | Stop after n instructions (infinite-loop guard) |

> Make sure the executable and the `.asm`/`.o` file are in the same folder.

---

## Architecture

SIMPLEX is a Transputer-style stack machine with four 32-bit registers: `A` (accumulator), `B` (below A), `PC`, and `SP`. Instructions are 32 bits — lower 8 bits are the opcode, upper 24 bits are the signed operand.

### Instruction Set

| Mnemonic | Opcode | Description |
|----------|--------|-------------|
| `ldc`    | 0  | B := A; A := value |
| `adc`    | 1  | A := A + value |
| `ldl`    | 2  | B := A; A := mem[SP + offset] |
| `stl`    | 3  | mem[SP + offset] := A; A := B |
| `ldnl`   | 4  | A := mem[A + offset] |
| `stnl`   | 5  | mem[A + offset] := B |
| `add`    | 6  | A := B + A |
| `sub`    | 7  | A := B − A |
| `shl`    | 8  | A := B << A |
| `shr`    | 9  | A := B >> A (logical) |
| `adj`    | 10 | SP := SP + value |
| `a2sp`   | 11 | SP := A; A := B |
| `sp2a`   | 12 | B := A; A := SP |
| `call`   | 13 | B := A; A := PC; PC := PC + offset |
| `return` | 14 | PC := A; A := B |
| `brz`    | 15 | if A == 0: PC := PC + offset |
| `brlz`   | 16 | if A < 0: PC := PC + offset |
| `br`     | 17 | PC := PC + offset |
| `HALT`   | 18 | Stop emulator |
| `data`   | —  | Reserve and initialise a memory word |
| `SET`    | —  | Assign value to label — no word emitted, PC unchanged |

Branch displacement = `target_address − (current_PC + 1)`

---

## Assembler Design

- **Single `runPass()` routine** for both passes. Pass 1 builds the symbol table (no output, undefined labels tolerated). Pass 2 resolves labels, emits code and listing, enforces all errors.
- **Instruction table** — all 19 instructions defined in a static `InstructionDef` table with `opcode`, `hasOperand`, and `isBranch` fields. No per-instruction logic hard-coded outside the table.
- **Number parsing** via `strtol` (base 0): plain digits → decimal, `0x` prefix → hex, leading `0` → octal.
- **Listing file** (advanced format) — shows address, encoded word, optional label, mnemonic, and operand. Branch targets display the label name where resolvable (e.g. `br loop`).

### Error & Warning Detection

| Category | Condition |
|----------|-----------|
| Label | Duplicate definition, invalid name, undefined reference, unused (warning) |
| Instruction | Unknown mnemonic, missing operand, unexpected operand, extra tokens |
| Number | Malformed literal (full `strtol` end-pointer check) |
| Pseudo-ops | `SET` without a label (error); `data` without a label (warning) |

All errors in a run are reported — the assembler does not stop at the first error.

---

## Test Programs

| File | Description | Expected Result |
|------|-------------|-----------------|
| `test1.asm` | Valid but nonsense — ldc, branches, forward label, data word | 1 warning (unused label), 8 words assembled |
| `test2.asm` | Intentional errors — duplicate label, undefined label, invalid number, missing/unexpected operand, bogus mnemonic, etc. | 10 errors, 1 warning, no output produced |
| `test3.asm` | `SET` pseudo-instruction test (`val=75`, `val2=66`) | 3 words assembled, SET emits no word |
| `test4.asm` | Provided reference program (recursive triangle numbers, count=10) | 76 words, 47,653 instructions, normal HALT |
| `01_bubble_sort.asm` | Sorts 8-element array `{42,7,19,3,55,1,28,14}` ascending | Memory shows `{1,3,7,14,19,28,42,55}` |
| `02_binary_search.asm` | Binary search on sorted 10-element array, target=19 | `result = 3` (0-based index) |
| `03_factorial.asm` | Recursive factorial with inline multiply loop, n=5 | `result = 120` (0x78) |
| `04_errors.asm` | Additional comprehensive error test | 11 errors, 2 warnings, no output produced |

---


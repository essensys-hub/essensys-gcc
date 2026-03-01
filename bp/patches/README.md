# MQX Submodule Patches for GCC/GAS

These patches adapt the MQX 4.2 submodule source for compilation with
`m68k-elf-gcc` and `m68k-elf-as` (GNU Assembler, GAS syntax).

The MQX submodule (`mqx/`) is not modified directly.  Patches are kept
here and applied during the build.

## Patch list

| Patch | Target file | Purpose |
|-------|-------------|---------|
| `001-psp-comp-gcc.patch` | `psp_comp.h` | Add `#elif defined(__GNUC__)` to include `gcc_comp.h` |
| `002-asm-mac-gcc.patch` | `asm_mac.h` | GCC/GAS assembler macros (ELF prefix, `.equ` syntax, `.balign`) |
| `003-psp-prv-gas-compat.patch` | `psp_prv.inc` | Replace `$4000` CW hex with `0x4000` GAS hex |
| `004-dispatch-section-gcc.patch` | `dispatch.S` | Fix `.section` directive for GAS ELF format |
| `005-ipsum-section-gcc.patch` | `ipsum.S` | Fix `.section` directive for GAS ELF format |

## How to apply

```bash
cd essensys-gcc
for p in bp/patches/0*.patch; do
  git apply --directory=. "$p"
done
```

Or from the Makefile (recommended):

```makefile
patch-mqx:
	@for p in bp/patches/0*.patch; do \
	  git apply --check "$$p" 2>/dev/null && git apply "$$p" || true; \
	done
```

## Assembler flags

GAS requires `--register-prefix-optional` to accept ColdFire register
names without `%` prefix (matching the CodeWarrior convention used
throughout MQX assembly):

```bash
m68k-elf-gcc -mcpu=52259 -Wa,--register-prefix-optional -c dispatch.S
```

## Key differences: CW assembler vs GAS

| Feature | CodeWarrior | GAS |
|---------|------------|-----|
| Register prefix | `d0`, `a0`, `sp` | `%d0`, `%a0`, `%sp` (or bare with flag) |
| Hex constant | `$4000` | `0x4000` |
| Section | `.section NAME,16,x` | `.section NAME, "ax"` |
| Symbol equate | `label .equ value` | `.equ label, value` |
| Alignment | `.align N` (bytes) | `.balign N` (bytes, unambiguous) |
| Symbol prefix | `_symbol` (prepended) | `symbol` (ELF, no prefix) |

#!/bin/sh
# NR: NR-gcc-16 essensys-hub/essensys-board-SC944D#1
# Tache 5.10 : une seule macro TYPE_* par cible BA, adresse I2C du type.
# Compile ba/source/hard.h sur l'hote (stubs PIC) pour chaque cible du
# ba/Makefile et verifie I2C_ADDR ; zero ou deux macros doivent echouer.
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../.." && pwd)
CC=${CC:-cc}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

cat > "$TMP/addr.c" <<'C'
#include <stdio.h>
#include "pic16f946_stub.h"
#include "hard.h"
int main(void) { printf("%#x\n", I2C_ADDR); return 0; }
C

fail=0
check() { # board macro attendu
  macro=$(make -s -C "$ROOT/ba" BOARD="$1" -pn 2>/dev/null | sed -n 's/^BOARD_TYPE = //p' | head -1)
  if [ "$macro" != "$2" ]; then
    echo "KO $1 : macro '$macro' (attendu $2)"; fail=1; return
  fi
  "$CC" -w -D"$macro" -I "$ROOT/tests/stubs" -I "$ROOT/ba/source" -o "$TMP/a_$1" "$TMP/addr.c"
  got=$("$TMP/a_$1")
  if [ "$got" = "$3" ]; then echo "OK $1 -> $macro -> $got"; else echo "KO $1 : $got (attendu $3)"; fail=1; fi
}
check SC940  TYPE_PIECES_DE_VIE 0x11
check SC942C TYPE_CHAMBRES      0x12
check SC941C TYPE_PIECES_D_EAU  0x13

if "$CC" -w -I "$ROOT/tests/stubs" -I "$ROOT/ba/source" -o "$TMP/none" "$TMP/addr.c" 2>/dev/null; then
  echo "KO aucune macro TYPE_* : hard.h aurait du refuser"; fail=1
else echo "OK aucune macro -> refus"; fi
if "$CC" -w -DTYPE_CHAMBRES -DTYPE_PIECES_D_EAU -I "$ROOT/tests/stubs" -I "$ROOT/ba/source" -o "$TMP/two" "$TMP/addr.c" 2>/dev/null; then
  echo "KO deux macros TYPE_* : hard.h aurait du refuser"; fail=1
else echo "OK deux macros -> refus"; fi

exit $fail

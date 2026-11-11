#!/usr/bin/env python3
"""Contrôle et finalisation du firmware BP produit par GCC.

- vérifie que le .s19 reste dans la zone application 0x3000-0x7DFFF ;
- vérifie l'en-tête du bootloader (version, saut absolu en 0x3004) ;
- calcule le CRC-16/MODBUS sur 0x3002-0x7DFFF (trous à 0xFF) et l'injecte
  en 0x3000 dans un nouveau .s19 (le build laisse la valeur 0x0102) ;
- facultatif : compare les symboles du .map GCC à la carte .xMAP
  CodeWarrior de référence.

Feature bp-gcc-firmware-2026-11-006. Python 3 sans dépendance.
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

APP_START, APP_END = 0x3000, 0x7DFFF
CRC_START = 0x3002
CRC_PLACEHOLDER = 0x0102


def crc16_modbus(data: bytes) -> int:
    crc = 0xFFFF
    for b in data:
        crc ^= b
        for _ in range(8):
            crc = (crc >> 1) ^ 0xA001 if crc & 1 else crc >> 1
    return crc


def load_s19(path: Path) -> dict[int, int]:
    mem: dict[int, int] = {}
    for line in path.read_text().splitlines():
        if not line.startswith("S3"):
            continue
        n = int(line[2:4], 16)
        addr = int(line[4:12], 16)
        for i, b in enumerate(bytes.fromhex(line[12 : 12 + 2 * (n - 5)])):
            mem[addr + i] = b
    return mem


def write_s19(mem: dict[int, int], path: Path) -> None:
    lines = ["S0030000FC"]
    addrs = sorted(mem)
    i = 0
    while i < len(addrs):
        start = addrs[i]
        chunk = [mem[start]]
        while i + 1 < len(addrs) and addrs[i + 1] == start + len(chunk) and len(chunk) < 32:
            i += 1
            chunk.append(mem[addrs[i]])
        i += 1
        count = 5 + len(chunk)
        total = count + sum(start.to_bytes(4, "big")) + sum(chunk)
        lines.append(f"S3{count:02X}{start:08X}{bytes(chunk).hex().upper()}{(~total) & 0xFF:02X}")
    lines.append("S70500003004C6")
    path.write_text("\n".join(lines) + "\n")


def check(mem: dict[int, int], expected_version: int | None) -> list[str]:
    errors = []
    lo, hi = min(mem), max(mem)
    if lo < APP_START or hi > APP_END:
        errors.append(f"hors zone application : 0x{lo:X}-0x{hi:X}")
    if mem.get(0x3004) != 0x4E or mem.get(0x3005) != 0xF9:
        errors.append("pas de saut absolu (jmp 0x4EF9) en 0x3004")
    version = (mem.get(0x3002, 0) << 8) | mem.get(0x3003, 0)
    if expected_version is not None and version != expected_version:
        errors.append(f"version 0x{version:04X} au lieu de 0x{expected_version:04X}")
    return errors


def app_image(mem: dict[int, int]) -> bytes:
    return bytes(mem.get(a, 0xFF) for a in range(CRC_START, APP_END + 1))


def symbols_gcc(map_text: str) -> set[str]:
    """Symboles du .map GNU, ou d'une sortie de nm (symboles locaux inclus)."""
    found = set(re.findall(r"^\s+0x[0-9a-f]+\s+([A-Za-z_]\w*)\s*$", map_text, re.M))
    found |= set(re.findall(r"^[0-9a-f]{8} [tTdDrRbB] ([A-Za-z_]\w*)$", map_text, re.M))
    return found


def symbols_cw(xmap_text: str) -> set[str]:
    names = re.findall(r"^\s+[0-9A-F]{8} [0-9A-F]{8} \.text\s+(\S+)\s+\((\w+)_c\.obj", xmap_text, re.M)
    return {n[1:] if n.startswith("_") else n for n, _ in names}


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("s19", type=Path)
    ap.add_argument("--version", type=lambda v: int(v, 0))
    ap.add_argument("--out", type=Path, help="s19 avec CRC injecté")
    ap.add_argument("--map", type=Path, help=".map GNU ou sortie de m68k-elf-nm")
    ap.add_argument("--ref-xmap", type=Path)
    a = ap.parse_args(argv)

    mem = load_s19(a.s19)
    errors = check(mem, a.version)
    crc = crc16_modbus(app_image(mem))
    print(f"zone 0x{min(mem):X}-0x{max(mem):X}, {len(mem)} octets, CRC calculé 0x{crc:04X}")
    if a.out:
        mem[0x3000], mem[0x3001] = crc >> 8, crc & 0xFF
        write_s19(mem, a.out)
        print(f"CRC injecté : {a.out}")
    if a.map and a.ref_xmap:
        gcc = symbols_gcc(a.map.read_text(errors="replace"))
        ref = symbols_cw(a.ref_xmap.read_text(errors="replace"))
        missing = sorted(ref - gcc)
        print(f"fonctions applicatives de référence : {len(ref)}, absentes du build GCC : {len(missing)}")
        print("  (absente = intégrée par l'optimiseur ou supprimée car inutilisée, à vérifier une à une)")
        for name in missing:
            print(f"  écart : {name}")
    for e in errors:
        print(f"ERREUR : {e}", file=sys.stderr)
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())

/*
 * bootloader.c — Application header at fixed flash address 0x3000.
 *
 * GCC/GAS port of the legacy CodeWarrior C/bootloader.c. Emits the three
 * fixed-address sections the existing Essensys bootloader reads before it
 * launches the application image:
 *
 *   0x3000  .APP_CRC       unsigned short  CRC-16 of the application zone
 *   0x3002  .APP_VERSION   unsigned short  firmware version number
 *   0x3004  .APP_JUMP      jmp __boot      entry trampoline (6 bytes)
 *
 * Placement is enforced by bp/intflash.ld (.app_header @ 0x3000, ordered
 * CRC, VERSION, JUMP). The bootloader computes CRC-16 over 0x3002–0x7DFFF
 * and compares it against the halfword at 0x3000, then jumps to 0x3004.
 *
 * CRC is a placeholder here: the real value is injected post-link by the
 * CRC tool (as in the legacy build, where CodeWarrior stored 0x0102 and the
 * flashing/OTA step patched it). VERSION is a build-time constant.
 */

/* Defined in bp/bsp/m52259evb/boot.S (.global __boot). */
extern void __boot(void);

/* 0x3000 — CRC-16 placeholder, overwritten post-link by the CRC tool. */
__attribute__((section(".APP_CRC"), used))
const unsigned short APP_CRC = 0x0102;

/* 0x3002 — firmware version. Placeholder for this minimal build; the
 * metier build supplies the real us_BP_VERSION_SERVEUR. */
__attribute__((section(".APP_VERSION"), used))
const unsigned short APP_VERSION = 0x0000;

/* 0x3004 — `jmp __boot` trampoline. m68k-elf-gcc ignores the `naked`
 * attribute, but at -Os this leaf function needs no frame, so GCC emits the
 * inline `jmp __boot` at offset 0 (verified: 4ef9 0000 xxxx = 6-byte absolute
 * long jump, matching the legacy .APP_JUMP size) followed by an unreachable
 * `rts`. The bootloader jumps to 0x3004 and control transfers immediately. */
__attribute__((section(".APP_JUMP"), used))
void APP_CALL(void)
{
    __asm__ __volatile__("jmp __boot");
}

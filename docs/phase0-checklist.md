# Phase 0 — Checklist de validation

## Build
- [ ] docker build -t essensys-builder . reussit
- [ ] make bp produit BP_MQX_ETH.elf et .s19
- [ ] make -C ba all-boards produit SC940.hex, SC941C.hex, SC942C.hex

## Tests
- [ ] Tests unitaires BP : 7/7 passent
- [ ] Tests unitaires BA : 4/4 passent (19 tests)
- [ ] Tests integration : 3/3 passent (11 tests)
- [ ] CI GitHub Actions : workflow vert

## Artefacts BP
- [ ] ELF contient les sections .APP_JUMP, .APP_CRC, .APP_VERSION
- [ ] Flash < 512 Ko, SRAM < 64 Ko
- [ ] S19 valide (S0, S3, S7)

## Artefacts BA
- [ ] SC940.hex : TYPE_PIECES_DE_VIE (0x11)
- [ ] SC941C.hex : TYPE_PIECES_D_EAU (0x13)
- [ ] SC942C.hex : TYPE_CHAMBRES (0x12)
- [ ] Taille < 14 Ko chacun

## Flash (hors scope Phase 0)
- [ ] BP : flasher via USBDM et verifier boot
- [ ] BA : flasher via PICkit et verifier I2C

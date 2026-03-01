# essensys-gcc

Migration du firmware Essensys de CodeWarrior/MPLAB vers une toolchain GCC libre, avec build reproductible sous Docker et CI GitHub Actions.

## Boards supportees

| Board   | MCU        | Role                          | Artefact         |
|---------|------------|-------------------------------|------------------|
| SC944D  | MCF52259   | BP — passerelle Ethernet/MQ   | BP_MQX_ETH.elf/.s19 |
| SC940   | PIC16F946  | BA — pieces de vie (0x11)     | SC940.hex        |
| SC941C  | PIC16F946  | BA — pieces d'eau (0x13)      | SC941C.hex       |
| SC942C  | PIC16F946  | BA — chambres (0x12)          | SC942C.hex       |

## Prerequis

- Docker (pour le build cross-compile)
- GNU Make
- Git (avec support submodules)

## Build

```bash
# Construire l'image Docker avec les toolchains m68k-elf-gcc et XC8
docker build -t essensys-builder .

# Compiler tout (BP + BA)
docker run --rm -v $(pwd):/workspace essensys-builder make -C /workspace all

# Compiler BP seul
docker run --rm -v $(pwd):/workspace essensys-builder make -C /workspace bp

# Compiler BA (les 3 boards)
docker run --rm -v $(pwd):/workspace essensys-builder make -C /workspace ba
```

## Tests

Les tests unitaires et d'integration tournent sur la machine hote (pas besoin de Docker) :

```bash
# Tous les tests
make test

# Tests unitaires BP (7 suites)
make -C tests/bp run_tests

# Tests unitaires BA (4 suites, 19 tests)
make -C tests/ba run_tests

# Tests integration (3 suites, 11 tests)
make -C tests/integration run_tests
```

## Validation

Un script de validation verifie la structure du depot et lance les tests :

```bash
./scripts/validate.sh
```

Voir aussi la [checklist Phase 0](docs/phase0-checklist.md) pour la validation complete.

## CI / CD

Le workflow GitHub Actions [Essensys Firmware Pipeline](.github/workflows/firmware.yml) execute automatiquement :

1. Tests unitaires BP, BA et integration
2. Build BP (SC944D) dans Docker, avec upload de `BP_MQX_ETH.elf/.s19/.map`
3. Build BA en matrice (SC940, SC941C, SC942C), avec upload des `.hex`

## Structure du depot

```
.
├── Dockerfile          # Image Docker multi-toolchain
├── ct-ng.config        # Config crosstool-NG pour m68k-elf-gcc
├── build.sh            # Script de build de la toolchain
├── Makefile            # Build top-level (delegue a bp/ et ba/)
├── bp/                 # Firmware BP (MCF52259 + MQX)
│   ├── Makefile
│   ├── intflash.ld     # Linker script GNU ld
│   ├── compat/         # Couche de compatibilite CW -> GCC
│   └── bsp/m52259evb/  # BSP adapte pour GCC
├── ba/                 # Firmware BA (PIC16F946)
│   ├── Makefile
│   └── source/         # Sources applicatives
├── mqx/                # MQX 4.0 RTOS (submodule)
├── tests/
│   ├── unity/          # Framework Unity (submodule)
│   ├── stubs/          # Stubs hardware pour tests host
│   ├── bp/             # Tests unitaires BP
│   ├── ba/             # Tests unitaires BA
│   └── integration/    # Tests d'integration
├── scripts/
│   └── validate.sh     # Validation des artefacts
├── docs/
│   └── phase0-checklist.md
└── .github/workflows/
    └── firmware.yml    # Pipeline CI
```

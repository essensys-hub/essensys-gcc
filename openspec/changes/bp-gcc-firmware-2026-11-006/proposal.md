## Why

Seul CodeWarrior sait produire le firmware du BP SC944D (MCF52259). La chaîne open source de ce dépôt (m68k-elf-gcc, MQX 4.0, Docker) ne compile qu'un `main.c` minimal. Ticket : https://github.com/essensys-hub/essensys-feature-lifecycle/issues/66.

Un premier essai, le 11 novembre 2026, a branché les 30 sources 099-37 : elles compilent toutes, et l'édition de liens bute sur quelques points identifiés. Une sauvegarde 099-37 vérifiée (feature `bdm-firmware-backup-2026-11-005`) sert de référence.

## What Changes

- Les sources applicatives 099-37 sont compilées par `build.sh bp`, en local comme en CI Linux.
- Couche de compatibilité MQX 3.8 → 4.0 : types hérités, RTC, allocation mémoire.
- Pilote PWM de la sirène pour le MCF52259, absent de MQX 4.0.
- Contrôles de l'artefact : taille, en-tête `0x3000`, CRC, comparaison des tâches et symboles avec la carte mémoire CodeWarrior.
- Aucun flash sur une carte : l'essai sur banc fera l'objet d'une feature d'écriture séparée.

## Capabilities

### New Capabilities
- `bp-gcc-firmware`: production du firmware applicatif du BP avec la chaîne GCC open source.

### Modified Capabilities

## Impact

- `bp/Makefile`, `bp/compat/`, `bp/bsp/m52259evb/`, configuration MQX, sources applicatives intégrées au dépôt (encodage UTF-8, casse des `#include`).
- `essensys-doc` : guide de compilation sans CodeWarrior. `essensys-memory` : consignation du portage.
- Lié à essensys-gcc#14 (en-tête bootloader et vecteurs).

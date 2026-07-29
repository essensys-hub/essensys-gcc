# Évaluation — build Docker headless du firmware ColdFire SC944D

> Objectif : build 100 % reproductible, conteneurisé, non-interactif du firmware
> `BP_MQX_ETH` de la carte `essensys-board-SC944D`. Trois pistes tranchées
> **sur la base du code réel** (chemins cités), pas de suppositions.
>
> Date : 2026-07-28. Faits vérifiés depuis les dépôts locaux
> `~/ESSENSYS/essensys-gcc`, `~/ESSENSYS/essensys-board-SC944D`,
> `~/Projects/client-essensys-legacy`, et `~/ESSENSYS/essensys-doc`.

---

## TL;DR — recommandation

**Piste C (GCC bare-metal `m68k-elf-gcc`)**, déjà matérialisée par ce dépôt
`essensys-gcc`. Un build vert de `BP_MQX_ETH.elf/.s19` a été produit cette
session dans Docker, sans IDE ni licence. Piste A (CodeWarrior Linux) est
incertaine sur deux points bloquants (existence réelle d'un binaire `mwcc`/`mwld`
Linux + licence FLEXlm en conteneur éphémère) et conserve le verrou EWL.
Piste B (MCUXpresso) est **fermée** : MCUXpresso ne cible que les cœurs ARM,
pas ColdFire (m68k).

---

## Phase 1 — Investigation (faits sourcés)

### 1.1 Cible matérielle exacte

| Fait | Valeur | Source |
|------|--------|--------|
| MCU | **Freescale MCF52259CAG80** | `essensys-doc/archi/hardware-sc944d.md` (table « Microcontroleur ») |
| Cœur | **ColdFire V2**, 32-bit, 80 MHz | idem |
| Mémoire | 512 KB Flash interne, 64 KB SRAM | idem |
| `-proc` compilateur | **`52259`** | `essensys-board-SC944D/SC944D/Prog/099-37/BP_MQX_ETH/m52259evb_Int_Flash_Debug/C/crc.args` : `-proc 52259` |
| RTOS | MQX (4.0 côté migration, 3.8 d'origine) | `essensys-gcc/README.md`, `docs/mqx-4.0-vs-4.2.md` |

> ⚠️ **Nuance Piste A** : le part est **ColdFire V2** (famille MCF522xx). « ColdFire+ »
> désigne la famille **MCF51xx (V1)** — ce n'est PAS ce MCU. « CodeWarrior for
> ColdFire v7.2 » (classique, Metrowerks) supporte bien le MCF5225x, mais
> l'étiquette « ColdFire+ » de la piste pré-identifiée est imprécise et doit être
> levée avant tout choix (voir Phase 2.A).

### 1.2 Système de build actuel

Deux réalités coexistent :

- **Firmware d'origine (CodeWarrior, déployé)** : projet **Eclipse CDT managed build**.
  - `essensys-board-SC944D/SC944D/Prog/099-37/BP_MQX_ETH/.project` →
    `org.eclipse.cdt.managedbuilder.core.genmakebuilder`
  - `.cproject` → toolchain `com.freescale.coldfire.*` (« ColdFire Compiler /
    Linker / Assembler », `coldfire.toolchain.burner`)
  - `makefile` + `subdir.mk` + `*.args` **générés** par l'IDE (non éditables à la main).
  - Versions archivées : `Prog/099-37/`, `Prog/_Archives/{095-34,096-35,097-36}/`
    (schéma de version aligné sur les logs OVH `fw-vVERSION-TENTATIVE`).
- **Migration GCC (ce dépôt `essensys-gcc`)** : Makefiles écrits à la main + Docker
  multi-stage + CI. `essensys-gcc/bp/Makefile`, `Dockerfile`, `build.sh`.
- **Sources firmware** : le code C métier (Alarme, Chauffage, FilPilote, TeleInfo,
  Ethernet/RTCS, cryptage Rijndael/MD5…) vit dans
  `client-essensys-legacy/{C,Ethernet}/*.c` (≈ 30 unités, cf. la liste dans
  `BP_MQX_ETH.args`).

### 1.3 Toolchain réellement utilisée (CodeWarrior)

Invocation réelle extraite des `.args` générés (source de vérité) :

`.../m52259evb_Int_Flash_Debug/C/crc.args` (compilation) :
```
-proc 52259
-lavender model=ewl ,print=int ,scan=int ,io=raw
-I"$(MCUToolsBaseDirEnv)/ColdFire_Support/ewl/EWL_C/include"
-DDEBUG -opt level=0 -opt space -align coldfire -sdata 0
-define __CODEWARRIOR__=1 -define _DEBUG=1
```

`.../m52259evb_Int_Flash_Debug/BP_MQX_ETH.args` (link) :
```
-sym full -msgstyle parseable -proc 52259
-lavender model=ewl ,print=int ,scan=int ,io=raw
-nostdlib "$(BSP)/intflash.lcf" -m ___boot
-L"$(DEBUG)/rtcs" -L"$(MCUToolsBaseDirEnv)/ColdFire_Support/ewl/lib"
```

→ Compilateur/linker **Metrowerks `mwcc`/`mwld`** (flags `-proc`, `-lavender`,
`-msgstyle parseable`, `-opt space`, `-align coldfire`, `-sdata`). Fichier de
link **`intflash.lcf`** (format CodeWarrior). Point d'entrée `___boot`.

### 1.4 Dépendances propriétaires CodeWarrior (facteur de verrou)

| Dépendance | Preuve | Impact |
|------------|--------|--------|
| **EWL** (Embedded Warrior Library) | `-lavender model=ewl`, `-L".../ColdFire_Support/ewl/lib"`, includes `EWL_C/include` | libc propriétaire → **à remplacer** (newlib) pour sortir de CW |
| `#pragma define_section … far_absolute` | `client-essensys-legacy/C/bootloader.c:16-39` (`.APP_JUMP`/`.APP_CRC`/`.APP_VERSION`) | placement absolu → `__attribute__((section))` en GCC |
| `asm { … }` Metrowerks | `bootloader.c:19` `asm void APP_CALL(void){ jmp __boot; }` | asm inline → syntaxe GAS |
| `intflash.lcf` (linker CW) | `BP_MQX_ETH.args` | → linker script GNU `.ld` |
| Autres `#pragma` (align packed, interrupt) | `essensys-board-SC944D/BUILD_ON_LINUX.md` § « Porting to GCC » | → attributs GCC |

C'est le niveau de couplage EWL + pragmas + `.lcf` qui déciderait d'un
enfermement CodeWarrior — **mais** ce dépôt démontre que le portage est faisable
(voir 1.5 / Piste C).

### 1.5 Rôle exact de `essensys-gcc` → ouvre la Piste C

`essensys-gcc` **est une vraie migration GCC ColdFire bare-metal fonctionnelle**,
pas une abstraction :

- Toolchain : **`m68k-elf-gcc`** construite depuis **crosstool-NG 1.26.0**
  (`Dockerfile`, stage `toolchain-builder`). Cible bare-metal ELF, **newlib**
  (remplace EWL). Vérifié : la collision `strlcpy`/`strdup` RTCS↔newlib a dû être
  résolue (`bp/patches/0001-mqx-gcc-compat.patch`).
- Linker : **`bp/intflash.ld`** (remplace `intflash.lcf`), avec sections
  bootloader `.APP_CRC/.APP_VERSION/.APP_JUMP` @ 0x3000 et vecteurs @ 0x3010
  (aligné sur le firmware déployé).
- Pragmas portés : `bp/bootloader.c` reproduit le `bootloader.c` legacy en
  attributs GCC ; patch MQX (`;`→`|` pour GAS, `movec` ColdFire en immédiats,
  macros GAS…) dans `bp/patches/0001-mqx-gcc-compat.patch`.
- **Preuve de build** (cette session, dans Docker) :
  `BP_MQX_ETH-local-dev.elf/.s19/.map`, `text=28440 data=88 bss=64`, dans le
  budget Flash 512 K / SRAM 64 K. `make test` (host) vert.

> État : la **chaîne** est prouvée sur une application minimale. Restent à
> intégrer les ~30 sources métier (`client-essensys-legacy/{C,Ethernet}/*.c`),
> l'injection CRC-16 post-link, et la validation matérielle JTAG/BDM
> (cf. issue essensys-gcc #14).

---

## Phase 2 — Évaluation des pistes

### Piste A — CodeWarrior for ColdFire v7.2 « Linux »

- **Compatibilité code** : ✅ native (c'est la toolchain d'origine ; `.project`/
  `.cproject`/`.args` sont directement buildables par CW ColdFire 7.x Eclipse).
- **Build headless** : le managed-build CDT génère un `makefile` invocable par
  `mwcc`/`mwld` en ligne de commande (`-proc 52259 -lavender model=ewl … intflash.lcf`),
  ou via `ecd.bat`/`eclipsec -nosplash -application …build`. Techniquement scriptable.
- **Blocage n°1 — Linux** : **non prouvé dans le code**. Les projets sont orientés
  Windows (`$(MCUToolsBaseDirEnv)`, chemins CW MCU). L'existence d'un binaire
  `mwcc`/`mwld` **Linux** pour CW ColdFire v7.2 est à vérifier auprès des release
  notes NXP — **à lever avant de retenir A**.
- **Blocage n°2 — Licence FLEXlm** : CW ColdFire est sous licence FLEXlm. En
  conteneur éphémère, seul un **serveur flottant** (`LM_LICENSE_FILE=port@host`)
  est viable ; node-locked (hostid MAC) et dongle USB sont incompatibles avec un
  Docker sans état. Ajoute une dépendance réseau/serveur → casse l'objectif
  « offline-friendly, sans état ».
- **Verrou EWL conservé** : on reste prisonnier de la lib propriétaire.
- **Install silencieuse Dockerfile** : installeur CW historiquement InstallAnywhere
  (mode `-i silent` possible **si** édition Linux existe) — conditionné au blocage n°1.

### Piste B — MCUXpresso IDE → **FERMÉE**

- **Devices supportés** : MCUXpresso IDE cible exclusivement les cœurs **ARM**
  (Cortex-M / Cortex-A : Kinetis, LPC, i.MX RT, MCX…). Il a unifié LPCXpresso +
  Kinetis Design Studio, **tous ARM**. **ColdFire (m68k) n'y figure pas** et n'y a
  jamais figuré ; l'outil NXP pour ColdFire reste CodeWarrior.
- **MCF52259 dans MCUXpresso ?** → **NON**. Verdict argumenté : archi m68k hors
  périmètre ARM de l'outil.
- **Vérification** : liste « Supported Devices » de MCUXpresso IDE (nxp.com) —
  aucun MCF5xxx.
- **Conclusion** : piste écartée, sans regret.

### Piste C — GCC ColdFire pur (`m68k-elf-gcc`) → **RECOMMANDÉE**

- **Compatibilité code** : ✅ démontrée par `essensys-gcc` (build vert). Portage
  EWL→newlib, `.lcf`→`.ld`, pragmas→attributs déjà faits pour la chaîne + le
  header bootloader ; reste l'intégration des sources métier.
- **Build headless** : `docker build` + `docker run … build.sh bp`, 0 IDE, 0 clic.
- **Licence** : **aucune** (GPL/BSD : GCC, binutils, newlib, crosstool-NG).
- **Reproductibilité Docker** : ✅ image multi-stage à versions épinglées,
  offline après build initial, sans état.
- **Piège à éviter** : le `Dockerfile` de `essensys-board-SC944D` utilise
  `gcc-m68k-linux-gnu` (apt) — c'est un toolchain **Linux/glibc userspace**, pas
  bare-metal ; inadapté à un firmware freestanding. `essensys-gcc` utilise le bon :
  **`m68k-elf-gcc`** (newlib, `-ffreestanding -nostdlib`).

---

## Matrice de décision

Notation : ✅ favorable · ⚠️ réserve · ❌ bloquant. (5 = idéal, 1 = rédhibitoire)

| Piste | Compatibilité code | Build headless | Licence | Repro. Docker | Effort | Risque legacy | Verdict |
|-------|--------------------|----------------|---------|---------------|--------|---------------|---------|
| **A — CodeWarrior CF v7.2 Linux** | ✅ native (5) | ⚠️ scriptable mais IDE-centric (3) | ❌ FLEXlm serveur requis (2) | ⚠️ dépend licence + édition Linux non prouvée (2) | ⚠️ install/licence (3) | ✅ zéro portage (5) | ⚠️ **Repli** si portage GCC bloque ; 2 inconnues bloquantes |
| **B — MCUXpresso** | ❌ ColdFire absent (1) | — | — | — | — | — | ❌ **Fermée** (ARM only) |
| **C — GCC `m68k-elf-gcc`** | ✅ prouvée, métier à intégrer (4) | ✅ `docker run build.sh` (5) | ✅ aucune (5) | ✅ épinglé, offline, sans état (5) | ⚠️ intégration métier + CRC + valid. HW (3) | ⚠️ iso-fonctionnel à valider sur cible (3) | ✅ **Recommandée** |

---

## POC — Piste C (déjà présent dans ce dépôt, prouvé cette session)

Le POC demandé **existe et fonctionne** : `essensys-gcc/Dockerfile` +
`essensys-gcc/build.sh`. Aucun Dockerfile redondant n'est fabriqué (les faits
priment). Versions épinglées :

| Élément | Version épinglée | Où |
|---------|------------------|-----|
| Image de base | `debian:bookworm-slim` | `Dockerfile` (2 stages) |
| crosstool-NG | `crosstool-ng-1.26.0` | `Dockerfile` stage `toolchain-builder` |
| m68k-elf-gcc | config 14.2.0 — **produit 13.2.0** ⚠️ (écart à réconcilier dans `ct-ng.config`) | `ct-ng.config` |
| binutils / newlib / gdb | 2.43 / 4.5 / 15.2 | `ct-ng.config` (commentaire `Dockerfile`) |
| Paquets outil | `srecord`, `cppcheck`, `lcov` | `Dockerfile` stage final |

Commandes (non-interactives, vérifiées) :
```bash
cd ~/ESSENSYS/essensys-gcc
docker build --build-arg SKIP_XC8=1 -t essensys-builder .          # BP seul, sans XC8
docker run --rm -e VERSION=local-dev -v "$PWD":/workspace \
    essensys-builder build.sh bp                                    # → bp/build/bp/BP_MQX_ETH-local-dev.{elf,s19,map}
docker run --rm -v "$PWD":/workspace essensys-builder \
    m68k-elf-size /workspace/bp/build/bp/BP_MQX_ETH-local-dev.elf
```

> Note : `build.sh bp` (et non `make build-bp`) est le point d'entrée correct — il
> patche `psp_comp.h` avant `make`. Premier `docker build` ≈ 30-60 min
> (crosstool-NG), ensuite caché.

---

## Recommandation finale (3-5 lignes)

Retenir **Piste C — `m68k-elf-gcc` bare-metal via `essensys-gcc`** : seule option
sans licence, offline, sans état, et **déjà prouvée** (build vert cette session).
Fermer **Piste B** (MCUXpresso = ARM only, pas de ColdFire). Garder **Piste A** en
repli documentaire uniquement. **Principal blocker à lever** : intégrer les ~30
sources métier `client-essensys-legacy/{C,Ethernet}/*.c` dans `bp/Makefile`
(EWL→newlib au cas par cas), puis injection CRC-16 post-link et validation
matérielle JTAG/BDM (issue essensys-gcc #14). Fait annexe à corriger : l'écart de
version GCC 13.2.0 vs 14.2.0 dans `ct-ng.config`.

---

## Ce qui n'a PAS pu être vérifié depuis le code (à lever)

1. **Existence réelle d'une édition Linux de CW ColdFire v7.2** (`mwcc`/`mwld`
   Linux) → release notes / installeur NXP. Sans ça, Piste A tombe.
2. **Type de licence CW disponible** (flottant vs node-locked/dongle) → sans
   serveur FLEXlm flottant, A est incompatible Docker éphémère.
3. **Iso-fonctionnalité GCC vs CodeWarrior sur cible** → nécessite flash JTAG/BDM
   d'un `.s19` GCC sur un SC944D réel (aucun test HW possible sans matériel).

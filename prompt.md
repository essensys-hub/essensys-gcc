# Prompt Cursor : migration du firmware Essensys BP_MQX_ETH vers une toolchain GCC open-source

## Contexte

Tu travailles dans le depot `essensys-gcc`. L'objectif est de **migrer le firmware Essensys BP_MQX_ETH** — actuellement compile avec CodeWarrior (Metrowerks) et MQX RTOS 3.8 — vers une chaine **GCC/binutils/gdb m68k-elf** open-source.

Ce firmware tourne sur une carte custom **SC944D** equipee d'un **MCF52259** (ColdFire V2, 80 MHz, 512 Ko Flash, 64 Ko SRAM). Il controle un systeme domotique complet : eclairage, volets, chauffage, alarme, arrosage, via 3 cartes auxiliaires (I2C), un ecran tactile (UART), un compteur Linky, et communique avec un serveur backend via HTTP polling.

### Sources de reference (par ordre de priorite)

1. **Code source existant** : `client-essensys-legacy/` — le firmware actuel complet
2. **Documentation architecture** : `essensys-doc/archi/` — 20+ documents techniques
3. **Rapport de recherche** : `deep-research-report.md` dans ce depot — etude des toolchains alternatives
4. **Skills Cursor** : `mcf52259-mqx-expert`, `altium-electronics-expert`

### Documents essentiels a consulter

| Document | Chemin | Contenu cle |
|----------|--------|-------------|
| Vue d'ensemble firmware | `essensys-doc/archi/legacy-client.md` | Architecture 5 taches, contraintes HTTP, table d'echange |
| Build et toolchain actuelle | `essensys-doc/archi/legacy-client-build.md` | Makefile CodeWarrior, structure projet, versions |
| Deploiement et OTA | `essensys-doc/archi/legacy-client-deployment.md` | Bootloader, zones memoire, format S19, CRC |
| GPIO et configuration | `essensys-doc/archi/legacy-client-config.md` | Mapping GPIO ↔ firmware ↔ hardware |
| Protocoles I2C/UART/SPI | `essensys-doc/archi/legacy-client-protocols.md` | Trames I2C 6 octets, CRC-16, SPI chip selects |
| Securite | `essensys-doc/archi/legacy-client-security.md` | MD5, Base64, AES/Rijndael, EEPROM |
| Hardware SC944D | `essensys-doc/archi/hardware-sc944d.md` | Schema fonctionnel, BOM, peripheriques MCU |
| Table d'echange | `essensys-doc/archi/exchange-table.md` | 953 indices, droits, persistance Flash |

## La cible : le firmware actuel

### Architecture logicielle (MQX RTOS 3.8)

```
BP_MQX_ETH (MCF52259 @ 80 MHz)
├── Main_task      (1596 o stack) — chauffage, alarme, scenarios, fil pilote
├── Ecran_task     (1500 o stack) — UART 0, ecran tactile
├── Boitiers_task  (1796 o stack) — I2C master, 3 BA (0x11, 0x12, 0x13)
├── TeleInfo_task  (1396 o stack) — UART 1, Linky 1200 bauds 7E1
└── Ethernet_task  (3000 o stack) — RTCS TCP/IP, HTTP polling 2s, OTA
    Stack totale : ~9 288 octets
```

### Peripheriques utilises

| Peripherique | Usage | Driver actuel |
|-------------|-------|---------------|
| Ethernet MAC + RTCS | HTTP polling serveur, OTA | MQX RTCS (TCP/IP complet) |
| I2C master | Communication 3 BA (relais, variateurs) | Polled mode, CRC-16, trames 6 octets |
| UART 0 | Ecran tactile (9600, 8N1) | MQX serial driver |
| UART 1 | Compteur Linky (1200, 7E1) | MQX serial driver |
| UART 2 | Debug EspionRS (9600) | MQX serial driver |
| SPI CS0 | EEPROM 25AA02E48T (MAC, cle serveur, code alarme) | Driver custom (`Eepromspi.c`) |
| SPI CS2 | Flash SST25VF016B (firmware OTA, 2 Mbit) | Driver custom (`Download.c`) |
| ADC | Sondes fuite lave-linge (AIN5, AIN6) | Registres directs |
| PWM | Fil pilote chauffage (4 zones), sirene interieure | MQX timer driver |
| GPIO | ~30 E/S (detecteurs alarme, relais, sirenes, cumulus) | Registres directs |
| GPT | Anemometre (comptage impulsions vent) | Registres directs |

### Bootloader et layout memoire Flash

```
0x00000000 ┌─────────────────────────────┐
           │  Vecteurs + Bootloader       │
           ├─────────────────────────────┤
           │  .APP_JUMP  (saut → __boot)  │
           │  .APP_CRC   (CRC 2 octets)   │
           │  .APP_VERSION (ex: 37)        │
           ├─────────────────────────────┤
           │                              │
           │  Application MQX             │
           │  (BP_MQX_ETH.elf)            │
           │  ~280 Ko utilises / 512 Ko    │
           │                              │
0x0007E000 ├─────────────────────────────┤
           │  Sauvegarde Tb_Echange[]     │
           │  Secteur Flash (4096 octets)  │
0x0007FFFF └─────────────────────────────┘

0x20000000 ┌─────────────────────────────┐
           │  SRAM 64 Ko                  │
           │  .data, .bss, stacks, heap   │
           │  ~50 Ko utilises              │
0x2000FFFF └─────────────────────────────┘

Flash SPI externe (SST25VF016B, CS2) :
           ┌─────────────────────────────┐
           │  Zone nouveau programme OTA  │
           │  2 Mbit = 256 Ko             │
           └─────────────────────────────┘
```

### Contraintes immutables du firmware

Ces contraintes sont codees en dur et tout le stack backend s'y conforme. Elles NE DOIVENT PAS changer lors de la migration :

1. **JSON malformes** : cles non-quotees (`{k:613,v:"1"}`)
2. **Content-Type** : `application/json ;charset=UTF-8` (espace avant `;`)
3. **Single-packet TCP** : reponse HTTP en un seul `recv()`
4. **HTTP 201** : tous les POST retournent `201 Created`
5. **`_de67f` first** : premier champ dans les reponses JSON actions
6. **Port 80 only** : le firmware ne peut PAS se connecter a un autre port
7. **Valeurs 8 bits** : chaque indice de Tb_Echange[] est `unsigned char` (0-255)
8. **Table 953 octets** : `Nb_Tbb_Donnees = 953` indices

## Objectif principal

Creer un **plan de migration phased** et les premiers artefacts qui permettent de :

1. **Phase 0** — Compiler le firmware actuel avec GCC (zero changement fonctionnel)
2. **Phase 1** — Remplacer MQX RTOS par un RTOS open-source (FreeRTOS)
3. **Phase 2** — Moderniser les communications (HTTPS, mDNS/Bonjour)
4. **Phase 3** — Ajouter les fonctionnalites planifiees (full status, Home Assistant)

Ce prompt concerne uniquement la **Phase 0**.

## Phase 0 : Compiler le firmware actuel avec GCC

### Objectif

Produire un `BP_MQX_ETH.elf` fonctionnellement identique au binaire CodeWarrior actuel, compile avec `m68k-elf-gcc`.

### Sous-etapes

#### 0.1 — Environnement de build

Creer ou mettre a jour :

- `Makefile` — utilisant une toolchain `m68k-elf-` configurable par prefixe
- `docs/toolchain.md` — installation crosstool-NG, flags GCC, formats de sortie, USBDM

#### 0.2 — Linker script

Creer `mcf52259_flash.ld` en remplacement de l'`intflash.lcf` CodeWarrior.

**Contraintes specifiques a respecter** (issues du firmware reel, pas de l'EVB generique) :

- La table des vecteurs doit etre au debut de la Flash (0x00000000)
- Les sections `.APP_JUMP`, `.APP_CRC`, `.APP_VERSION` doivent etre a des adresses fixes compatibles avec le bootloader existant
- `.text` et `.rodata` en Flash
- `.data` en SRAM avec LMA (Load Memory Address) en Flash
- `.bss` en SRAM, remise a zero au reset
- Pile en haut de SRAM (`0x2000FFFF`)
- La zone `0x7E000-0x7EFFF` est reservee pour la sauvegarde de `Tb_Echange[]` (ne pas ecraser)
- Symboles de link : `_sidata`, `_sdata`, `_edata`, `_sbss`, `_ebss`, `_estack`

#### 0.3 — Startup

Creer `startup_mcf52259.S` en remplacement du CRT MQX.

**Attention** : le startup MQX est plus complexe qu'un bare-metal classique. Il doit :

- Initialiser la table des vecteurs (256 entries)
- Configurer le VBR (aligned 1 MB)
- Copier `.data` de Flash vers SRAM
- Mettre `.bss` a zero
- Initialiser les registres systeme (RAMBAR, FLASHBAR, chip selects)
- Appeler le constructeur BSP MQX (`_bsp_init()` ou equivalent)
- Lancer le scheduler MQX

**Alternative pragmatique** : reutiliser le startup MQX 3.8 existant (fichiers `crt0.s`, `_bsp_cfinit.c`, `bsp_init.c`) et les adapter pour GCC. MQX 3.8 a ete publie en open source par Freescale.

#### 0.4 — Compatibilite MQX 3.8 + GCC

MQX 3.8 inclut un support GCC experimental. Evaluer :

- [ ] Le BSP `m52259evb` compile-t-il avec `m68k-elf-gcc` ?
- [ ] Les headers MQX utilisent-ils des `#pragma` Metrowerks a adapter ?
- [ ] Les sections absolues (`.APP_JUMP`, `.APP_CRC`, `.APP_VERSION`) utilisent `__declspec(far_absolute)` — trouver l'equivalent GCC (`__attribute__((section(...)))`)
- [ ] Le prefixe hongrois (`uc_`, `us_`, `vd_`, `sc_`, etc.) ne pose pas de probleme GCC
- [ ] Les options ABI : eviter `-mshort`, `-malign-int`, `-mrtd` au debut

#### 0.5 — Migration des pragmas Metrowerks

| Pragma CodeWarrior | Equivalent GCC | Fichier(s) concernes |
|-------------------|----------------|---------------------|
| `#pragma section far_absolute R ".APP_JUMP"` | `__attribute__((section(".APP_JUMP")))` | `bootloader.c` |
| `__declspec(far_absolute)` | `__attribute__((section(...)))` | `bootloader.c` |
| `#pragma once` | `#pragma once` (supporte par GCC) | Tous les headers |
| `#pragma options align=packed` | `__attribute__((packed))` | Headers structures |

#### 0.6 — RTCS (pile TCP/IP)

C'est le **point le plus critique** de la Phase 0. RTCS est la pile TCP/IP proprietaire MQX. Options :

| Option | Effort | Risque | Recommandation |
|--------|--------|--------|----------------|
| Compiler RTCS avec GCC (partie de MQX 3.8) | Moyen | Faible si MQX compile | **Privilegier en Phase 0** |
| Remplacer par lwIP | Eleve | API differente, tests requis | Phase 1 |
| Remplacer par FNET | Moyen | API differente, licence Apache 2.0 | Phase 1 alternative |

#### 0.7 — Artefacts de sortie

| Artefact | Commande | Usage |
|----------|----------|-------|
| `BP_MQX_ETH.elf` | `m68k-elf-gcc -T mcf52259_flash.ld` | Debug JTAG/BDM |
| `BP_MQX_ETH.s19` | `m68k-elf-objcopy -O srec --srec-forceS3` | Flash USBDM + OTA serveur |
| `BP_MQX_ETH.bin` | `m68k-elf-objcopy -O binary` | Archive |
| `BP_MQX_ETH.map` | `-Wl,-Map` | Analyse memoire |

#### 0.8 — Documentation

Creer :

- `docs/migration-from-codewarrior.md` — conversion LCF → LD, pragmas, EWL/MSL, pieges
- `docs/toolchain.md` — crosstool-NG, flags, USBDM, formats
- `docs/phase0-checklist.md` — checklist de validation avant flash

## Contraintes techniques

### Flags GCC obligatoires

```
CFLAGS  = -mcpu=52259 -msoft-float -ffreestanding -fno-builtin \
          -ffunction-sections -fdata-sections -fno-common \
          -Os -g3 -Wall -Wextra

LDFLAGS = -mcpu=52259 -nostdlib -Wl,-T,mcf52259_flash.ld \
          -Wl,--gc-sections -Wl,-Map,BP_MQX_ETH.map
```

### Memoire de reference (SC944D, PAS EVB)

| Region | Adresse debut | Taille | Usage |
|--------|--------------|--------|-------|
| Flash interne | `0x00000000` | 512 Ko | Code + donnees const + sauvegarde |
| SRAM interne | `0x20000000` | 64 Ko | .data, .bss, stacks, heap |
| Registres | `0x40000000` | — | Peripheriques MCU |
| Flash SPI externe | Via SPI CS2 | 256 Ko (2 Mbit) | Zone OTA |

### Endianness

**Big-endian** (natif ColdFire m68k). Impact sur : acces registres, protocole I2C (CRC-16), communication reseau, structures packees.

## Ce que tu ne dois pas faire

- Ne pas changer le comportement fonctionnel du firmware (Phase 0 = iso-fonctionnel)
- Ne pas inventer de registres peripheriques — utiliser les headers NXP/MQX existants
- Ne pas ajouter de HAL qui n'existe pas dans le firmware actuel
- Ne pas supposer qu'OpenOCD fonctionne pour cette cible (utiliser USBDM)
- Ne pas masquer les hypotheses : tout point non verifie doit etre documente
- Ne pas toucher au protocole HTTP (contraintes immutables section 3)
- Ne pas modifier la structure de `Tb_Echange[]` (953 indices, format fige)
- Ne pas supprimer le bootloader ou les sections absolues

## Workflow demande

1. Analyse l'etat actuel du depot `essensys-gcc` et du depot `client-essensys-legacy`
2. Consulte la documentation dans `essensys-doc/archi/legacy-client-*.md` et `hardware-sc944d.md`
3. Evalue la compilabilite de MQX 3.8 avec GCC (sources disponibles ?)
4. Cree l'arborescence du projet :
   ```
   essensys-gcc/
   ├── README.md
   ├── Makefile
   ├── mcf52259_flash.ld
   ├── startup_mcf52259.S
   ├── src/main.c                 # Point d'entree minimal (test)
   ├── docs/
   │   ├── toolchain.md
   │   ├── migration-from-codewarrior.md
   │   └── phase0-checklist.md
   └── deep-research-report.md    # Existant
   ```
5. Cree le linker script avec les zones memoire reelles (bootloader, sauvegarde Flash)
6. Cree un startup minimal ColdFire V2 (table vecteurs + init .data/.bss)
7. Cree le Makefile avec targets ELF, S19, BIN, MAP
8. Documente la migration, les hypotheses et les risques

## Validation attendue

Avant de considerer la Phase 0 comme terminee :

- [ ] Le Makefile compile un `main.c` minimal en ELF valide (en theorie)
- [ ] Le linker script respecte le layout memoire du bootloader existant
- [ ] Les sections `.APP_JUMP`, `.APP_CRC`, `.APP_VERSION` sont placees aux bonnes adresses
- [ ] Le startup initialise VBR, .data, .bss
- [ ] Les artefacts S19 et BIN sont generes
- [ ] La documentation liste explicitement tout ce qui n'a pas pu etre verifie sans toolchain/materiel
- [ ] Un plan d'integration de MQX 3.8 est documente (meme si non execute)

## Format de reponse attendu

A la fin du travail, fournir :

1. **Resume** des fichiers crees ou modifies
2. **Hypotheses conservees** (et sources)
3. **Risques** identifies pour la Phase 0
4. **Bloqueurs** potentiels pour l'integration de MQX 3.8 avec GCC
5. **Prochaines etapes** concretes vers un firmware compilable

## Feuille de route complete (pour reference)

```
Phase 0 — Build GCC iso-fonctionnel        ← CE PROMPT
  └─ Compiler MQX 3.8 + firmware avec m68k-elf-gcc
  └─ Valider sur materiel (JTAG/BDM)
  └─ OTA S19 compatible avec l'existant

Phase 1 — Remplacement RTOS
  └─ Migrer MQX 3.8 → FreeRTOS
  └─ Remplacer RTCS → lwIP ou FNET
  └─ Conserver toutes les contraintes HTTP

Phase 2 — Modernisation reseau
  └─ HTTPS via BearSSL/wolfSSL (si memoire suffisante)
  └─ mDNS/Bonjour pour decouverte locale
  └─ DNS mon.essensys.fr → mon.essensys.local
  Ref: essensys-doc/new_feature/firmware-v2-local-fullstatus.md

Phase 3 — Nouvelles fonctionnalites
  └─ Full status (remontee etat reel des BA)
  └─ Integration Home Assistant native (MQTT Discovery)
  └─ Broadcast reseau (SSDP/UPnP)
  Ref: essensys-doc/new_feature/home-assistant-integration.md
       essensys-doc/new_feature/firmware-v2-ethernet-modernization.md
```

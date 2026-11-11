## Context

Premier essai (copie de travail, 11 novembre 2026) : 291 erreurs au départ, 0 après les corrections ci-dessous, puis 4 problèmes à l'édition de liens. Les sources de `client-essensys-legacy` sont identiques au code 099-37 de `essensys-board-SC944D`, aux accents de commentaires près.

## Goals / Non-Goals

**Goals :** un firmware 099-37 lié par GCC, contrôlé statiquement contre la référence.

**Non-Goals :** flash sur carte, identité binaire avec CodeWarrior (impossible avec un autre compilateur), évolution fonctionnelle du firmware.

## Decisions

- **Sources dans `bp/app/`**, copiées depuis `essensys-board-SC944D` (référence 099-37) et converties en UTF-8, car GCC refuse les identifiants accentués en Latin-1. Les `#include` sont corrigés à la casse réelle des fichiers.
- **Types MQX 3.8** par `psptypes_legacy.h`, inclus après `mqx.h` dans un en-tête de compatibilité forcé pour l'application seulement.
- **`-DDEBUG`** : la référence a été construite avec la cible « Int_Flash_Debug » ; tout `EspionRS.c` en dépend.
- **`Tb_Echange_Droits`** : le header définit la table dans un fichier et produit une définition provisoire dans l'autre (`#define extern`). CodeWarrior fusionnait les deux ; on compile les sources applicatives avec `-fcommon`, qui reproduit ce comportement sans toucher au code 099-37.
- **Pilote PWM** : réécrit d'après le pilote d'origine désassemblé depuis la sauvegarde 099-37 (codes `ioctl` 0x1801-0x1803, PWM impairs sur le port TA, champ PAR à 3).
- **Patch MQX** : l'étape de patch échoue désormais bruyamment au lieu d'être ignorée (`|| true`).
- **RTC** : `_rtc_sync_with_mqx` et `RTC_TIME_STRUCT` réécrits au-dessus de l'API RTC de MQX 4.0.
- **`sbrk`** : implémenté sur `_mem_alloc` de MQX, pour `malloc` de newlib.
- **PWM** : petit pilote MQX `pwm:` pour le module PWM du MCF52259, avec l'interface d'origine.
- **Libc** : vérifier la variante multilib choisie (`m5208` aujourd'hui) et forcer celle du MCF52259 si besoin.

## Risks / Trade-offs

- Les écarts MQX 3.8 → 4.0 peuvent changer le comportement des pilotes (I2C, RTC, série) sans erreur de compilation → comparaison avec la référence et essai sur banc avant tout déploiement.
- Le pilote PWM est écrit sans le code d'origine → vérification des registres par test unitaire, puis sur banc.
- Le temps réel peut différer (autre compilateur, autres options) → à mesurer sur banc dans la feature d'écriture.

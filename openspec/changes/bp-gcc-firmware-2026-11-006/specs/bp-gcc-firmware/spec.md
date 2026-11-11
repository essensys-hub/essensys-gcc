## Purpose

Produire le firmware applicatif du BP SC944D avec la chaîne GCC open source, sans dépendre de CodeWarrior, à partir des sources de la version 099-37.

## ADDED Requirements

### Requirement: Compilation des sources 099-37
Le build `build.sh bp` SHALL compiler et lier toutes les sources applicatives 099-37 en un `.elf` et un `.s19`, sans erreur, dans l'image Docker du dépôt.

#### Scenario: Build sous Linux
- **WHEN** la CI lance `build.sh bp` sur un système de fichiers sensible à la casse
- **THEN** le build réussit et produit le `.elf`, le `.s19` et le `.map`

### Requirement: Configuration identique à la cible d'origine
Le build SHALL reproduire les options de la cible CodeWarrior « Int_Flash_Debug » qui affectent le code produit, dont `DEBUG`, ainsi que les périphériques activés (I2C0, FLASHX, console série).

#### Scenario: Espion série présent
- **WHEN** le firmware est lié
- **THEN** les fonctions de l'espion série, dont `vd_EspionMainActive`, sont présentes dans le `.map`

### Requirement: Pilote PWM de la sirène
Le firmware SHALL commander la sortie PWM de la sirène par un pilote réel du module PWM du MCF52259, avec la même interface (`fopen("pwm:")`, configuration, activation de la broche, changement du rapport cyclique).

#### Scenario: Interface conservée
- **WHEN** `Hard.c` ouvre `pwm:` puis appelle les trois `ioctl`
- **THEN** chaque appel atteint le pilote et programme les registres PWM attendus, vérifiés par un test unitaire sur hôte

### Requirement: Artefact compatible avec le bootloader
Le `.s19` SHALL tenir dans la zone application `0x3000`-`0x7DFFF`, porter un en-tête au format du bootloader en `0x3000` et un CRC-16 valide sur `0x3002`-`0x7DFFF`.

#### Scenario: Contrôle automatique
- **WHEN** le build se termine
- **THEN** un script vérifie la plage d'adresses, l'en-tête et le CRC, et échoue sinon

### Requirement: Comparaison avec la référence 099-37
Le build SHALL produire un rapport comparant les tâches MQX et les symboles applicatifs du `.map` GCC à la carte mémoire CodeWarrior 099-37.

#### Scenario: Écarts signalés
- **WHEN** un symbole applicatif de la référence manque dans le `.map` GCC
- **THEN** le rapport le liste comme écart

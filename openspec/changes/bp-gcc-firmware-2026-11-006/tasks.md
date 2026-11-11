## 1. Portage du firmware BP vers GCC (#16)

- [x] 1.1 Intégrer les sources 099-37 dans `bp/app/` (UTF-8, casse des `#include`) et les brancher dans `bp/Makefile`
- [x] 1.2 Activer I2C0, FLASHX et la console série dans la configuration MQX, ajouter `-DDEBUG`
- [x] 1.3 Couche de compatibilité MQX 3.8 : types hérités, RTC, `sbrk`
- [x] 1.4 Corriger la double définition de `Tb_Echange_Droits`
- [x] 1.5 Écrire le pilote PWM du MCF52259 et son test unitaire sur hôte
- [x] 1.6 Vérifier et corriger la variante multilib de la libc (GCC associe -mcpu=52259 à la variante m5208, même ISA_A+ : rien à corriger)
- [x] 1.7 Script de contrôle de l'artefact : plage d'adresses, en-tête `0x3000`, CRC
- [x] 1.8 Rapport de comparaison `.map` GCC / `.xMAP` CodeWarrior 099-37
- [ ] 1.9 Build vert en CI Linux, puis `/checkup`

## 2. Documentation (essensys-doc#41)

- [ ] 2.1 Guide « compiler le firmware du BP sans CodeWarrior » et écarts connus dans `essensys-doc`

## 3. Mémoire du projet (essensys-memory#39)

- [ ] 3.1 Consigner le change et les pièges du portage dans `essensys-memory`

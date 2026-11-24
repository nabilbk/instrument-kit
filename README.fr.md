# Kit d'instruments IDEV

Cadrans, bandes défilantes, horizon artificiel et voyants d'alerte avec **un seul modèle de
données pour deux moteurs d'affichage** : Qt Quick (Qt 6) et LVGL 9 (C, pour ESP32 et STM32).

On décrit un instrument une fois dans `spec/instruments.json` (bornes, unité, zones
vert/orange/rouge, seuils d'alerte avec hystérésis, amortissement) ; la construction génère une
table C utilisée à l'identique par les widgets LVGL et par le module QML `IdevInstruments`.

- Composants : `RoundGauge`, `LinearTape`, `AttitudeIndicator`, `Annunciator`.
- Démos 800 × 480 : combiné de bord automobile et écran de vol principal (PFD), dans les deux moteurs.
- Captures : `docs/screenshots/` ; vérifications : `docs/VERIFICATION.md`.
- Spécification (stories, critères Étant donné / Quand / Alors, exigences chiffrées, feuille de
  route) : `docs/SPEC.md` ; API : `docs/API.md`.

Construction : voir le [README anglais](README.md#build).

**Ce kit n'est pas un instrument certifié** : il sert au prototypage et à la démonstration.

Licences : code du kit sous MIT ; LVGL sous MIT ; Qt sous LGPLv3 ou licence commerciale
(le kit n'utilise que des modules LGPLv3).

# Vérification v0.1 — cahier de recette

*Tess (qualité) — 18/11/2026. Tout ce qui figure ici a été exécuté ; ce qui ne l'a pas été est
signalé comme tel.*

## 1. Environnement réellement utilisé

| Élément | Valeur |
|---|---|
| Machine | VM Linux x86_64, 2 vCPU Intel Xeon 2,3 GHz, 7 Go RAM, sans écran ni GPU |
| Compilateurs | GCC 13.3, CMake 3.28, Ninja |
| LVGL | v9.2.2 (clone GitHub), `LV_COLOR_DEPTH 32`, rendu logiciel, mode `DIRECT` dans un tampon mémoire, export PNG par libpng |
| Qt | **6.8.3 compilé depuis les sources** (qtbase + qtshadertools + qtdeclarative, sans Qt Quick Controls, sans OpenGL), plateforme `offscreen`, moteur `QT_QUICK_BACKEND=software` |

PySide6 n'a pas pu être installé (accès à pypi.org refusé par le proxy de l'environnement) :
Qt a donc été compilé depuis les dépôts GitHub `qt/qtbase`, `qt/qtshadertools`,
`qt/qtdeclarative` (tag v6.8.3). Avantage : le module C++ et les démos Qt ont réellement été
**compilés et exécutés**, pas seulement chargés en QML pur.

## 2. Résultats

| Vérification | Commande | Résultat |
|---|---|---|
| Construction LVGL, `-Wall -Wextra -Werror` | `cmake --preset lvgl-headless && cmake --build --preset lvgl-headless` | OK, 0 avertissement |
| Construction Qt, `-Wall -Wextra -Werror` | `cmake --preset qt -DCMAKE_PREFIX_PATH=/opt/qt6 && cmake --build --preset qt` | OK, 0 avertissement |
| qmllint sur les 3 modules QML | `cmake --build build/qt --target all_qmllint` | OK, 0 avertissement |
| Tests CTest, préréglage LVGL | `ctest --preset lvgl-headless` | 9/9 réussis |
| Tests CTest, préréglage Qt | `ctest --preset qt` | 7/7 réussis |
| Avertissements QML à l'exécution | lancement des 2 démos (nominal et `--invalid`) | aucun |
| ASan + UBSan, côté LVGL | build Debug `-fsanitize=address,undefined`, `ctest` + rendu des 2 démos à 9 instants | 9/9 réussis, aucune erreur après correction (voir § 4) |
| ASan + UBSan, côté Qt (notre code instrumenté, Qt non instrumenté) | idem, `ctest` | 7/7 réussis, aucune erreur |

### Couverture des critères d'acceptation (SPEC.md)

| Critère | Test | Statut |
|---|---|---|
| S1.1 table générée, partagée | build des deux moteurs depuis le même JSON ; `idev_core_tests` (lookup) ; `tst_instrument::configFromSpec` | OK |
| S1.2 spec invalide rejetée | `spec_rejects_overlap`, `spec_rejects_out_of_range`, `spec_rejects_duplicate` | OK |
| S1.3 id inconnu | `lvgl_selftest` (NULL, mauvais type) ; `tst_instrument::unknownConfigId` | OK |
| S2.1–S2.3 cadran, ratio, butée | `idev_core_tests` (ratio, format, butée) ; captures | OK |
| S3.1–S3.3 amortissement | `idev_core_tests::test_damping` ; `lvgl_selftest` ; `tst_instrument::dampingConverges` | OK |
| S4.1–S4.3 hystérésis | `idev_core_tests::test_alerts_hysteresis` | OK |
| S5.1–S5.3 bande | captures PFD (155 kt, étiquettes, zones, case) | OK, contrôle visuel |
| S6.1–S6.4 horizon | captures PFD nominal (t = 8 s), virage 52° (t = 19,5 s) | OK, contrôle visuel |
| S7.1–S7.4 voyant, clignotement, acquittement | `idev_core_tests::test_annunciator` ; `lvgl_selftest` (2 Hz, clic, réarmement) ; `tst_instrument::annunciatorBlinkAndAcknowledge` | OK |
| S8.1–S8.3 donnée invalide | `idev_core_tests::test_invalid` ; captures `*-invalid.png` | OK |
| S9.1–S9.2 deux moteurs, 800 × 480 | captures ci-dessous, même instant de scénario | OK (écarts : voir § 4) |

### Mesures (sur la VM, **pas sur cible**)

| Mesure | Commande | Résultat |
|---|---|---|
| Tas LVGL par widget (64 bits, allocateur intégré, objet + état + minuteur) | `idev_lvgl_headless --memreport` | RoundGauge 472 o ; LinearTape 408 o ; AttitudeIndicator 408 o ; Annunciator 408 o — exigence NF2 (≤ 512 o) tenue sur PC |
| Temps de rendu LVGL, combiné 800 × 480, 300 trames | `idev_lvgl_headless --demo car --time 20 --bench 300` | trame mise à jour : 1,38 ms en moyenne (max 2,37 ms) ; écran complet : 1,06 ms |
| Temps de rendu LVGL, PFD 800 × 480, 300 trames | `idev_lvgl_headless --demo pfd --time 20 --bench 300` | trame mise à jour : 0,48 ms (max 0,67 ms) ; écran complet : 0,43 ms |

Ces temps viennent d'un Xeon à 2,3 GHz : ils ne disent **rien** du respect de NF1 (60 i/s) sur
ESP32-S3 ou STM32, où le rendu logiciel et surtout le transfert vers l'écran coûtent bien plus.
La mesure sur carte est prévue en v0.2. Aucune mesure de fréquence d'affichage n'a été faite
côté Qt (le moteur logiciel hors écran n'est pas représentatif d'une cible avec GPU).

## 3. Captures

Toutes en 800 × 480, produites par les commandes indiquées, et relues à l'œil une par une.

| Démo | Qt Quick | LVGL 9 |
|---|---|---|
| Combiné, alertes (t = 26 s : 150 km/h LIMIT, TEMP en alarme, FUEL en attention) | `screenshots/qt-car-cluster.png` | `screenshots/lvgl-car-cluster.png` |
| Combiné, nominal (t = 12 s) | `screenshots/qt-car-cluster-nominal.png` | `screenshots/lvgl-car-cluster-nominal.png` |
| Combiné, capteurs RPM et TEMP perdus (`--invalid`) | `screenshots/qt-car-cluster-invalid.png` | `screenshots/lvgl-car-cluster-invalid.png` |
| PFD, virage à 52° (t = 19,5 s, BANK) | `screenshots/qt-cockpit-pfd.png` | `screenshots/lvgl-cockpit-pfd.png` |
| PFD, nominal AP engagé (t = 8 s) | `screenshots/qt-cockpit-pfd-nominal.png` | `screenshots/lvgl-cockpit-pfd-nominal.png` |
| PFD, décrochage (t = 33,5 s, STALL) | `screenshots/qt-cockpit-pfd-stall.png` | `screenshots/lvgl-cockpit-pfd-stall.png` |
| PFD, tangage et altitude perdus (`--invalid`) | `screenshots/qt-cockpit-pfd-invalid.png` | `screenshots/lvgl-cockpit-pfd-invalid.png` |

Commandes : `idev_lvgl_headless --demo car|pfd --time T [--invalid] --png F` et
`QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software idev_qt_car_cluster|idev_qt_cockpit_pfd --time T [--invalid] --grab F`.

## 4. Défauts trouvés pendant la recette et corrigés

| Défaut | Correction |
|---|---|
| Qt : `AircraftSymbol` et `AttitudeIndicator` refusés au chargement (« Cannot override FINAL property ») : propriétés nommées `left` et `top` | Renommées `isLeft`, `topY` |
| Qt : graduations de la bande non positionnées (« Unable to assign [undefined] to x ») : l'id `scale` masqué par la propriété `Item.scale` | id renommé `scaleArea` |
| Qt : qmllint ne trouvait pas le module `IdevDemoSim` (dossier ≠ URI) | `QT_QML_OUTPUT_DIRECTORY` commun, dépendance `QtQuick` déclarée, `pragma ComponentBehavior: Bound` |
| Deux moteurs : barreaux ±20° de l'échelle de tangage qui chevauchaient l'échelle de roulis | Barreaux limités à 80 % du rayon de l'échelle de roulis (S6.3) |
| Deux moteurs : étiquette de bande à moitié cachée sous la case de lecture | Étiquette omise si elle tombe sous la case (S5.3) |
| LVGL (UBSan) : débordement d'entier signé dans `lv_draw_sw_mask.c` de LVGL, provoqué par les coordonnées géantes du quadrilatère « sol » de l'horizon | Géométrie bornée au cercle circonscrit à la vue ; sol tracé en un seul triangle (supprime aussi une couture visible entre deux triangles) |
| LVGL : test du clignotement trop strict vis-à-vis de la période de 50 ms du minuteur | Instant de contrôle placé au milieu de la demi-période ; latence de phase ≤ 50 ms documentée |

## 5. Écarts connus entre moteurs (acceptés pour v0.1)

- Polices : Montserrat bitmap (LVGL) contre police système (Qt). Les étiquettes d'échelle de
  tangage sont tournées avec l'horizon en Qt, droites en LVGL.
- Les valeurs à l'instant t peuvent différer de ±1 unité : LVGL rejoue le scénario en temps
  virtuel exact, Qt en temps réel (minuteur de 16 ms).
- Le moteur de rendu Qt utilisé est le moteur logiciel ; le moteur RHI (GPU) n'a pas été essayé.

## 6. Non vérifié en v0.1

- Aucun essai sur matériel (ESP32-S3, STM32, carte Linux embarqué), donc NF1 non vérifiée.
- Le simulateur interactif SDL2 (`IDEV_LVGL_SDL=ON`, `lvgl/platform/sdl_main.c`) **n'a pas été
  compilé** : les en-têtes de développement SDL2 ne sont pas installés sur la VM.
- Pas de test Qt Quick Test des composants QML (v0.2), pas de comparaison automatique de
  captures, pas de couverture de code mesurée, pas de TSan ni Valgrind.
- Pas d'essai sous Windows ni macOS.

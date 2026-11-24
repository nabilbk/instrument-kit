# Kit d'instruments IDEV — spécification v0.1

*Rédaction : Sofia (analyse métier) — relecture architecture : Viktor — 18/11/2026*
*Statut : v0.1 livrée en interne (produit IDEV, pas de client signataire).*

## 1. Contexte

IDEV vend un forfait « Prototype d'IHM embarquée ». Les demandes observées sur Upwork
reviennent à trois familles d'écrans :

- simulateur de cockpit : pages d'écran multifonction (horizon artificiel, bandes de
  vitesse et d'altitude, alarmes) ;
- HUD ou petit afficheur sur ESP32 ;
- combiné de bord automobile (compte-tours, vitesse, voyants).

Le client hésite souvent entre Qt Quick (Linux embarqué avec GPU) et LVGL (microcontrôleur).
Nous n'avons trouvé aucun kit d'instruments qui offre **le même modèle de données sur les deux
moteurs**. Le kit sert de vitrine : il montre en une démo qu'on sait livrer les deux, et il
raccourcit nos prototypes (on part d'instruments testés au lieu de partir de zéro).

**Besoin, pas solution :** l'intégrateur doit décrire un instrument une seule fois (bornes,
unités, zones, seuils, amortissement) et obtenir le même comportement, quel que soit le moteur
d'affichage choisi.

## 2. Acteurs

| Acteur | Attente |
|---|---|
| Intégrateur (développeur chez le client, ou IDEV) | Décrire les instruments en JSON, brancher ses données, compiler pour sa cible. |
| Utilisateur final (pilote en simulateur, conducteur) | Lire la valeur et l'état d'alerte d'un coup d'œil. |
| Commercial IDEV (Nabil) | Montrer la même démo en Qt et en LVGL, captures à l'appui. |

## 3. Périmètre v0.1 (MoSCoW)

| Priorité | Élément |
|---|---|
| **Must** | Modèle de données commun décrit une fois (`spec/instruments.json`) et consommé par les deux moteurs |
| **Must** | 4 composants dans chaque moteur : RoundGauge, LinearTape, AttitudeIndicator, Annunciator |
| **Must** | Zones vert/orange/rouge, seuils d'alerte avec hystérésis, amortissement du 1er ordre |
| **Must** | État « donnée invalide » visible (valeur `---`, aiguille masquée, drapeau ATT) |
| **Must** | 2 démos (combiné auto, PFD de cockpit) dans chaque moteur, avec générateur de données simulées |
| **Must** | Compilation CMake sur PC et rendu sans écran (PNG) pour la vérification |
| **Should** | Acquittement d'une alarme par appui sur le voyant |
| **Should** | Mesure de l'empreinte mémoire LVGL par widget, mesure du temps de rendu sur PC |
| **Could** | Simulateur interactif SDL2 pour LVGL |
| **Won't (v0.1)** | Voir § 6 Hors périmètre |

## 4. Stories et critères d'acceptation

Chaque critère porte un identifiant ; le plan de vérification (`docs/VERIFICATION.md`) indique
le test qui le couvre. Un critère sans test n'est pas livré.

### S1 — Décrire un instrument une seule fois
*En tant qu'intégrateur, je décris un instrument en JSON pour que Qt et LVGL l'affichent à
l'identique.*

- **S1.1** Étant donné `spec/instruments.json` valide, quand je lance la construction CMake,
  alors une table C est générée et compilée dans `idev_core`, utilisée par les widgets LVGL
  et par le type QML `Instrument` (même `configId`).
- **S1.2** Étant donné un fichier où une zone dépasse `[min, max]`, où deux zones se
  chevauchent ou où un identifiant est dupliqué, quand je construis, alors la génération
  échoue avec un message qui nomme l'instrument fautif.
- **S1.3** Étant donné un `configId` inconnu, quand je crée le composant, alors LVGL renvoie
  `NULL` et Qt émet un avertissement (`unknownConfigId`) sans planter.

### S2 — Lire une valeur sur un cadran à zones (RoundGauge)
- **S2.1** Étant donné `car.rpm` (0–8000, zones vert < 6000, orange 6000–6500, rouge > 6500),
  quand la valeur vaut 4000, alors l'aiguille est à mi-course de l'arc (ratio 0,5) et la
  lecture numérique affiche `4000`.
- **S2.2** Étant donné `labelScale = 0,001`, quand le cadran est dessiné, alors les
  graduations majeures portent `0 … 8` et la légende `RPM x1000`.
- **S2.3** Étant donné une valeur hors bornes (9000), quand elle est reçue, alors l'aiguille
  reste en butée (8000) et l'alerte est évaluée sur la valeur brute (alarme).

### S3 — Amortir l'aiguille
- **S3.1** Étant donné `dampingMs = 80` et une valeur affichée à 1000, quand la consigne passe
  à 5000, alors après 80 ms l'affichage vaut 1000 + 63,2 % × 4000 (± 5).
- **S3.2** Étant donné la première valeur valide (au démarrage ou après une perte), quand elle
  arrive, alors l'affichage y saute directement, sans balayage depuis une valeur périmée.
- **S3.3** Étant donné un affichage arrivé à la consigne, alors le minuteur d'amortissement
  s'arrête (aucun réveil CPU tant que rien ne bouge).

### S4 — Être alerté sans papillotement
- **S4.1** Étant donné les seuils `cautionHigh = 6000`, `warningHigh = 6500`,
  `hysteresis = 100`, quand la valeur monte à 6600, alors le niveau passe à « alarme »
  immédiatement.
- **S4.2** Quand elle redescend à 6450, alors le niveau reste « alarme » ; à 6390, il passe à
  « attention » ; à 5890, il revient à « normal ».
- **S4.3** Étant donné des seuils bas (`car.fuel` : attention ≤ 20 %, alarme ≤ 10 %), le même
  comportement s'applique dans l'autre sens.

### S5 — Lire une bande défilante (LinearTape)
- **S5.1** Étant donné `pfd.airspeed` (plage visible 80 kt), quand la valeur vaut 155, alors
  la case de lecture au centre affiche `155`, les graduations de 120 à 190 kt sont visibles
  (une tous les 5 kt, majeure tous les 10 kt) et les étiquettes 120, 140 et 180 sont lisibles
  (160 est omise car elle tomberait sous la case, voir S5.3).
- **S5.2** Les zones colorées sont tracées le long du bord gradué ; la case et son pointeur
  prennent la couleur du niveau d'alerte.
- **S5.3** Aucune étiquette n'est dessinée à moitié sous la case de lecture.

### S6 — Lire l'assiette (AttitudeIndicator)
- **S6.1** Étant donné roulis 0 et tangage +5°, alors la ligne d'horizon est sous le symbole
  avion de 5 × (hauteur / plage visible) pixels.
- **S6.2** Étant donné un roulis à droite de 30°, alors l'horizon et le pointeur de roulis
  tournent de 30° dans le sens antihoraire (convention « sky pointer »).
- **S6.3** Les barreaux de l'échelle de tangage restent à l'intérieur de l'échelle de roulis.
- **S6.4** Étant donné `|roulis| ≥ 45°`, alors le pointeur de roulis passe au rouge et le
  voyant BANK s'allume (démo PFD).

### S7 — Voyant d'alerte (Annunciator)
- **S7.1** Étant donné le niveau normal, alors le voyant est éteint (texte grisé) ; s'il est
  `active` (ex. AP engagé), il est allumé vert fixe.
- **S7.2** Étant donné le niveau attention, alors il est allumé orange fixe.
- **S7.3** Étant donné le niveau alarme non acquittée, alors il clignote en rouge à
  `blinkHz` (2 Hz par défaut, rapport cyclique 50 %) ; tous les voyants clignotent en phase.
- **S7.4** Quand l'utilisateur appuie sur le voyant, alors l'alarme est acquittée et le voyant
  reste rouge fixe ; une nouvelle hausse de niveau relance le clignotement.

### S8 — Voir qu'une donnée est perdue (état d'erreur)
- **S8.1** Étant donné une valeur `NaN`, alors la lecture affiche `---`, l'aiguille d'un
  cadran est masquée, la bande n'affiche plus de graduations et sa case affiche `---` en rouge.
- **S8.2** Étant donné un roulis ou un tangage invalide, alors l'horizon est remplacé par un
  fond neutre et un drapeau rouge `ATT`.
- **S8.3** Une donnée invalide ne déclenche pas d'alarme de dépassement (niveau normal).

### S9 — Démontrer les deux moteurs
- **S9.1** Étant donné le même instant de scénario (ex. t = 26 s), quand je produis les
  captures Qt et LVGL d'une démo, alors elles montrent les mêmes valeurs, zones et voyants
  (le rendu des polices peut différer).
- **S9.2** Les démos font 800 × 480 px, la résolution d'écran la plus courante dans les
  demandes (dalles 5"/7" RGB pour ESP32-S3 et Linux embarqué).

## 5. Exigences non fonctionnelles

| Id | Exigence | Chiffre | Statut v0.1 |
|---|---|---|---|
| NF1 | Fréquence d'affichage visée sur cible | **60 i/s** (période de rafraîchissement 16 ms) pour une démo 800 × 480 sur Linux embarqué avec GPU (Qt Quick) ; **60 i/s** pour un instrument isolé ≤ 320 × 320 sur ESP32-S3 (LVGL, rafraîchissement partiel) ; ≥ 30 i/s pour l'écran complet 800 × 480 sur ESP32-S3 | **Non mesuré sur cible** (pas de matériel). Temps de rendu mesuré sur PC seulement, voir VERIFICATION.md |
| NF2 | Empreinte tas LVGL par widget | ≤ **512 octets** par widget (objet LVGL + état + minuteur), aucun tampon d'image par widget | Mesuré sur PC 64 bits : 408 à 472 o. Sur cible 32 bits, attendu plus bas (à confirmer) |
| NF3 | Données de configuration | En flash (`const`), 0 octet de RAM, pas d'analyse JSON sur la cible | Fait (table C générée) |
| NF4 | CPU au repos | Aucun minuteur actif quand toutes les valeurs sont stables et qu'aucune alarme ne clignote | Fait (minuteurs mis en pause) |
| NF5 | Latence d'alerte | Changement de niveau d'alerte visible à la trame suivante (≤ 16 ms après réception) | Par construction (évaluation à la réception) |
| NF6 | Plateformes | Qt 6.8 LTS ou plus ; LVGL 9.2 (testé), C99 ; ESP32-S3 et STM32 avec FPU visés | PC Linux seulement en v0.1 |
| NF7 | Licence | Code du kit sous MIT ; LVGL MIT ; Qt LGPLv3 ou commercial, sans module GPL | Fait |
| NF8 | Langues | Libellés des instruments dans le JSON ; textes QML fixes via `qsTr()` | Fait |

## 6. Hors périmètre v0.1

- Certification ou conformité (DO-178C, ISO 26262, EN 50128) : le kit est un outil de
  prototypage et de démonstration, **pas un instrument de vol ni un composant de sécurité**.
- Qt for MCUs, Slint, Qt Widgets.
- Acquisition de données réelles (CAN, ARINC 429, MAVLink, capteurs) : l'intégrateur pousse
  ses valeurs dans `value` / `idev_instrument_widget_set_value()`.
- Indicateur de cap (HSI), bande de cap, variomètre, bandes horizontales, bargraphes.
- Thèmes modifiables à l'exécution, mode nuit, rotation d'écran.
- Interaction tactile autre que l'acquittement d'un voyant.
- Analyse du JSON à l'exécution sur la cible.
- Mesures sur matériel réel (ESP32-S3, STM32, carte Linux embarqué) : prévues en v0.2.

## 7. Risques et questions ouvertes

| Risque / question | Parade |
|---|---|
| 60 i/s plein écran sur ESP32-S3 800 × 480 irréaliste sans PSRAM rapide et double tampon | Mesurer en v0.2 sur carte (esp_lvgl_port, tampon 10–25 % en SRAM interne, double tampon) ; ajuster NF1 avec un chiffre mesuré |
| Rendu des polices différent entre Qt et LVGL (Montserrat bitmap contre police système) | Accepté pour v0.1 ; option v0.2 : embarquer Montserrat dans le module QML |
| La démo Qt n'a été rendue qu'avec le moteur logiciel (`QT_QUICK_BACKEND=software`) | Vérifier en v0.2 avec le moteur RHI (OpenGL/Vulkan) sur cible |
| Le kit pourrait être pris pour un instrument certifié | Avertissement explicite dans le README et ici (§ 6) |
| Question : faut-il une API C++ pure (sans QObject) pour Qt Widgets ? | À trancher selon la demande client |

## 8. Feuille de route

| Version | Contenu |
|---|---|
| **v0.1** (cette livraison) | Modèle commun, 4 composants × 2 moteurs, 2 démos × 2 moteurs, captures, tests du cœur C |
| **v0.2** | Portage ESP32-S3 (esp_lvgl_port, sdkconfig de performance) avec mesure i/s et RAM sur carte ; Qt Quick Test pour les composants QML ; comparaison automatique des captures en CI ; police Montserrat côté Qt |
| **v0.3** | Bande de cap / HSI, variomètre, bargraphes ; thème jour/nuit ; adaptateurs de données (CAN, MAVLink) en exemple |
| **v1.0** | API stabilisée, documentation de référence, paquet ESP-IDF Component Registry et module Qt installable |

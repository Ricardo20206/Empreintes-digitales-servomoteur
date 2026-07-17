# Schéma de câblage — JM-101B (6 fils) + SG90 + Arduino Mega 2560

## Schéma physique (visuel)

Ouvrez **[schema_cablage_physique.svg](schema_cablage_physique.svg)** pour le diagramme complet.

---

## Une seule masse — pas deux !

Le JM-101B n'a **qu'une seule masse** : le fil **ROUGE**.

Le fil **NOIR** n'est **pas** une masse : c'est le **RX** (réception des données série depuis l'Arduino). C'est une confusion fréquente, car le noir sert souvent à la masse sur d'autres composants.

À l'intérieur du module, la masse d'alimentation et la masse signal sont déjà reliées — vous n'avez qu'un seul fil GND à connecter (le rouge).

---

## Les 6 fils de votre capteur

Ordre sur le connecteur (de haut en bas, comme sur votre module) :

| # | Couleur | Fonction | Arduino Mega | Obligatoire ? |
|---|---------|----------|--------------|---------------|
| 1 | **Rouge** | GND (masse unique) | **GND** | Oui |
| 2 | **Jaune** | TX — module envoie | **Pin 19 (RX1)** | Oui |
| 3 | **Blanc** | TouchVin — alim. détection doigt | **3.3V** *(ou laisser libre)* | Non |
| 4 | **Bleu** | Touch — sortie détection doigt | *Non connecté* | Non |
| 5 | **Vert** | VCC — alimentation 3,3 V | **3.3V** | Oui |
| 6 | **Noir** | RX — module reçoit | **Pin 18 (TX1)** | Oui |

### Fils indispensables (4 sur 6)

Pour faire fonctionner le capteur avec notre firmware, seuls **4 fils** sont nécessaires :

```
  VERT   → 3.3V
  ROUGE  → GND
  JAUNE  → Pin 19 (RX1)
  NOIR   → Pin 18 (TX1)
```

Les fils **blanc** et **bleu** servent à la détection tactile basse consommation. Notre firmware interroge le capteur en continu, donc **vous pouvez les laisser non connectés**.

Option recommandée pour le blanc : le brancher aussi sur **3.3V** (comme le vert) pour activer le circuit tactile — sans impact sur le fonctionnement actuel.

---

## Schéma général corrigé

```
  JM-101B (6 fils)                         ARDUINO MEGA 2560
  ─────────────────                        ───────────────────

  VERT  (VCC 3,3V)  ─────────────────────► 3.3V
  ROUGE (GND)       ─────────────────────► GND  ◄── masse commune
  JAUNE (TX)        ─────────────────────► Pin 19 (RX1)
  NOIR  (RX)        ◄───────────────────── Pin 18 (TX1)
  BLANC (TouchVin)  ──► 3.3V  (optionnel)
  BLEU  (Touch out)     (non connecté)

  SG90                                     ARDUINO MEGA 2560
  ────                                     ───────────────────
  ROUGE  (+5V)      ─────────────────────► 5V
  MARRON (GND)      ─────────────────────► GND
  ORANGE (signal)   ─────────────────────► Pin 9

  LED verte                                ARDUINO MEGA 2560
  ─────────                                ───────────────────
  Anode (+)  ──► résistance 220 Ω ───────► Pin 6
  Cathode (−) ───────────────────────────► GND

  LED rouge                                ARDUINO MEGA 2560
  ─────────                                ───────────────────
  Anode (+)  ──► résistance 220 Ω ───────► Pin 7
  Cathode (−) ───────────────────────────► GND

  Buzzer (passif / piezo)                  ARDUINO MEGA 2560
  ───────────────────────                  ───────────────────
  Signal (+) ────────────────────────────► Pin 8
  GND (−)    ────────────────────────────► GND
```

### LEDs et buzzer

| Composant | Broche Mega | Comportement |
|-----------|-------------|--------------|
| **LED verte** | **Pin 6** | Allumée si empreinte reconnue (pendant l'ouverture du servo) |
| **LED rouge** | **Pin 7** | Allumée pendant l'alarme (empreinte inconnue) |
| **Buzzer** | **Pin 8** | Siffle **10 s** si empreinte fausse ; s'arrête si empreinte correcte pendant ce temps |

Chaque LED : **anode** (patte longue) → résistance **220 Ω** → broche Arduino ; **cathode** (patte courte) → **GND**.

Buzzer piezo passif : fil signal → **pin 8**, autre fil → **GND**. (Si buzzer actif à 5 V : + → pin 8, − → GND.)

---

## Règle TX / RX

```
  Capteur JAUNE (TX)  ──►  Arduino Pin 19 (RX1)
  Capteur NOIR  (RX)  ◄──  Arduino Pin 18 (TX1)
```

Si le message `Capteur introuvable` apparaît → **inversez jaune et noir** (pas blanc).

---

## Servomoteur SG90

| Fil | Fonction | Arduino Mega |
|-----|----------|--------------|
| **Rouge** | +5 V | **5V** (ou alim. externe) |
| **Marron** | GND | **GND** |
| **Orange** | Signal PWM | **Pin 9** |

---

## Alimentation

Le SG90 peut tirer jusqu'à **650 mA** en pic. Alimentation externe **5 V ≥ 1 A** recommandée pour le servo, avec **masse commune** vers l'Arduino :

```
  Alim 5V externe
       (+) ──► Rouge du SG90
       (−) ──► GND Arduino (= Rouge capteur + Marron servo)
```

---

## Paramètres de communication

| Interface | Vitesse | Broches |
|-----------|---------|---------|
| Capteur (Serial1) | 57600 baud | TX1 = 18, RX1 = 19 |
| Moniteur USB | 115200 baud | USB |

---

## Vérification

1. Câbler les **4 fils essentiels** (vert, rouge, jaune, noir).
2. Téléverser le firmware, Moniteur Série à **115200 baud**.
3. Message attendu : `Capteur detecte.`
4. Si erreur → inverser **jaune ↔ noir**.
5. Brancher le servo, enregistrer une empreinte avec `e1`.

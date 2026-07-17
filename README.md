# Lecteur d'empreintes — Arduino Mega + JM-101B + SG90

Système d'identification biométrique avec servomoteur, LEDs, alarme sonore et **commande Bluetooth** depuis un téléphone Android.

| Composant | Broche / liaison | Rôle |
|-----------|------------------|------|
| **Arduino Mega 2560** | — | Contrôleur principal |
| **JM-101B** (AS608) | Serial1 (18 / 19) | Capteur d'empreintes |
| **Tower Pro SG90** | Pin **9** | Ouverture / fermeture |
| **LED verte** | Pin **6** | Accès autorisé |
| **LED rouge** | Pin **7** | Accès refusé |
| **Buzzer** | Pin **8** | Alarme 10 s si empreinte fausse |
| **HC-05 Bluetooth** | Serial2 (16 / 17) | Commande servo depuis le téléphone |

---

## Sommaire

1. [Fonctionnement](#fonctionnement)
2. [Contrôle depuis le téléphone](#contrôle-depuis-le-téléphone)
3. [Structure du projet](#structure-du-projet)
4. [Prérequis logiciels](#prérequis-logiciels)
5. [Câblage](#câblage)
6. [Compiler et téléverser](#compiler-et-téléverser)
7. [Enregistrer des empreintes](#enregistrer-des-empreintes)
8. [Commandes disponibles](#commandes-disponibles)
9. [Tester le système](#tester-le-système)
10. [Dépannage](#dépannage)
11. [Paramètres modifiables](#paramètres-modifiables)

---

## Fonctionnement

1. Posez un doigt sur le capteur JM-101B **ou** envoyez une commande depuis le téléphone / le PC.
2. Le module capture l'image et la compare à sa mémoire (mode **1:N**), ou exécute la commande reçue.
3. **Empreinte reconnue** :
   - LED **verte** allumée
   - Servo ouvert à **90°** pendant **5 secondes**
   - Puis servo refermé et LEDs éteintes
   - Si le buzzer était en cours → **arrêt immédiat**
4. **Empreinte inconnue** :
   - LED **rouge** allumée
   - **Buzzer** qui siffle pendant **10 secondes**
   - Servo reste fermé
   - Pendant ces 10 s : empreinte **correcte** ou commande **`a`** (téléphone) arrête le buzzer
5. **Commande téléphone `o`** : servo ouvert (reste ouvert jusqu'à **`f`**)

Les empreintes sont stockées **dans le capteur** (FLASH), pas sur l'Arduino. Elles survivent à un redémarrage ou à un nouveau firmware, tant que la base n'est pas vidée.

---

## Contrôle depuis le téléphone

Guide détaillé : [`docs/BLUETOOTH.md`](docs/BLUETOOTH.md)

### Matériel

- Module **HC-05**
- Smartphone **Android** (le HC-05 n'est **pas** compatible iPhone)
- App : **Serial Bluetooth Terminal** (Play Store)

### Câblage HC-05

| HC-05 | Arduino Mega |
|-------|--------------|
| VCC | **5V** |
| GND | **GND** |
| TX | **Pin 17** (RX2) |
| RX | **Pin 16** (TX2) |

### Utilisation

1. Téléverser le firmware : `pio run -t upload`
2. Apparier **HC-05** (PIN souvent **1234** ou **0000**)
3. Ouvrir Serial Bluetooth Terminal → connecter HC-05
4. Envoyer (avec retour à la ligne) :

| Commande | Action |
|----------|--------|
| `o` ou `ouvrir` | Ouvre le servo (reste ouvert) |
| `f` ou `fermer` | Ferme le servo |
| `a` | Arrête le buzzer |
| `h` | Aide |

---

## Structure du projet

```
Lecteur d'empreintes/
├── platformio.ini
├── src/main.cpp
├── firmware/LecteurEmpreintes/LecteurEmpreintes.ino
├── tools/serial_cli.py
├── docs/
│   ├── CABLAGE.md
│   ├── BLUETOOTH.md              # Contrôle téléphone
│   └── schema_cablage_physique.svg
├── .vscode/tasks.json
└── README.md
```

---

## Prérequis logiciels

### 1. PlatformIO

- Extension **PlatformIO IDE** dans Cursor / VS Code  
- Ou PlatformIO Core (`pio --version`)

### 2. Python 3 + pyserial

```powershell
pip install -r tools/requirements.txt
```

Ou : **Terminal → Exécuter la tâche… → `Setup: Installer dependances Python`**

### 3. Bibliothèque Adafruit Fingerprint

Installée automatiquement par PlatformIO.

Avec l'**Arduino IDE** :

1. Ouvrir `firmware/LecteurEmpreintes/LecteurEmpreintes.ino`
2. **Croquis → Inclure une bibliothèque → Gérer les bibliothèques**
3. Chercher **Adafruit Fingerprint** → Installer
4. Carte : **Arduino Mega 2560**

---

## Câblage

Documentation complète : [`docs/CABLAGE.md`](docs/CABLAGE.md)  
Schéma visuel : [`docs/schema_cablage_physique.svg`](docs/schema_cablage_physique.svg)

### Vue d'ensemble

```
  JM-101B          Arduino Mega 2560          Accessoires
  ───────          ─────────────────          ───────────
  VERT  ─────────► 3.3V
  ROUGE ─────────► GND ◄───────────────────── masses communes
  JAUNE ─────────► Pin 19 (RX1)
  NOIR  ◄───────── Pin 18 (TX1)
  BLANC (opt.) ──► 3.3V
  BLEU  (libre)

                   Pin 9  ◄── Orange SG90
                   5V     ◄── Rouge  SG90
                   GND    ◄── Marron SG90

                   Pin 6  ◄── LED verte (+ 220 Ω)
                   Pin 7  ◄── LED rouge (+ 220 Ω)
                   Pin 8  ◄── Buzzer (signal)
                   Pin 17 ◄── HC-05 TX
                   Pin 16 ──► HC-05 RX
                   5V/GND ◄── HC-05 VCC/GND
                   GND    ◄── cathodes LEDs + GND buzzer
```

### Capteur JM-101B (6 fils)

> **Une seule masse** : le fil **rouge**. Le fil **noir** est le **RX**, pas une masse.

| Fil | Fonction | Arduino Mega | Obligatoire |
|-----|----------|--------------|-------------|
| **Vert** | VCC 3,3 V | **3.3V** | Oui |
| **Rouge** | GND | **GND** | Oui |
| **Jaune** | TX (module → Arduino) | **Pin 19 (RX1)** | Oui |
| **Noir** | RX (Arduino → module) | **Pin 18 (TX1)** | Oui |
| **Blanc** | TouchVin | 3.3V ou libre | Non |
| **Bleu** | Touch (sortie) | Non connecté | Non |

```
Jaune (TX capteur)  →  Pin 19 (RX1)
Noir  (RX capteur)  ←  Pin 18 (TX1)
```

Si `Capteur introuvable` → **inverser jaune ↔ noir**.

### Servo SG90

| Fil | Arduino Mega |
|-----|--------------|
| Rouge | **5V** (ou alim. externe 5 V ≥ 1 A) |
| Marron | **GND** |
| Orange | **Pin 9** |

### LEDs

| LED | Broche | Montage |
|-----|--------|---------|
| Verte | **Pin 6** | Anode (+) → **220 Ω** → pin 6 · Cathode (−) → GND |
| Rouge | **Pin 7** | Anode (+) → **220 Ω** → pin 7 · Cathode (−) → GND |

Pattes : longue = anode (+), courte = cathode (−).

### Buzzer

| Fil | Arduino Mega |
|-----|--------------|
| Signal (+) | **Pin 8** |
| GND (−) | **GND** |

Buzzer piezo **passif** recommandé (le firmware utilise `tone()` à 2500 Hz).  
Buzzer actif 5 V : + → pin 8, − → GND (son continu au lieu d'un sifflement modulé).

### Bluetooth HC-05

| HC-05 | Arduino Mega |
|-------|--------------|
| VCC | **5V** |
| GND | **GND** |
| TX | **Pin 17** (RX2) |
| RX | **Pin 16** (TX2) |

Voir [`docs/BLUETOOTH.md`](docs/BLUETOOTH.md).

### Alimentation recommandée

Le SG90 peut tirer jusqu'à ~650 mA en pic :

- Arduino alimenté par **USB**
- Servo sur source **5 V ≥ 1 A** externe
- **Masse commune** entre alim. externe, Arduino, capteur, servo, LEDs et buzzer

---

## Compiler et téléverser

Branchez l'Arduino Mega en USB.

### Depuis Cursor

| Action | Comment |
|--------|---------|
| **Téléverser** | `Ctrl+Shift+B` ou tâche **`Arduino: Televerser`** |
| Compiler seulement | Tâche **`Arduino: Compiler`** |
| Moniteur série | Tâche **`Arduino: Moniteur serie`** |

### Depuis le terminal

À la racine du projet :

```powershell
# Compiler
pio run

# Téléverser
pio run -t upload

# Port explicite (exemple)
pio run -t upload --upload-port COM7

# Lister les ports
pio device list

# Moniteur série (115200 baud)
pio device monitor
```

### Message attendu au démarrage

```
=== Lecteur d'empreintes JM-101B + Servo SG90 ===
Capteur detecte.
Empreintes enregistrees : 0 / 162

Posez votre doigt pour vous identifier.
Commandes : e<ID> enregistrer | d<ID> supprimer | c vider | l stats
```

Si `ERREUR : capteur introuvable` → [Dépannage](#dépannage).

---

## Enregistrer des empreintes

Trois méthodes. Dans tous les cas : **deux lectures du même doigt**.

### Procédure commune

```
1. Lancer l'enregistrement pour un ID (ex. 1)
2. Poser le doigt          → "Image capturee."
3. Retirer le doigt        → "Retirez le doigt..."
4. Reposer le MÊME doigt   → seconde capture
5. Succès                  → "Empreinte n1 enregistree avec succes."
```

Conseils :

- Toujours le **même doigt** (index recommandé)
- Doigt bien centré, pression légère
- Attendre la LED du capteur entre les étapes
- IDs utilisables : **1 à 127**

---

### Méthode A — Moniteur série

1. Ouvrir : `pio device monitor` (ou tâche **Arduino: Moniteur serie**)
2. Vérifier `Capteur detecte.`
3. Taper **`e1`** puis Entrée
4. Suivre les instructions à l'écran
5. Autre personne : **`e2`**, etc.

---

### Méthode B — Script Python

> Un seul programme à la fois sur le port COM.  
> **Fermez le moniteur série** avant d'utiliser le script.

```powershell
python tools/serial_cli.py ports
python tools/serial_cli.py enregistrer --id 1
python tools/serial_cli.py enregistrer --id 2
python tools/serial_cli.py enregistrer --id 1 --port COM7
```

Ou : **Terminal → Exécuter la tâche… → `Empreinte: Enregistrer`**

---

### Méthode C — Tâches Cursor

| Tâche | Action |
|-------|--------|
| **Empreinte: Enregistrer** | Enregistre un ID (demande le numéro) |
| **Empreinte: Lister** | Nombre d'empreintes |
| **Empreinte: Supprimer** | Supprime un ID |
| **Empreinte: Moniteur Python** | Moniteur série via Python |

---

## Commandes disponibles

### Moniteur série Arduino (115200 baud)

| Commande | Action | Exemple |
|----------|--------|---------|
| `o` / `ouvrir` | Ouvrir le servo (reste ouvert) | `o` |
| `f` / `fermer` | Fermer le servo | `f` |
| `a` | Arrêter le buzzer / alarme | `a` |
| `e<ID>` | Enregistrer une empreinte | `e1`, `e12` |
| `d<ID>` | Supprimer une empreinte | `d3` |
| `l` | Afficher le nombre d'empreintes | `l` |
| `c` | **Vider toute la base** (irréversible) | `c` |
| `h` | Aide | `h` |

Ces commandes fonctionnent sur **USB (115200)** et **Bluetooth HC-05 (9600)**.

### Script Python `tools/serial_cli.py`

| Commande | Action |
|----------|--------|
| `python tools/serial_cli.py ports` | Lister les ports COM |
| `python tools/serial_cli.py enregistrer --id 1` | Enregistrer l'ID 1 |
| `python tools/serial_cli.py supprimer --id 2` | Supprimer l'ID 2 |
| `python tools/serial_cli.py liste` | Statistiques |
| `python tools/serial_cli.py vider` | Vider la base (confirmation) |
| `python tools/serial_cli.py monitor` | Moniteur série |

Option : `--port COM7` (ou `-p COM7`).

### PlatformIO

| Commande | Action |
|----------|--------|
| `pio run` | Compiler |
| `pio run -t upload` | Téléverser |
| `pio device list` | Lister les ports |
| `pio device monitor` | Moniteur série |

---

## Tester le système

1. Téléverser le firmware (`pio run -t upload`).
2. Enregistrer une empreinte (`e1`).
3. Poser le **doigt enregistré** :
   - `Acces autorise`
   - LED **verte**
   - Servo ouvert 5 s puis refermé
4. Poser un **doigt non enregistré** :
   - `Acces refuse`
   - LED **rouge** + **buzzer** 10 s
   - Servo inchangé
5. Pendant le buzzer, poser le **doigt enregistré** :
   - Buzzer **arrêté immédiatement**
   - LED verte + servo ouvert

---

## Dépannage

| Problème | Solution |
|----------|----------|
| `Capteur introuvable` | Vérifier 3.3V (vert) et GND (rouge) ; **inverser jaune ↔ noir** |
| Port COM « Accès refusé » | Fermer moniteur série / Arduino IDE / autre script |
| `Les deux empreintes ne correspondent pas` | Recommencer `e1`, même doigt, bien centré |
| `Emplacement deja occupe` | Autre ID (`e2`) ou supprimer (`d1`) |
| Servo ne bouge pas | Pin 9, 5 V, GND ; alim. externe recommandée |
| Arduino redémarre avec le servo | Alim. 5 V externe pour le servo |
| LED ne s'allume pas | Polarité (anode = patte longue) + résistance 220 Ω |
| Buzzer ne sonne pas | Vérifier pin 8 et GND ; buzzer passif préférable |
| Bluetooth : rien ne se passe | Inverser TX/RX HC-05 ; baud 9600 ; appairage OK |
| iPhone + HC-05 | Non compatible — utiliser Android |
| `pyserial manquant` | `pip install -r tools/requirements.txt` |

---

## Paramètres modifiables

Dans `src/main.cpp` (ou `firmware/LecteurEmpreintes/LecteurEmpreintes.ino`) :

```cpp
#define SERVO_PIN            9      // PWM servo
#define LED_VERTE_PIN        6      // LED accès OK
#define LED_ROUGE_PIN        7      // LED accès refusé
#define BUZZER_PIN           8      // buzzer alarme
#define SERVO_FERME          0      // angle fermé
#define SERVO_OUVERT         90     // angle ouvert
#define TEMPS_OUVERTURE_MS   5000   // durée ouverture (ms)
#define TEMPS_BUZZER_MS      10000  // durée alarme (ms)
#define FREQ_BUZZER_HZ       2500   // fréquence du sifflement
```

Puis : `pio run -t upload`.

---

## Communication série

| Liaison | Vitesse | Broches |
|---------|---------|---------|
| Capteur JM-101B ↔ Mega | **57600** baud | Serial1 : TX1=18, RX1=19 |
| PC ↔ Mega (USB) | **115200** baud | USB natif |
| Téléphone ↔ HC-05 ↔ Mega | **9600** baud | Serial2 : TX2=16, RX2=17 |

---

## Licence / usage

Projet pédagogique / DIY.  
Capteur JM-101B compatible avec la [Adafruit Fingerprint Sensor Library](https://github.com/adafruit/Adafruit-Fingerprint-Sensor-Library).

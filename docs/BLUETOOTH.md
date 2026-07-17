# Contrôle du servo depuis le téléphone (HC-05)

Le firmware écoute les commandes sur **USB** et sur le module **Bluetooth HC-05**.

## Matériel requis

- Module **HC-05** (Bluetooth classique)
- Smartphone **Android** (iPhone non compatible HC-05)
- App gratuite : **Serial Bluetooth Terminal** (ou équivalent)

## Câblage rapide

| HC-05 | Arduino Mega |
|-------|--------------|
| VCC | 5V |
| GND | GND |
| TX | Pin **17** (RX2) |
| RX | Pin **16** (TX2) — idéalement via diviseur 1kΩ + 2kΩ |

Détails : [`CABLAGE.md`](CABLAGE.md)

## Configuration téléphone (Android)

1. Téléverser le firmware mis à jour (`pio run -t upload`).
2. Allumer le Mega + HC-05 (LED HC-05 qui clignote = en attente).
3. Sur le téléphone : **Paramètres → Bluetooth** → associer **HC-05**
   - Code PIN souvent : **1234** ou **0000**
4. Ouvrir **Serial Bluetooth Terminal**.
5. Menu → **Devices** → choisir **HC-05**.
6. Envoyer les commandes (avec retour à la ligne / NL) :

| Commande | Action |
|----------|--------|
| `o` ou `ouvrir` | Ouvre le servo (reste ouvert jusqu'à `f`) |
| `f` ou `fermer` | Ferme le servo |
| `a` | Arrête le buzzer / alarme |
| `h` | Affiche l'aide |
| `e1` | Enregistrer empreinte n°1 |
| `l` | Stats empreintes |

## Comportement

- **`o`** : LED verte + servo ouvert → reste ouvert jusqu'à **`f`**
- Empreinte correcte : ouverture **automatique 5 s** puis fermeture (comme avant)
- Empreinte fausse : buzzer 10 s ; depuis le téléphone tapez **`a`** ou posez l'empreinte OK

## Dépannage

| Problème | Solution |
|----------|----------|
| HC-05 invisible | Alim. 5 V / GND ; LED allumée ; mode appairage |
| Connecté mais rien | Inverser TX/RX ; baud **9600** dans l'app |
| iPhone | HC-05 incompatible — prévoir ESP8266/WiFi |
| PIN refusé | Essayer 1234 puis 0000 |

/*
 * LecteurEmpreintes — Arduino Mega 2560 + JM-101B + SG90 + LEDs + buzzer + HC-05
 *
 * USB (Serial, 115200) et Bluetooth HC-05 (Serial2, 9600) :
 *   e<ID>  Enregistrer une empreinte (ex: e1)
 *   d<ID>  Supprimer une empreinte (ex: d3)
 *   c      Vider toute la base
 *   l      Afficher les statistiques
 *   o      Ouvrir le servo (telephone / PC)
 *   f      Fermer le servo
 *   a      Arreter l'alarme buzzer
 *   h      Aide
 *
 * Empreinte fausse  → LED rouge + buzzer 10 s
 * Empreinte correcte pendant l'alarme → buzzer arrete + acces OK
 */

#include <Adafruit_Fingerprint.h>
#include <Servo.h>

#define SERVO_PIN            9
#define LED_VERTE_PIN        6
#define LED_ROUGE_PIN        7
#define BUZZER_PIN           8
#define SERVO_FERME          0
#define SERVO_OUVERT         90
#define TEMPS_OUVERTURE_MS   5000
#define TEMPS_BUZZER_MS      10000
#define FREQ_BUZZER_HZ       2500
#define BT_BAUD              9600

Adafruit_Fingerprint capteur(&Serial1);
Servo servo;

bool alarmeActive = false;
unsigned long alarmeDebut = 0;
bool porteOuverteManuelle = false;
unsigned long ouvertureAutoDebut = 0;
bool ouvertureAutoActive = false;

void enregistrerEmpreinte(uint8_t id);
void supprimerEmpreinte(uint8_t id);
void viderBase();
void afficherStats();
void ouvrirPorte();
void fermerPorte();
void eteindreLeds();
void demarrerAlarme();
void arreterAlarme();
void gererAlarme();
void gererOuvertureAuto();
void accesAutorise();
void ouvrirDepuisCommande(bool autoFermeture);
void fermerDepuisCommande();
void traiterLigne(String ligne, Stream &sortie);
void traiterCommandesPorts();
void envoyerVersTous(const __FlashStringHelper *msg);
void afficherAide(Stream &sortie);
bool detecterCapteur();

bool capteurOk = false;

void setup() {
  Serial.begin(115200);
  while (!Serial) { ; }

  // HC-05 sur Serial2 : TX2=16, RX2=17
  Serial2.begin(BT_BAUD);

  pinMode(LED_VERTE_PIN, OUTPUT);
  pinMode(LED_ROUGE_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  eteindreLeds();
  noTone(BUZZER_PIN);

  Serial.println(F("\n=== Lecteur d'empreintes JM-101B + Servo SG90 ==="));
  Serial.println(F("Bluetooth HC-05 pret (Serial2 @ 9600 baud)."));
  Serial.println(F("Recherche du capteur..."));

  // Le JM-101B met ~200-500 ms a demarrer apres mise sous tension
  delay(800);

  capteurOk = detecterCapteur();

  if (!capteurOk) {
    Serial.println(F("ERREUR : capteur introuvable."));
    Serial.println(F("A faire dans l'ordre :"));
    Serial.println(F("  1) INVERSER jaune <-> noir (pin 19 / pin 18)"));
    Serial.println(F("  2) Vert = 3.3V (ou essayer 5V), Rouge = GND"));
    Serial.println(F("  3) Masse commune Mega + capteur"));
    Serial.println(F("  4) Rebrancher le connecteur a fond"));
    Serial.println(F("Le servo Bluetooth (o/f) reste utilisable."));
    digitalWrite(LED_ROUGE_PIN, HIGH);
  } else {
    Serial.println(F("Capteur detecte."));
    afficherStats();
  }

  servo.attach(SERVO_PIN);
  fermerPorte();

  Serial.println(F("\nPosez votre doigt ou utilisez le telephone (o/f)."));
  Serial.println(F("Commandes : o ouvrir | f fermer | a alarme off | e<ID> | d<ID> | c | l | h\n"));
  Serial2.println(F("HC-05 pret. Tapez h pour l'aide."));
}

bool detecterCapteur() {
  const uint32_t bauds[] = {57600UL, 9600UL, 115200UL, 38400UL, 19200UL};

  for (uint8_t b = 0; b < 5; b++) {
    Serial1.end();
    delay(50);
    Serial1.begin(bauds[b]);
    delay(100);
    capteur.begin(bauds[b]);
    delay(300);

    for (uint8_t essai = 0; essai < 3; essai++) {
      if (capteur.verifyPassword()) {
        Serial.print(F("  OK a "));
        Serial.print(bauds[b]);
        Serial.println(F(" baud"));
        return true;
      }
      delay(150);
    }
    Serial.print(F("  Echec a "));
    Serial.print(bauds[b]);
    Serial.println(F(" baud"));
  }
  return false;
}

void loop() {
  traiterCommandesPorts();
  gererAlarme();
  gererOuvertureAuto();

  if (!capteurOk) {
    return;
  }

  uint8_t resultat = capteur.getImage();
  if (resultat != FINGERPRINT_OK) {
    return;
  }

  resultat = capteur.image2Tz();
  if (resultat != FINGERPRINT_OK) {
    return;
  }

  resultat = capteur.fingerFastSearch();
  if (resultat == FINGERPRINT_OK) {
    Serial.print(F("Acces autorise — empreinte n"));
    Serial.print(capteur.fingerID);
    Serial.print(F(" (confiance : "));
    Serial.print(capteur.confidence);
    Serial.println(F(")"));
    Serial2.print(F("OK empreinte n"));
    Serial2.println(capteur.fingerID);
    accesAutorise();
  } else if (resultat == FINGERPRINT_NOTFOUND) {
    Serial.println(F("Acces refuse — empreinte inconnue."));
    Serial2.println(F("REFUS empreinte inconnue"));
    if (!alarmeActive) {
      demarrerAlarme();
    }
  }
}

void traiterCommandesPorts() {
  if (Serial.available()) {
    String ligne = Serial.readStringUntil('\n');
    traiterLigne(ligne, Serial);
  }
  if (Serial2.available()) {
    String ligne = Serial2.readStringUntil('\n');
    traiterLigne(ligne, Serial2);
  }
}

void traiterLigne(String ligne, Stream &sortie) {
  ligne.trim();
  ligne.toLowerCase();
  if (ligne.length() == 0) {
    return;
  }

  // Accepter aussi "ouvrir" / "fermer"
  if (ligne == "ouvrir") {
    ligne = "o";
  } else if (ligne == "fermer") {
    ligne = "f";
  }

  char cmd = ligne.charAt(0);

  if (cmd == 'h' || cmd == '?') {
    afficherAide(sortie);
    return;
  }
  if (cmd == 'o') {
    sortie.println(F("Ouverture servo (telephone/PC)..."));
    ouvrirDepuisCommande(false);
    return;
  }
  if (cmd == 'f') {
    sortie.println(F("Fermeture servo..."));
    fermerDepuisCommande();
    return;
  }
  if (cmd == 'a') {
    arreterAlarme();
    sortie.println(F("Alarme arretee."));
    return;
  }
  if (cmd == 'l') {
    afficherStats();
    return;
  }
  if (cmd == 'c') {
    viderBase();
    return;
  }
  if (cmd == 'e' || cmd == 'd') {
    uint8_t id = (uint8_t)ligne.substring(1).toInt();
    if (id == 0) {
      sortie.println(F("ID invalide (1-127)."));
      return;
    }
    if (cmd == 'e') {
      enregistrerEmpreinte(id);
    } else {
      supprimerEmpreinte(id);
    }
    return;
  }

  sortie.println(F("Commande inconnue. Tapez h pour l'aide."));
}

void afficherAide(Stream &sortie) {
  sortie.println(F("--- Aide ---"));
  sortie.println(F("o / ouvrir  Ouvrir le servo"));
  sortie.println(F("f / fermer  Fermer le servo"));
  sortie.println(F("a           Arreter buzzer/alarme"));
  sortie.println(F("e1          Enregistrer empreinte n1"));
  sortie.println(F("d1          Supprimer empreinte n1"));
  sortie.println(F("l           Stats empreintes"));
  sortie.println(F("c           Vider la base"));
  sortie.println(F("h           Cette aide"));
}

void ouvrirDepuisCommande(bool autoFermeture) {
  arreterAlarme();
  porteOuverteManuelle = !autoFermeture;
  ouvertureAutoActive = autoFermeture;
  if (autoFermeture) {
    ouvertureAutoDebut = millis();
  }
  digitalWrite(LED_ROUGE_PIN, LOW);
  digitalWrite(LED_VERTE_PIN, HIGH);
  ouvrirPorte();
  envoyerVersTous(F("Servo OUVERT"));
}

void fermerDepuisCommande() {
  porteOuverteManuelle = false;
  ouvertureAutoActive = false;
  fermerPorte();
  eteindreLeds();
  envoyerVersTous(F("Servo FERME"));
}

void accesAutorise() {
  arreterAlarme();
  porteOuverteManuelle = false;
  ouvertureAutoActive = true;
  ouvertureAutoDebut = millis();
  digitalWrite(LED_ROUGE_PIN, LOW);
  digitalWrite(LED_VERTE_PIN, HIGH);
  Serial.println(F("LED verte allumee"));
  ouvrirPorte();
}

void gererOuvertureAuto() {
  if (!ouvertureAutoActive) {
    return;
  }
  if (millis() - ouvertureAutoDebut >= TEMPS_OUVERTURE_MS) {
    ouvertureAutoActive = false;
    if (!porteOuverteManuelle) {
      fermerPorte();
      eteindreLeds();
      Serial.println(F("Porte refermee."));
      Serial2.println(F("Porte refermee."));
    }
  }
}

void demarrerAlarme() {
  alarmeActive = true;
  alarmeDebut = millis();
  digitalWrite(LED_VERTE_PIN, LOW);
  digitalWrite(LED_ROUGE_PIN, HIGH);
  tone(BUZZER_PIN, FREQ_BUZZER_HZ);
  Serial.println(F("Buzzer actif 10 s — posez l'empreinte correcte pour arreter."));
  Serial2.println(F("ALARME 10s — empreinte OK ou tapez a"));
}

void arreterAlarme() {
  alarmeActive = false;
  noTone(BUZZER_PIN);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_ROUGE_PIN, LOW);
}

void gererAlarme() {
  if (!alarmeActive) {
    return;
  }
  if (millis() - alarmeDebut >= TEMPS_BUZZER_MS) {
    arreterAlarme();
    Serial.println(F("Alarme terminee (10 s)."));
    Serial2.println(F("Alarme terminee."));
  }
}

void ouvrirPorte() {
  servo.write(SERVO_OUVERT);
  Serial.println(F("Servo -> OUVERT"));
}

void fermerPorte() {
  servo.write(SERVO_FERME);
}

void eteindreLeds() {
  digitalWrite(LED_VERTE_PIN, LOW);
  digitalWrite(LED_ROUGE_PIN, LOW);
}

void envoyerVersTous(const __FlashStringHelper *msg) {
  Serial.println(msg);
  Serial2.println(msg);
}

void enregistrerEmpreinte(uint8_t id) {
  arreterAlarme();
  Serial.print(F("Enregistrement empreinte n"));
  Serial.print(id);
  Serial.println(F(" — posez le doigt..."));
  Serial2.print(F("Enregistrement n"));
  Serial2.println(id);

  int resultat = -1;
  while (resultat != FINGERPRINT_OK) {
    resultat = capteur.getImage();
    switch (resultat) {
      case FINGERPRINT_OK:
        Serial.println(F("  Image capturee."));
        break;
      case FINGERPRINT_NOFINGER:
        break;
      default:
        Serial.println(F("  Erreur capture, reessayez."));
        delay(800);
        break;
    }
  }

  resultat = capteur.image2Tz(1);
  if (resultat != FINGERPRINT_OK) {
    Serial.println(F("Echec conversion image."));
    return;
  }

  Serial.println(F("Retirez le doigt..."));
  delay(2000);
  while (capteur.getImage() != FINGERPRINT_NOFINGER) { delay(100); }

  Serial.println(F("Reposez le meme doigt..."));
  while (capteur.getImage() != FINGERPRINT_OK) { delay(100); }

  resultat = capteur.image2Tz(2);
  if (resultat != FINGERPRINT_OK) {
    Serial.println(F("Echec seconde capture."));
    return;
  }

  resultat = capteur.createModel();
  if (resultat != FINGERPRINT_OK) {
    Serial.println(F("Les deux empreintes ne correspondent pas."));
    return;
  }

  resultat = capteur.storeModel(id);
  if (resultat == FINGERPRINT_OK) {
    Serial.print(F("Empreinte n"));
    Serial.print(id);
    Serial.println(F(" enregistree avec succes."));
    Serial2.println(F("Empreinte OK"));
  } else if (resultat == FINGERPRINT_BADLOCATION) {
    Serial.println(F("Emplacement deja occupe."));
  } else {
    Serial.println(F("Erreur lors de l'enregistrement."));
  }
}

void supprimerEmpreinte(uint8_t id) {
  uint8_t resultat = capteur.deleteModel(id);
  if (resultat == FINGERPRINT_OK) {
    Serial.print(F("Empreinte n"));
    Serial.print(id);
    Serial.println(F(" supprimee."));
  } else {
    Serial.println(F("Suppression impossible."));
  }
}

void viderBase() {
  uint8_t resultat = capteur.emptyDatabase();
  if (resultat == FINGERPRINT_OK) {
    Serial.println(F("Base d'empreintes videe."));
  } else {
    Serial.println(F("Erreur lors du vidage."));
  }
}

void afficherStats() {
  capteur.getParameters();
  Serial.print(F("Empreintes enregistrees : "));
  Serial.print(capteur.templateCount);
  Serial.print(F(" / "));
  Serial.println(capteur.capacity);
  Serial2.print(F("Empreintes : "));
  Serial2.print(capteur.templateCount);
  Serial2.print(F("/"));
  Serial2.println(capteur.capacity);
}

/*
 * LecteurEmpreintes.ino
 * Arduino Mega 2560 + capteur JM-101B (AS608) + servo SG90 + LEDs + buzzer
 *
 * Bibliothèque requise : Adafruit Fingerprint Sensor Library
 *
 * Fonctionnement :
 *   - Empreinte reconnue  → LED verte + servo ouvert 5 s
 *   - Empreinte inconnue  → LED rouge + buzzer 10 s
 *   - Empreinte correcte pendant le buzzer → arrêt immédiat + accès OK
 *
 * Commandes série (115200 baud) :
 *   e<ID>  Enregistrer | d<ID> Supprimer | c Vider | l Stats
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
#define FREQ_BUZZER_HZ       5000

Adafruit_Fingerprint capteur(&Serial1);
Servo servo;

bool alarmeActive = false;
unsigned long alarmeDebut = 0;

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
void accesAutorise();
void traiterCommandeSerie();

void setup() {
  Serial.begin(115200);
  while (!Serial) { ; }

  pinMode(LED_VERTE_PIN, OUTPUT);
  pinMode(LED_ROUGE_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  eteindreLeds();
  noTone(BUZZER_PIN);

  Serial1.begin(57600);
  capteur.begin(57600);

  Serial.println(F("\n=== Lecteur d'empreintes JM-101B + Servo SG90 ==="));

  if (!capteur.verifyPassword()) {
    Serial.println(F("ERREUR : capteur introuvable."));
    Serial.println(F("  → Vérifiez le câblage TX/RX (fil jaune ↔ pin 19, fil noir ↔ pin 18)."));
    Serial.println(F("  → Vérifiez l'alimentation 3,3 V (fil vert) et la masse (fil rouge)."));
    digitalWrite(LED_ROUGE_PIN, HIGH);
    while (true) { delay(1000); }
  }

  Serial.println(F("Capteur détecté."));
  afficherStats();

  servo.attach(SERVO_PIN);
  fermerPorte();

  Serial.println(F("\nPlacez votre doigt sur le capteur pour vous identifier."));
  Serial.println(F("Commandes admin : e<ID> enregistrer | d<ID> supprimer | c vider | l stats\n"));
}

void loop() {
  traiterCommandeSerie();
  gererAlarme();

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
    Serial.print(F("Accès autorisé — empreinte n°"));
    Serial.print(capteur.fingerID);
    Serial.print(F(" (confiance : "));
    Serial.print(capteur.confidence);
    Serial.println(F(")"));
    accesAutorise();
  } else if (resultat == FINGERPRINT_NOTFOUND) {
    Serial.println(F("Accès refusé — empreinte inconnue."));
    if (!alarmeActive) {
      demarrerAlarme();
    }
  }
}

void accesAutorise() {
  arreterAlarme();
  digitalWrite(LED_ROUGE_PIN, LOW);
  digitalWrite(LED_VERTE_PIN, HIGH);
  Serial.println(F("LED verte allumée"));
  ouvrirPorte();
  delay(TEMPS_OUVERTURE_MS);
  fermerPorte();
  eteindreLeds();
  Serial.println(F("Porte refermée. En attente..."));
}

void demarrerAlarme() {
  alarmeActive = true;
  alarmeDebut = millis();
  digitalWrite(LED_VERTE_PIN, LOW);
  digitalWrite(LED_ROUGE_PIN, HIGH);
  tone(BUZZER_PIN, FREQ_BUZZER_HZ);
  Serial.println(F("Buzzer actif 10 s — posez l'empreinte correcte pour arrêter."));
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
    Serial.println(F("Alarme terminée (10 s)."));
  }
}

void ouvrirPorte() {
  servo.write(SERVO_OUVERT);
  Serial.println(F("Servo → OUVERT"));
}

void fermerPorte() {
  servo.write(SERVO_FERME);
}

void eteindreLeds() {
  digitalWrite(LED_VERTE_PIN, LOW);
  digitalWrite(LED_ROUGE_PIN, LOW);
}

void traiterCommandeSerie() {
  if (!Serial.available()) {
    return;
  }

  String ligne = Serial.readStringUntil('\n');
  ligne.trim();
  if (ligne.length() == 0) {
    return;
  }

  char cmd = ligne.charAt(0);
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
      Serial.println(F("ID invalide (1–127)."));
      return;
    }
    if (cmd == 'e') {
      enregistrerEmpreinte(id);
    } else {
      supprimerEmpreinte(id);
    }
  }
}

void enregistrerEmpreinte(uint8_t id) {
  arreterAlarme();
  Serial.print(F("Enregistrement empreinte n°"));
  Serial.print(id);
  Serial.println(F(" — posez le doigt..."));

  int resultat = -1;
  while (resultat != FINGERPRINT_OK) {
    resultat = capteur.getImage();
    switch (resultat) {
      case FINGERPRINT_OK:
        Serial.println(F("  Image capturée."));
        break;
      case FINGERPRINT_NOFINGER:
        break;
      default:
        Serial.println(F("  Erreur capture, réessayez."));
        delay(800);
        break;
    }
  }

  resultat = capteur.image2Tz(1);
  if (resultat != FINGERPRINT_OK) {
    Serial.println(F("Échec conversion image."));
    return;
  }

  Serial.println(F("Retirez le doigt..."));
  delay(2000);
  while (capteur.getImage() != FINGERPRINT_NOFINGER) { delay(100); }

  Serial.println(F("Reposez le même doigt..."));
  while (capteur.getImage() != FINGERPRINT_OK) { delay(100); }

  resultat = capteur.image2Tz(2);
  if (resultat != FINGERPRINT_OK) {
    Serial.println(F("Échec seconde capture."));
    return;
  }

  resultat = capteur.createModel();
  if (resultat != FINGERPRINT_OK) {
    Serial.println(F("Les deux empreintes ne correspondent pas."));
    return;
  }

  resultat = capteur.storeModel(id);
  if (resultat == FINGERPRINT_OK) {
    Serial.print(F("Empreinte n°"));
    Serial.print(id);
    Serial.println(F(" enregistrée avec succès."));
  } else if (resultat == FINGERPRINT_BADLOCATION) {
    Serial.println(F("Emplacement déjà occupé."));
  } else {
    Serial.println(F("Erreur lors de l'enregistrement."));
  }
}

void supprimerEmpreinte(uint8_t id) {
  uint8_t resultat = capteur.deleteModel(id);
  if (resultat == FINGERPRINT_OK) {
    Serial.print(F("Empreinte n°"));
    Serial.print(id);
    Serial.println(F(" supprimée."));
  } else {
    Serial.println(F("Suppression impossible (ID inexistant ?)."));
  }
}

void viderBase() {
  uint8_t resultat = capteur.emptyDatabase();
  if (resultat == FINGERPRINT_OK) {
    Serial.println(F("Base d'empreintes vidée."));
  } else {
    Serial.println(F("Erreur lors du vidage."));
  }
}

void afficherStats() {
  capteur.getParameters();
  Serial.print(F("Empreintes enregistrées : "));
  Serial.print(capteur.templateCount);
  Serial.print(F(" / "));
  Serial.println(capteur.capacity);
}

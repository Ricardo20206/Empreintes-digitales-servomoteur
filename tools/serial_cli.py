#!/usr/bin/env python3
"""Interface série pour le lecteur d'empreintes Arduino Mega."""

from __future__ import annotations

import argparse
import sys
import time

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    print("pyserial manquant. Installez avec : pip install -r tools/requirements.txt")
    sys.exit(1)

BAUD = 115200
TIMEOUT = 0.5

MOTS_cles_arduino = ("arduino", "ch340", "cp210", "ftdi", "usb-serial", "wch")


def trouver_port(explicite: str | None) -> str:
    if explicite:
        return explicite

    ports = list(list_ports.comports())
    if not ports:
        print("Aucun port COM detecte. Branchez l'Arduino Mega.")
        sys.exit(1)

    for port in ports:
        desc = f"{port.description} {port.manufacturer or ''}".lower()
        if any(mot in desc for mot in MOTS_cles_arduino):
            print(f"Port auto-detecte : {port.device} ({port.description})")
            return port.device

    print("Ports disponibles :")
    for i, port in enumerate(ports, 1):
        print(f"  [{i}] {port.device} — {port.description}")

    if len(ports) == 1:
        print(f"Un seul port trouve, utilisation de {ports[0].device}")
        return ports[0].device

    choix = input("Numero du port a utiliser : ").strip()
    try:
        return ports[int(choix) - 1].device
    except (ValueError, IndexError):
        print("Choix invalide.")
        sys.exit(1)


def ouvrir_port(port_name: str) -> serial.Serial:
    try:
        ser = serial.Serial(port_name, BAUD, timeout=TIMEOUT)
    except serial.SerialException as exc:
        print(f"Impossible d'ouvrir {port_name} : {exc}")
        print("Fermez le Moniteur Serie Arduino ou tout autre programme utilisant ce port.")
        sys.exit(1)

    time.sleep(2)  # laisse le temps au Mega de redemarrer apres l'ouverture du port
    ser.reset_input_buffer()
    return ser


def lire_lignes(ser: serial.Serial, duree: float = 0.3) -> list[str]:
    lignes: list[str] = []
    fin = time.time() + duree
    while time.time() < fin:
        raw = ser.readline()
        if raw:
            lignes.append(raw.decode("utf-8", errors="replace").rstrip())
    return lignes


def envoyer_commande(ser: serial.Serial, cmd: str) -> None:
    ser.write(f"{cmd}\n".encode("utf-8"))
    ser.flush()


def cmd_liste(args: argparse.Namespace) -> None:
    port = trouver_port(args.port)
    with ouvrir_port(port) as ser:
        envoyer_commande(ser, "l")
        for ligne in lire_lignes(ser, 1.0):
            print(ligne)


def cmd_enregistrer(args: argparse.Namespace) -> None:
    port = trouver_port(args.port)
    print(f"Enregistrement empreinte n{args.id} sur {port}")
    print("Suivez les instructions affichees ci-dessous.\n")

    with ouvrir_port(port) as ser:
        envoyer_commande(ser, f"e{args.id}")

        fin = time.time() + 120
        while time.time() < fin:
            raw = ser.readline()
            if not raw:
                continue
            ligne = raw.decode("utf-8", errors="replace").rstrip()
            if ligne:
                print(ligne)
            if "enregistree avec succes" in ligne or "deja occupe" in ligne:
                break
            if "ne correspondent pas" in ligne or "Echec" in ligne:
                sys.exit(1)


def cmd_supprimer(args: argparse.Namespace) -> None:
    port = trouver_port(args.port)
    with ouvrir_port(port) as ser:
        envoyer_commande(ser, f"d{args.id}")
        for ligne in lire_lignes(ser, 1.0):
            print(ligne)


def cmd_vider(args: argparse.Namespace) -> None:
    port = trouver_port(args.port)
    confirm = input("Vider toute la base d'empreintes ? (o/N) : ").strip().lower()
    if confirm != "o":
        print("Annule.")
        return
    with ouvrir_port(port) as ser:
        envoyer_commande(ser, "c")
        for ligne in lire_lignes(ser, 1.0):
            print(ligne)


def cmd_monitor(args: argparse.Namespace) -> None:
    port = trouver_port(args.port)
    print(f"Moniteur serie sur {port} — Ctrl+C pour quitter\n")
    with ouvrir_port(port) as ser:
        try:
            while True:
                raw = ser.readline()
                if raw:
                    print(raw.decode("utf-8", errors="replace").rstrip())
        except KeyboardInterrupt:
            print("\nMoniteur ferme.")


def cmd_ports(_: argparse.Namespace) -> None:
    ports = list(list_ports.comports())
    if not ports:
        print("Aucun port COM detecte.")
        return
    for port in ports:
        print(f"{port.device}\t{port.description}")


def main() -> None:
    parser = argparse.ArgumentParser(description="CLI serie — Lecteur d'empreintes")
    parser.add_argument("--port", "-p", help="Port COM (ex: COM3). Auto-detecte si omis.")
    sub = parser.add_subparsers(dest="command", required=True)

    p_ports = sub.add_parser("ports", help="Lister les ports COM")
    p_ports.set_defaults(func=cmd_ports)

    p_liste = sub.add_parser("liste", help="Afficher le nombre d'empreintes")
    p_liste.set_defaults(func=cmd_liste)

    p_enroll = sub.add_parser("enregistrer", help="Enregistrer une empreinte")
    p_enroll.add_argument("--id", "-i", type=int, required=True, help="Numero d'empreinte (1-127)")
    p_enroll.set_defaults(func=cmd_enregistrer)

    p_del = sub.add_parser("supprimer", help="Supprimer une empreinte")
    p_del.add_argument("--id", "-i", type=int, required=True)
    p_del.set_defaults(func=cmd_supprimer)

    p_clear = sub.add_parser("vider", help="Vider toute la base")
    p_clear.set_defaults(func=cmd_vider)

    p_mon = sub.add_parser("monitor", help="Moniteur serie interactif")
    p_mon.set_defaults(func=cmd_monitor)

    args = parser.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()

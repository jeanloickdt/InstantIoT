/*
 * Les destinations — ce qu'un croquis obtient sans le demander.
 *
 * Une destination n'a pas de comportement : c'est un hôte, un port, un
 * jeton. Ce qui peut être faux, ce sont donc les **défauts** — et un défaut
 * faux ne se voit pas à la relecture, il se voit quand la carte n'arrive
 * pas à joindre le serveur et que personne ne sait pourquoi.
 *
 * Le cas qui a motivé ce fichier : `Cloud(T).plaintext()`. Le port du
 * portier TLS ne veut plus rien dire une fois qu'on ne chiffre plus, mais
 * un port que le croquis a nommé lui-même, si.
 */

#include <stdio.h>
#include <string.h>
#include <type_traits>

#include "../../src/Destinations.hpp"

using namespace iiot;

static int failures = 0;
static int checks   = 0;

static void ok(bool cond, const char* what) {
    checks++;
    if (!cond) { failures++; printf("  ✗ %s\n", what); }
}
static void section(const char* name) { printf("── %s\n", name); }

int main() {
    const char* T = "JETON";

    section("Cloud : ce qu'on obtient sans rien demander");
    {
        SecureDestination d = Cloud(T);
        ok(strcmp(d.host, "instantiot.cloud") == 0, "l'hote du cloud");
        ok(d.port == 9443, "le portier TLS");
        ok(d.token == T, "le jeton");
        ok(d.checksIdentity, "l'identite du serveur est verifiee");
        ok(d.caPem == nullptr, "contre les racines embarquees");
        ok(d.heartbeatMs == 5000, "et la carte signale sa presence");
    }

    section("Le clair : un choix explicite, jamais un defaut");
    {
        // Le type change, et c'est le sujet : sans TLS dans le type, aucune
        // pile TLS n'est embarquee. Une carte qui n'en a pas peut compiler.
        auto clair = Cloud(T).plaintext();
        ok((std::is_same<decltype(clair), PlainDestination>::value),
           "plaintext() rend une destination d'un AUTRE type");
        ok(clair.port == 9001, "et le port suit : le portier en clair, pas celui du TLS");
        ok(strcmp(clair.host, "instantiot.cloud") == 0, "l'hote ne bouge pas");
        ok(clair.token == T, "le jeton non plus — il passera lisible, et c'est le prix");
    }
    {
        // Un port nomme par le croquis sait quelque chose que le defaut
        // ignore. Le remplacer serait le contredire.
        auto clair = Cloud(T).at("preprod.exemple", 8443).plaintext();
        ok(clair.port == 8443, "un port nomme survit au passage en clair");
        ok(strcmp(clair.host, "preprod.exemple") == 0, "l'hote nomme aussi");
    }

    section("Les deux facons de sortir de la verification");
    {
        SecureDestination d = Cloud(T).withCertificate("-----BEGIN CERTIFICATE-----");
        ok(d.caPem != nullptr, "une racine a soi est fournie");
        ok(d.checksIdentity, "…et l'identite reste verifiee — contre elle");
    }
    {
        SecureDestination d = Cloud(T).withoutCertCheck();
        ok(!d.checksIdentity, "la verification tombe");
        ok((std::is_same<decltype(d), SecureDestination>::value),
           "mais le trajet reste chiffre : le type ne change pas");
    }

    section("MyServer : chez soi, en clair");
    {
        PlainDestination d = MyServer("192.168.1.42", T);
        ok(strcmp(d.host, "192.168.1.42") == 0, "l'hote donne");
        ok(d.port == 9001, "le port par defaut d'un serveur InstantIoT");

        PlainDestination p = MyServer("192.168.1.42", 9002, T);
        ok(p.port == 9002, "…ou celui qu'on nomme");
    }
    {
        // Un auto-hebergeur qui met du TLS devant garde son port : c'est
        // lui qui a choisi ou ecoute son serveur.
        auto sur = MyServer("maison.exemple", 9002, T).secure();
        ok((std::is_same<decltype(sur), SecureDestination>::value), "secure() chiffre");
        ok(sur.port == 9002, "et garde le port nomme");
        ok(sur.plaintext().port == 9002, "l'aller-retour ne perd pas le port");
    }

    section("Le battement");
    {
        ok(Cloud(T).heartbeatEvery(0).heartbeatMs == 0, "0 le coupe");
        ok(MyServer("h", T).heartbeatEvery(20000).heartbeatMs == 20000, "et il se regle");
    }

    printf(failures ? "\n%d ECHEC(S) sur %d\n" : "\ntout passe (%d/%d)\n",
           failures ? failures : checks, checks);
    return failures ? 1 : 0;
}

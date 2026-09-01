/*
 * Destinations — what a sketch gets without asking.
 *
 * A destination has no behaviour: it is a host, a port, a token. So what
 * can be wrong are the **defaults** — and a wrong default is invisible on
 * reading; it shows up when the board cannot reach the server and nobody
 * knows why.
 *
 * The case that motivated this file: `Cloud(T).plaintext()`. The TLS
 * gateway's port means nothing once we stop encrypting, but a port the
 * sketch named itself does.
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
    const char* T = "TOKEN";

    section("Cloud: what you get without asking for anything");
    {
        SecureDestination d = Cloud(T);
        ok(strcmp(d.host, "instantiot.cloud") == 0, "the cloud host");
        ok(d.port == 9443, "the TLS gateway");
        ok(d.token == T, "the token");
        ok(d.checksIdentity, "the server identity is verified");
        ok(d.caPem == nullptr, "against the embedded roots");
        ok(d.heartbeatMs == 5000, "and the board announces its presence");
    }

    section("Plaintext: an explicit choice, never a default");
    {
        // The type changes, and that is the point: with no TLS in the
        // type, no TLS stack is embedded. A board that has none can
        // compile.
        auto plain = Cloud(T).plaintext();
        ok((std::is_same<decltype(plain), PlainDestination>::value),
           "plaintext() returns a destination of ANOTHER type");
        ok(plain.port == 9001, "and the port follows: the plain gateway, not the TLS one");
        ok(strcmp(plain.host, "instantiot.cloud") == 0, "the host does not move");
        ok(plain.token == T, "nor the token — it will travel readable, and that is the price");
    }
    {
        // A port named by the sketch knows something the default does
        // not. Replacing it would contradict it.
        auto plain = Cloud(T).at("staging.example", 8443).plaintext();
        ok(plain.port == 8443, "a named port survives the switch to plaintext");
        ok(strcmp(plain.host, "staging.example") == 0, "so does a named host");
    }

    section("The two ways out of verification");
    {
        SecureDestination d = Cloud(T).withCertificate("-----BEGIN CERTIFICATE-----");
        ok(d.caPem != nullptr, "a root of your own is supplied");
        ok(d.checksIdentity, "…and the identity is still verified — against it");
    }
    {
        SecureDestination d = Cloud(T).withoutCertCheck();
        ok(!d.checksIdentity, "verification falls away");
        ok((std::is_same<decltype(d), SecureDestination>::value),
           "but the journey stays encrypted: the type does not change");
    }

    section("MyServer: at home, in plaintext");
    {
        PlainDestination d = MyServer("192.168.1.42", T);
        ok(strcmp(d.host, "192.168.1.42") == 0, "the given host");
        ok(d.port == 9001, "the default port of an InstantIoT server");

        PlainDestination p = MyServer("192.168.1.42", 9002, T);
        ok(p.port == 9002, "…or the one you name");
    }
    {
        // A self-hoster putting TLS in front keeps their port: they chose
        // where their server listens.
        auto secure = MyServer("home.example", 9002, T).secure();
        ok((std::is_same<decltype(secure), SecureDestination>::value), "secure() encrypts");
        ok(secure.port == 9002, "and keeps the named port");
        ok(secure.plaintext().port == 9002, "the round trip does not lose the port");
    }

    section("The heartbeat");
    {
        ok(Cloud(T).heartbeatEvery(0).heartbeatMs == 0, "0 disables it");
        ok(MyServer("h", T).heartbeatEvery(20000).heartbeatMs == 20000, "and it is settable");
    }

    printf(failures ? "\n%d FAILURE(S) of %d\n" : "\nall pass (%d/%d)\n",
           failures ? failures : checks, checks);
    return failures ? 1 : 0;
}

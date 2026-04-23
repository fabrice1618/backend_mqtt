#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <stdexcept>
#include <mosquitto.h>
#include "client_bdd.hpp"
#include "client_mqtt.hpp"
#include "config.h"

/* ── Globals ────────────────────────────────────────────────────── */

static volatile int running = 1;
static ClientBDD   *g_bdd;

static void on_signal(int sig)
{
    (void)sig;
    running = 0;
}

/* ── Parsing JSON minimaliste ───────────────────────────────────── */

static float parse_float_field(const char *json, const char *key)
{
    char pattern[64];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);

    const char *p = strstr(json, pattern);
    if (!p) return -999.0f;

    p += strlen(pattern);
    while (*p == ' ' || *p == ':' || *p == '\t') p++;

    return (float)atof(p);
}

/* ── Callback message ───────────────────────────────────────────── */

static void on_message(const char *topic, const char *payload)
{
    printf("[MQTT] %s → %s\n", topic, payload);

    char capteur_id[64] = "inconnu";
    const char *slash = strrchr(topic, '/');
    if (slash && *(slash + 1))
        snprintf(capteur_id, sizeof(capteur_id), "%s", slash + 1);

    float temperature = parse_float_field(payload, "temperature");
    float pression    = parse_float_field(payload, "pression");
    float humidite    = parse_float_field(payload, "humidite");

    if (temperature > -900.0f || pression > -900.0f || humidite > -900.0f) {
        if (g_bdd->insert(capteur_id, temperature, pression, humidite) == 0)
            printf("[DB]  Inséré: %s T=%.1f P=%.1f H=%.1f\n",
                   capteur_id, temperature, pression, humidite);
    }
}

/* ── Main ───────────────────────────────────────────────────────── */

int main(void)
{
    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);

    try {
        ClientBDD bdd(DB_HOST, DB_USER, DB_PASS, DB_NAME);
        g_bdd = &bdd;

        ClientMQTT mqtt(MQTT_HOST, MQTT_PORT, MQTT_CLIENT_ID, MQTT_TOPIC, MQTT_KEEPALIVE,
                        on_message, MQTT_USER, MQTT_PASS);

        bdd.open();
        mqtt.open();

        printf("=== Serveur MQTT→MySQL démarré ===\n");
        printf("Broker : %s:%d | Topic : %s\n", MQTT_HOST, MQTT_PORT, MQTT_TOPIC);
        printf("Ctrl+C pour arrêter\n\n");

        while (running) {
            bdd.ensure_connection();
            mqtt.loop(1000);
        }
    } catch (const std::exception &e) {
        fprintf(stderr, "Erreur fatale: %s\n", e.what());
        return 1;
    }

    printf("\nServeur arrêté.\n");
    return 0;
}

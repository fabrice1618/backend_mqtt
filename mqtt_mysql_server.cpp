#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <csignal>
#include <mosquitto.h>
#include <mysql/mysql.h>

/* ── Configuration ──────────────────────────────────────────────── */

#define MQTT_HOST        "localhost"
#define MQTT_PORT        1883
#define MQTT_TOPIC       "capteurs/#"
#define MQTT_CLIENT_ID   "mqtt_mysql_server"
#define MQTT_KEEPALIVE   60

#define DB_HOST          "localhost"
#define DB_USER          "capteurs_user"
#define DB_PASS          "capteurs_pass"
#define DB_NAME          "capteurs_db"

/* ── Globals ────────────────────────────────────────────────────── */

static MYSQL           *db_conn;
static struct mosquitto *mosq;
static volatile int     running = 1;

/* ── Signaux ────────────────────────────────────────────────────── */

static void on_signal(int sig)
{
    (void)sig;
    running = 0;
}

/* ── Base de données ────────────────────────────────────────────── */

static int db_connect(void)
{
    db_conn = mysql_init(nullptr);
    if (!db_conn) {
        fprintf(stderr, "[DB] mysql_init échoué\n");
        return -1;
    }

    if (!mysql_real_connect(db_conn, DB_HOST, DB_USER, DB_PASS, DB_NAME, 0, nullptr, 0)) {
        fprintf(stderr, "[DB] connexion échouée: %s\n", mysql_error(db_conn));
        return -1;
    }

    printf("[DB] Connecté à %s/%s\n", DB_HOST, DB_NAME);
    return 0;
}

static int db_insert(const char *capteur_id,
                     float temperature, float pression, float humidite)
{
    char query[512];
    char esc_id[129];

    /* Échapper l'identifiant capteur */
    mysql_real_escape_string(db_conn, esc_id, capteur_id,
                             (unsigned long)strlen(capteur_id));

    snprintf(query, sizeof(query),
        "INSERT INTO releves (capteur_id, temperature, pression, humidite) "
        "VALUES ('%s', %.2f, %.2f, %.2f)",
        esc_id, temperature, pression, humidite);

    if (mysql_query(db_conn, query)) {
        fprintf(stderr, "[DB] INSERT échoué: %s\n", mysql_error(db_conn));
        return -1;
    }
    return 0;
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

static char *parse_string_field(const char *json, const char *key,
                                char *buf, size_t buflen)
{
    char pattern[64];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);

    const char *p = strstr(json, pattern);
    if (!p) return NULL;

    p += strlen(pattern);
    while (*p == ' ' || *p == ':' || *p == '\t') p++;
    if (*p != '"') return NULL;
    p++;

    size_t i = 0;
    while (*p && *p != '"' && i < buflen - 1)
        buf[i++] = *p++;
    buf[i] = '\0';

    return buf;
}

/* ── Callback MQTT ──────────────────────────────────────────────── */

static void on_message(struct mosquitto *, void *,
                       const struct mosquitto_message *msg)
{

    if (!msg->payload || msg->payloadlen == 0)
        return;

    /* Copie NUL-terminée du payload */
    char payload[1024];
    size_t len = (size_t)msg->payloadlen;
    if (len >= sizeof(payload)) len = sizeof(payload) - 1;
    memcpy(payload, msg->payload, len);
    payload[len] = '\0';

    printf("[MQTT] %s → %s\n", msg->topic, payload);

    /* Extraire le capteur_id du topic (capteurs/<id>) ou du JSON */
    char capteur_id[64] = "inconnu";
    const char *slash = strrchr(msg->topic, '/');
    if (slash && *(slash + 1))
        snprintf(capteur_id, sizeof(capteur_id), "%s", slash + 1);

    /* Parser les champs */
    char json_id[64];
    if (parse_string_field(payload, "capteur_id", json_id, sizeof(json_id)))
        snprintf(capteur_id, sizeof(capteur_id), "%s", json_id);

    float temperature = parse_float_field(payload, "temperature");
    float pression    = parse_float_field(payload, "pression");
    float humidite    = parse_float_field(payload, "humidite");

    /* Insérer en base */
    if (temperature > -900.0f || pression > -900.0f || humidite > -900.0f) {
        if (db_insert(capteur_id, temperature, pression, humidite) == 0)
            printf("[DB]  Inséré: %s T=%.1f P=%.1f H=%.1f\n",
                   capteur_id, temperature, pression, humidite);
    }
}

static void on_connect(struct mosquitto *m, void *, int rc)
{
    if (rc == 0) {
        printf("[MQTT] Connecté au broker %s:%d\n", MQTT_HOST, MQTT_PORT);
        mosquitto_subscribe(m, nullptr, MQTT_TOPIC, 1);
        printf("[MQTT] Abonné au topic: %s\n", MQTT_TOPIC);
    } else {
        fprintf(stderr, "[MQTT] Connexion échouée: %s\n", mosquitto_connack_string(rc));
    }
}

/* ── Reconnexion DB automatique ─────────────────────────────────── */

static int ensure_db_connection(void)
{
    if (db_conn && mysql_ping(db_conn) == 0)
        return 0;

    fprintf(stderr, "[DB] Connexion perdue, reconnexion...\n");
    if (db_conn) mysql_close(db_conn);
    return db_connect();
}

/* ── Main ───────────────────────────────────────────────────────── */

int main(void)
{
    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);

    /* Initialisation Mosquitto */
    mosquitto_lib_init();

    /* Connexion DB */
    if (db_connect() != 0)
        goto cleanup;

    /* Création du client MQTT */
    mosq = mosquitto_new(MQTT_CLIENT_ID, true, nullptr);
    if (!mosq) {
        fprintf(stderr, "[MQTT] Impossible de créer le client\n");
        goto cleanup;
    }

    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_message_callback_set(mosq, on_message);

    /* Connexion MQTT */
    if (mosquitto_connect(mosq, MQTT_HOST, MQTT_PORT, MQTT_KEEPALIVE) !=
        MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "[MQTT] Connexion au broker échouée\n");
        goto cleanup;
    }

    printf("=== Serveur MQTT→MySQL démarré ===\n");
    printf("Broker : %s:%d | Topic : %s\n", MQTT_HOST, MQTT_PORT, MQTT_TOPIC);
    printf("Ctrl+C pour arrêter\n\n");

    /* Boucle principale */
    while (running) {
        ensure_db_connection();
        int rc = mosquitto_loop(mosq, 1000, 1);
        if (rc != MOSQ_ERR_SUCCESS && running) {
            fprintf(stderr, "[MQTT] Erreur: %s — reconnexion dans 5s\n",
                    mosquitto_strerror(rc));
            sleep(5);
            mosquitto_reconnect(mosq);
        }
    }

cleanup:
    printf("\nArrêt en cours...\n");
    if (mosq) {
        mosquitto_disconnect(mosq);
        mosquitto_destroy(mosq);
    }
    if (db_conn) mysql_close(db_conn);
    mosquitto_lib_cleanup();
    printf("Serveur arrêté.\n");
    return 0;
}

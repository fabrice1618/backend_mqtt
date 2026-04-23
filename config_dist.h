#pragma once

/* Copier ce fichier en config.h et adapter les valeurs. */

/* ── Broker MQTT ────────────────────────────────────────────────── */
#define MQTT_HOST       "localhost"
#define MQTT_PORT       1883
#define MQTT_TOPIC      "capteurs/#"
#define MQTT_CLIENT_ID  "mqtt_mysql_server"
#define MQTT_KEEPALIVE  60

/* ── Base de données MySQL ──────────────────────────────────────── */
#define DB_HOST  "localhost"
#define DB_USER  "capteurs_user"
#define DB_PASS  "capteurs_pass"
#define DB_NAME  "capteurs_db"

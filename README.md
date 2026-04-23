# backend_mqtt — Pont MQTT → MySQL

Serveur C++17 qui reçoit les relevés de capteurs via MQTT et les persiste dans une base de données MySQL.

## Architecture

```
Capteur  ──MQTT──►  Mosquitto broker  ──►  mqtt_mysql_server  ──►  MySQL (capteurs_db)
```

Trois composants :

| Fichier | Rôle |
|---|---|
| `mqtt_mysql_server.cpp` | Point d'entrée, parsing JSON, boucle principale |
| `client_mqtt.cpp/.hpp` | Encapsulation libmosquitto (connexion, souscription, reconnexion) |
| `client_bdd.cpp/.hpp` | Encapsulation libmysqlclient (connexion, insertion, reconnexion) |

## Prérequis

- `g++` avec support C++17
- `libmosquitto-dev`
- `libmysqlclient-dev`
- Broker Mosquitto accessible sur `localhost:1883`
- Instance MySQL accessible sur `localhost`

## Installation

```bash
# Dépendances (Debian/Ubuntu)
sudo apt install libmosquitto-dev libmysqlclient-dev mosquitto

# Compilation
make

# Base de données
mysql -u root -p < schema.sql
```

## Configuration

Les paramètres sont définis dans `config.h`, inclus par `mqtt_mysql_server.cpp`.
Ce fichier est personnel et exclu du dépôt (`.gitignore`).

Le dépôt fournit `config_dist.h` comme modèle. Avant la première compilation :

```bash
cp config_dist.h config.h
# puis éditer config.h avec les valeurs locales
```

Paramètres disponibles :

```cpp
/* ── Broker MQTT ── */
#define MQTT_HOST       "localhost"
#define MQTT_PORT       1883
#define MQTT_TOPIC      "capteurs/#"
#define MQTT_CLIENT_ID  "mqtt_mysql_server"
#define MQTT_KEEPALIVE  60

/* ── MySQL ── */
#define DB_HOST  "localhost"
#define DB_USER  "capteurs_user"
#define DB_PASS  "capteurs_pass"
#define DB_NAME  "capteurs_db"
```

## Lancement

```bash
./mqtt_mysql_server
```

Arrêt propre avec `Ctrl+C` (SIGINT) ou `SIGTERM`.

## Format des messages MQTT

### Topic

```
capteurs/<capteur_id>
```

Le segment final du topic est utilisé comme identifiant du capteur (ex. `capteurs/bmp280` → `capteur_id = "bmp280"`).

### Payload JSON

```json
{
  "temperature": 23.5,
  "pression": 1013.2,
  "humidite": 55.0
}
```

| Champ | Type | Description |
|---|---|---|
| `temperature` | float | Température en °C |
| `pression` | float | Pression atmosphérique en hPa |
| `humidite` | float | Humidité relative en % |

**Règles d'insertion :**

- Les champs absents ou invalides reçoivent la valeur sentinelle `-999.0`.
- La ligne est **ignorée** si les trois champs `temperature`, `pression` et `humidite` sont tous absents/invalides.
- Si au moins un champ est présent, la ligne est insérée avec `-999.0` pour les champs manquants.

### Exemples

Message minimal (température seule) :
```
topic  : capteurs/bmp280
payload: {"temperature": 22.1}
```

Message complet avec identifiant explicite :
```
topic  : capteurs/salon
payload: {"capteur_id": "bmp280_salon", "temperature": 21.3, "pression": 1015.0, "humidite": 48.7}
```

## Schéma de la base de données

Table `releves` dans la base `capteurs_db` :

| Colonne | Type | Description |
|---|---|---|
| `id` | INT AUTO_INCREMENT | Clé primaire |
| `capteur_id` | VARCHAR(64) | Identifiant du capteur |
| `temperature` | FLOAT | Température (°C), `-999.0` si absente |
| `pression` | FLOAT | Pression (hPa), `-999.0` si absente |
| `humidite` | FLOAT | Humidité (%), `-999.0` si absente |
| `horodatage` | TIMESTAMP | Date/heure d'insertion (automatique) |

## Résilience

- **MQTT** : en cas d'erreur de boucle, attente 5 secondes puis `mosquitto_reconnect`.
- **MySQL** : `ensure_connection()` appelé à chaque itération via `mysql_ping` ; reconnexion transparente si la connexion est perdue.

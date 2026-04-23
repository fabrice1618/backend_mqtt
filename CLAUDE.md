# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

```bash
make          # compile mqtt_mysql_server
make clean    # remove the binary
```

Compiler: `g++ -Wall -Wextra -std=c++17 -O2`  
Link dependencies: `libmosquitto`, `libmysqlclient`

## Database setup

```bash
mysql -u root -p < schema.sql
```

Creates database `capteurs_db` with table `releves` (capteur_id, temperature, pression, humidite, horodatage).

Default DB credentials are hardcoded in `mqtt_mysql_server.cpp`: user `capteurs_user`, password `capteurs_pass`, host `localhost`.

## Running

```bash
./mqtt_mysql_server
```

Requires a running Mosquitto broker on `localhost:1883` and a reachable MySQL instance. Stop with Ctrl+C (SIGINT) or SIGTERM.

## Architecture

Single-file C++17 server (`mqtt_mysql_server.cpp`) that bridges MQTT sensor messages to a MySQL database.

**Data flow:** MQTT broker → `on_message` callback → minimal JSON parser → `db_insert` → MySQL `releves` table.

- Subscribes to wildcard topic `capteurs/#`; the sensor ID is extracted from the last path segment (e.g. `capteurs/bmp280` → `capteur_id = "bmp280"`) and can be overridden by a `capteur_id` field in the JSON payload.
- Expected JSON payload fields: `temperature`, `pression`, `humidite` (all floats). Missing fields default to `-999.0` sentinel and the row is skipped if all three are absent/invalid.
- `ensure_db_connection()` is called each loop iteration to transparently reconnect on MySQL drop.
- MQTT loop errors trigger a 5-second backoff then `mosquitto_reconnect`.
- JSON parsing is hand-rolled (`parse_float_field`) — no external JSON library.

#!/bin/bash

BROKER="localhost"
PORT=1883
USER="mqtt"
PASS="mqtt_pwd"
TOPIC="capteurs/bmp280"
PAYLOAD='{"temperature": 22.5, "pression": 1013.25, "humidite": 58.3}'

mosquitto_pub -h "$BROKER" -p "$PORT" -u "$USER" -P "$PASS" -t "$TOPIC" -m "$PAYLOAD"

echo "Message envoyé sur $TOPIC : $PAYLOAD"

-- Création de la base de données pour les relevés capteurs
CREATE DATABASE IF NOT EXISTS capteurs_db;
USE capteurs_db;

-- Table des relevés : température, pression, humidité
CREATE TABLE IF NOT EXISTS releves (
    id          INT AUTO_INCREMENT PRIMARY KEY,
    capteur_id  VARCHAR(64)   NOT NULL,
    temperature FLOAT,
    pression    FLOAT,
    humidite    FLOAT,
    horodatage  TIMESTAMP     NOT NULL DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_capteur (capteur_id),
    INDEX idx_horodatage (horodatage)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

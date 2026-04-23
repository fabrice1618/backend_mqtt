#include "client_mqtt.hpp"
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <unistd.h>

ClientMQTT::ClientMQTT(const char *host, int port, const char *client_id,
                       const char *topic, int keepalive, MessageCallback cb)
    : mosq_(nullptr), host_(host), port_(port), client_id_(client_id),
      topic_(topic), keepalive_(keepalive), cb_(cb)
{
    mosquitto_lib_init();
}

ClientMQTT::~ClientMQTT()
{
    close();
    mosquitto_lib_cleanup();
}

void ClientMQTT::open()
{
    mosq_ = mosquitto_new(client_id_, true, this);
    if (!mosq_)
        throw std::runtime_error("[MQTT] Impossible de créer le client");

    mosquitto_connect_callback_set(mosq_, on_connect);
    mosquitto_message_callback_set(mosq_, on_message);

    if (mosquitto_connect(mosq_, host_, port_, keepalive_) != MOSQ_ERR_SUCCESS)
        throw std::runtime_error("[MQTT] Connexion au broker échouée");
}

int ClientMQTT::loop(int timeout_ms)
{
    int rc = mosquitto_loop(mosq_, timeout_ms, 1);
    if (rc != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "[MQTT] Erreur: %s — reconnexion dans 5s\n",
                mosquitto_strerror(rc));
        sleep(5);
        mosquitto_reconnect(mosq_);
    }
    return rc;
}

void ClientMQTT::close()
{
    if (mosq_) {
        mosquitto_disconnect(mosq_);
        mosquitto_destroy(mosq_);
        mosq_ = nullptr;
    }
}

void ClientMQTT::on_connect(struct mosquitto *m, void *userdata, int rc)
{
    ClientMQTT *self = static_cast<ClientMQTT *>(userdata);
    if (rc == 0) {
        printf("[MQTT] Connecté au broker %s:%d\n", self->host_, self->port_);
        mosquitto_subscribe(m, nullptr, self->topic_, 1);
        printf("[MQTT] Abonné au topic: %s\n", self->topic_);
    } else {
        fprintf(stderr, "[MQTT] Connexion échouée: %s\n", mosquitto_connack_string(rc));
    }
}

void ClientMQTT::on_message(struct mosquitto *, void *userdata,
                            const struct mosquitto_message *msg)
{
    ClientMQTT *self = static_cast<ClientMQTT *>(userdata);

    if (!msg->payload || msg->payloadlen == 0)
        return;

    char payload[1024];
    size_t len = (size_t)msg->payloadlen;
    if (len >= sizeof(payload)) len = sizeof(payload) - 1;
    memcpy(payload, msg->payload, len);
    payload[len] = '\0';

    self->cb_(msg->topic, payload);
}

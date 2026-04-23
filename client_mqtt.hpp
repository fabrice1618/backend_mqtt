#pragma once
#include <mosquitto.h>
#include <functional>

class ClientMQTT {
public:
    using MessageCallback = std::function<void(const char *topic, const char *payload)>;

private:
    struct mosquitto *mosq_;
    const char       *host_;
    int               port_;
    const char       *client_id_;
    const char       *topic_;
    int               keepalive_;
    MessageCallback   cb_;
    const char       *user_;
    const char       *pass_;

public:
    ClientMQTT(const char *host, int port, const char *client_id,
               const char *topic, int keepalive, MessageCallback cb,
               const char *user = nullptr, const char *pass = nullptr);
    ~ClientMQTT();
    void open();
    int  loop(int timeout_ms);
    void close();

private:
    static void on_connect(struct mosquitto *m, void *userdata, int rc);
    static void on_message(struct mosquitto *, void *userdata,
                           const struct mosquitto_message *msg);
};

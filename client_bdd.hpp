#pragma once
#include <mysql/mysql.h>

class ClientBDD {
private:
    MYSQL      *conn_;
    const char *host_;
    const char *user_;
    const char *pass_;
    const char *name_;

public:
    ClientBDD(const char *host, const char *user,
              const char *pass, const char *name);
    ~ClientBDD();
    int open();
    int ensure_connection();
    int insert(const char *capteur_id,
               float temperature, float pression, float humidite);
private:
    int connect();
};

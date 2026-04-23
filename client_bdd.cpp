#include "client_bdd.hpp"
#include <cstdio>
#include <cstring>

ClientBDD::ClientBDD(const char *host, const char *user,
                     const char *pass, const char *name)
    : conn_(nullptr), host_(host), user_(user), pass_(pass), name_(name)
{}

ClientBDD::~ClientBDD()
{
    if (conn_) mysql_close(conn_);
}

int ClientBDD::connect()
{
    conn_ = mysql_init(nullptr);
    if (!conn_) {
        fprintf(stderr, "[DB] mysql_init échoué\n");
        return -1;
    }

    if (!mysql_real_connect(conn_, host_, user_, pass_, name_, 0, nullptr, 0)) {
        fprintf(stderr, "[DB] connexion échouée: %s\n", mysql_error(conn_));
        return -1;
    }

    printf("[DB] Connecté à %s/%s\n", host_, name_);
    return 0;
}

int ClientBDD::open()
{
    return connect();
}

int ClientBDD::ensure_connection()
{
    if (conn_ && mysql_ping(conn_) == 0)
        return 0;

    fprintf(stderr, "[DB] Connexion perdue, reconnexion...\n");
    if (conn_) mysql_close(conn_);
    return connect();
}

int ClientBDD::insert(const char *capteur_id,
                      float temperature, float pression, float humidite)
{
    char query[512];
    char esc_id[129];

    /* Échapper l'identifiant capteur */
    mysql_real_escape_string(conn_, esc_id, capteur_id,
                             (unsigned long)strlen(capteur_id));

    snprintf(query, sizeof(query),
        "INSERT INTO releves (capteur_id, temperature, pression, humidite) "
        "VALUES ('%s', %.2f, %.2f, %.2f)",
        esc_id, temperature, pression, humidite);

    if (mysql_query(conn_, query)) {
        fprintf(stderr, "[DB] INSERT échoué: %s\n", mysql_error(conn_));
        return -1;
    }
    return 0;
}

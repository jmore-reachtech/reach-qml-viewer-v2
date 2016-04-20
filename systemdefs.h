#ifndef SYSTEMDEFS_H
#define SYSTEMDEFS_H

#define SYSTEM_SETTINGS_FILE "/application/src/settings.json"
#define HEARTBEAT_TEXT "ping"
#define HEARTBEAT_RESPONSE_TEXT "pong"
#define APPLICATION_SETTINGS_FILE "/application/src/application.conf"
#define APPLICATION_SETTINGS_SECTION "Application"
#define TRANSLATION_FILE_PATH "/application/src/translate.txt"

struct tcp_ports{
    int port = -1;
    bool parse_json = false;
};

#endif // SYSTEMDEFS_H

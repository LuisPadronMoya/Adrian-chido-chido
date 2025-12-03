#pragma once

#include <Firebase_ESP_Client.h>

class FirebaseCom {
public:
    FirebaseData fbdo;
    FirebaseAuth auth;
    FirebaseConfig config;

    FirebaseCom(String& _databaseURL,
                String& _apiKey);
    ~FirebaseCom();

    void pushData(FirebaseJson json, String path);

private:
    String databaseURL;
    String apiKey;
};

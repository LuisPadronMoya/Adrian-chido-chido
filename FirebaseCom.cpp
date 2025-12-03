#include "FirebaseCom.hpp"

FirebaseCom::FirebaseCom(String& _databaseURL,
                         String& _apiKey) : 
                         databaseURL(_databaseURL), 
                         apiKey(_apiKey) {

    config.database_url = databaseURL;
    config.api_key = apiKey;
    config.signer.test_mode = true;
    Firebase.begin(&config, &auth);
}

FirebaseCom::~FirebaseCom() {
    // Destructor vacío, pero puede usarse para limpieza si es necesario
}

void FirebaseCom::pushData(FirebaseJson json, String path){
    if (Firebase.RTDB.pushJSON(&fbdo, path, &json)) {
        Serial.printf("Subida exitosa a Firebase. Path: %s\n", path.c_str());
    } else {
        Serial.printf("Fallo de subida a Firebase: %s\n", fbdo.errorReason().c_str());
    }
}

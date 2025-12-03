#include "Protocol.hpp"

namespace Protocol {

    const char* const TYPE_NTP_REQ      = "NTP_REQ";
    const char* const TYPE_NTP_RES      = "NTP_RES";
    const char* const TYPE_ACK          = "ACK";
    const char* const TYPE_DATA_PREFIX  = "DATA";

    String getType(const JsonDocument& doc) {
        const char* t = doc["type"] | "";
        return String(t);
    }

    bool isNtpReq(const JsonDocument& doc) {
        return getType(doc).equalsIgnoreCase(TYPE_NTP_REQ);
    }

    bool isData(const JsonDocument& doc) {
        String t = getType(doc);
        // DATA, DATA_HIST, etc. (cualquier cosa que empiece con "DATA")
        if (t.length() < 4) return false;
        return t.substring(0, 4).equalsIgnoreCase(TYPE_DATA_PREFIX);
    }

    void makeNtpRes(DynamicJsonDocument& doc,
                    unsigned long long t1,
                    unsigned long long t2,
                    unsigned long long t3) {
        doc.clear();
        doc["type"] = TYPE_NTP_RES;
        doc["t1"]   = t1;
        doc["t2"]   = t2;
        doc["t3"]   = t3;
    }

    void makeAck(DynamicJsonDocument& doc,
                 unsigned long long serverTs,
                 const char* status) {
        doc.clear();
        doc["type"]      = TYPE_ACK;
        doc["status"]    = status;
        doc["server_ts"] = serverTs;
    }
}

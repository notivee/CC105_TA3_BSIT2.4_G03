#ifndef MODELS_H
#define MODELS_H

#include <string>

struct Asset {
    int id;
    std::string name;
    std::string location; // Location code A..G
    std::string status;   // "Operational" or "Dispatched (Req ...)"
};

struct ServiceRequest {
    std::string reqId;
    int assetId;
    int urgency;      // 1 (lowest) to 5 (highest)
    int arrivalOrder; // Monotonically increasing arrival sequence
};

#endif

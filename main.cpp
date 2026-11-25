#include "Models.h"
#include "AssetIndexes.h"
#include "DispatchEngine.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <limits>
#include <iomanip>
#include <cctype>

class SmartCitySystem {
private:
    std::vector<Asset> assets; // Authoritative asset record collection
    AssetBST bst;
    AssetHashTable hashTable;
    RequestMaxHeap requestHeap;
    CityRoadGraph graph;
    int nextArrivalCounter = 1;
    const std::string DEPOT = "A";

    static std::string trim(const std::string& s) {
        size_t first = s.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = s.find_last_not_of(" \t\r\n");
        return s.substr(first, last - first + 1);
    }

    static std::string toUpperStr(std::string s) {
        for (char& c : s) {
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
        return s;
    }

public:
    bool addAssetRecord(int id, const std::string& name, const std::string& rawLoc, const std::string& status, bool verbose = true) {
        std::string loc = toUpperStr(trim(rawLoc));
        std::string cleanName = trim(name);

        if (id <= 0) {
            if (verbose) std::cout << "  [Rejected] Asset ID must be a positive integer.\n";
            return false;
        }
        if (cleanName.empty()) {
            if (verbose) std::cout << "  [Rejected] Asset name cannot be empty.\n";
            return false;
        }
        if (!graph.hasLocation(loc)) {
            if (verbose) std::cout << "  [Rejected] Invalid location code '" << loc << "' (valid: A-G).\n";
            return false;
        }
        if (bst.search(id) != -1) {
            if (verbose) std::cout << "  [Rejected] Duplicate Asset ID " << id << " already exists. Indexes unchanged.\n";
            return false;
        }

        size_t idx = assets.size();
        assets.push_back({id, cleanName, loc, status});
        bst.insert(id, idx);
        hashTable.insert(id, idx);

        if (verbose) {
            std::cout << "  [Added] Asset " << id << " (" << cleanName << ") at " << loc
                      << " (" << graph.getLocationName(loc) << ") -> Hash Bucket ["
                      << hashTable.hashFn(id) << "].\n";
        }
        return true;
    }

    bool addRequestRecord(const std::string& rawReqId, int assetId, int urgency, int arrival, bool verbose = true) {
        std::string reqId = trim(rawReqId);
        if (reqId.empty()) {
            if (verbose) std::cout << "  [Rejected] Request ID cannot be empty.\n";
            return false;
        }
        if (bst.search(assetId) == -1) {
            if (verbose) std::cout << "  [Rejected] Target Asset ID " << assetId << " does not exist.\n";
            return false;
        }
        if (urgency < 1 || urgency > 5) {
            if (verbose) std::cout << "  [Rejected] Urgency must be between 1 and 5.\n";
            return false;
        }
        requestHeap.push({reqId, assetId, urgency, arrival});
        if (arrival >= nextArrivalCounter) nextArrivalCounter = arrival + 1;
        if (verbose) {
            std::cout << "  [Queued] Request " << reqId << " for Asset " << assetId
                      << " (Urgency: " << urgency << ", Arrival: " << arrival << ").\n";
        }
        return true;
    }

    void loadInitialDataset(const std::string& dir) {
        std::ifstream locFile(dir + "/locations.csv");
        std::ifstream linkFile(dir + "/links.csv");
        std::ifstream assetFile(dir + "/assets.csv");
        std::ifstream reqFile(dir + "/requests.csv");

        if (!locFile || !linkFile || !assetFile || !reqFile) {
            std::cout << "[Warning] Could not open one or more CSV files in '" << dir << "/'. Starting empty.\n";
            return;
        }

        std::string line;
        std::getline(locFile, line);
        while (std::getline(locFile, line)) {
            if (trim(line).empty()) continue;
            std::stringstream ss(line);
            std::string code, name;
            std::getline(ss, code, ',');
            std::getline(ss, name, ',');
            graph.addLocation(toUpperStr(trim(code)), trim(name));
        }

        std::getline(linkFile, line);
        while (std::getline(linkFile, line)) {
            if (trim(line).empty()) continue;
            std::stringstream ss(line);
            std::string u, v;
            std::getline(ss, u, ',');
            std::getline(ss, v, ',');
            graph.addTwoWayLink(toUpperStr(trim(u)), toUpperStr(trim(v)));
        }

        std::getline(assetFile, line);
        while (std::getline(assetFile, line)) {
            if (trim(line).empty()) continue;
            std::stringstream ss(line);
            std::string idStr, name, loc, status;
            std::getline(ss, idStr, ',');
            std::getline(ss, name, ',');
            std::getline(ss, loc, ',');
            std::getline(ss, status, ',');
            addAssetRecord(std::stoi(trim(idStr)), trim(name), trim(loc), trim(status), false);
        }

        std::getline(reqFile, line);
        while (std::getline(reqFile, line)) {
            if (trim(line).empty()) continue;
            std::stringstream ss(line);
            std::string reqId, assetStr, urgStr, arrStr;
            std::getline(ss, reqId, ',');
            std::getline(ss, assetStr, ',');
            std::getline(ss, urgStr, ',');
            std::getline(ss, arrStr, ',');
            addRequestRecord(trim(reqId), std::stoi(trim(assetStr)), std::stoi(trim(urgStr)), std::stoi(trim(arrStr)), false);
        }

        std::cout << "[System Ready] Loaded " << assets.size() << " assets and "
                  << requestHeap.size() << " service requests (Group 03: Public Facility Maintenance).\n";
    }

    // 1. Asset entry
    void menuAssetEntry() {
        int id;
        std::string name, loc;
        std::cout << "  Enter new Asset ID (positive integer): ";
        if (!(std::cin >> id)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "  [Error] Invalid numeric Asset ID.\n";
            return;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "  Enter Asset Name: ";
        std::getline(std::cin, name);
        std::cout << "  Enter Location Code (A-G): ";
        std::getline(std::cin, loc);

        addAssetRecord(id, name, loc, "Operational", true);
    }

    // 2. Lookup (BST + Hash Table comparison)
    void menuLookup() {
        int id;
        std::cout << "  Enter Asset ID to look up: ";
        if (!(std::cin >> id)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "  [Error] Invalid numeric Asset ID.\n";
            return;
        }

        int bstIdx = bst.search(id);
        size_t bucket = 0, depth = 0;
        int hashIdx = hashTable.search(id, bucket, depth);

        if (bstIdx == -1 || hashIdx == -1) {
            std::cout << "  [Not Found] Asset ID " << id << " does not exist in BST or Hash Table.\n";
            return;
        }

        const Asset& a = assets[static_cast<size_t>(bstIdx)];
        const Asset& b = assets[static_cast<size_t>(hashIdx)];
        std::cout << "  [BST Match]   Record #" << bstIdx << " -> ID " << a.id
                  << " | " << a.name << " | Loc: " << a.location
                  << " (" << graph.getLocationName(a.location) << ") | Status: " << a.status << "\n";
        std::cout << "  [Hash Match]  Bucket [" << bucket << "], Chain Pos " << depth
                  << " -> ID " << b.id << " | " << b.name << " | Status: " << b.status << "\n";
        hashTable.printBuckets();
    }

    // 3. Ordered listing (BST In-Order)
    void menuOrderedListing() {
        std::cout << "\n--- ASSET LISTING (BST IN-ORDER ASCENDING BY ID) ---\n";
        bst.printInOrder(assets);
    }

    // 4. Request entry
    void menuRequestEntry() {
        std::string reqId;
        int assetId, urgency;
        std::cout << "  Enter Request ID (e.g., R7): ";
        std::cin >> reqId;
        std::cout << "  Enter Target Asset ID: ";
        if (!(std::cin >> assetId)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "  [Error] Invalid numeric Asset ID.\n";
            return;
        }
        std::cout << "  Enter Urgency (1-5, 5 = highest): ";
        if (!(std::cin >> urgency)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "  [Error] Invalid numeric Urgency.\n";
            return;
        }
        addRequestRecord(reqId, assetId, urgency, nextArrivalCounter, true);
    }

    // 5. Queue preview
    void menuQueuePreview() {
        if (requestHeap.empty()) {
            std::cout << "  [Empty Queue] No pending service requests.\n";
            return;
        }
        auto ordered = requestHeap.getOrderedPreview();
        std::cout << "\n--- PENDING SERVICE REQUESTS (MAX-HEAP PRIORITY ORDER) ---\n";
        for (size_t i = 0; i < ordered.size(); ++i) {
            size_t idx = static_cast<size_t>(bst.search(ordered[i].assetId));
            std::cout << "  " << (i + 1) << ". Req " << ordered[i].reqId
                      << " | Asset " << ordered[i].assetId << " (" << assets[idx].name
                      << " @ " << assets[idx].location << ")"
                      << " | Urgency: " << ordered[i].urgency
                      << " | Arrival: " << ordered[i].arrivalOrder << "\n";
        }
    }

    // 6. Route preview for top pending request
    void menuRoutePreview() {
        if (requestHeap.empty()) {
            std::cout << "  [Empty Queue] No pending request available for route preview.\n";
            return;
        }
        const ServiceRequest& nextReq = requestHeap.top();
        size_t idx = static_cast<size_t>(bst.search(nextReq.assetId));
        const Asset& targetAsset = assets[idx];

        std::cout << "\n--- ROUTE PREVIEW FOR TOP PENDING REQUEST (" << nextReq.reqId << ") ---\n";
        std::cout << "  Target Asset : " << targetAsset.id << " - " << targetAsset.name << "\n";
        std::cout << "  Origin Depot : " << DEPOT << " (" << graph.getLocationName(DEPOT) << ")\n";
        std::cout << "  Destination  : " << targetAsset.location << " (" << graph.getLocationName(targetAsset.location) << ")\n";

        std::vector<std::string> path = graph.findShortestRoute(DEPOT, targetAsset.location);
        if (path.empty()) {
            std::cout << "  [Unreachable] No road link path exists from " << DEPOT << " to "
                      << targetAsset.location << ". Request " << nextReq.reqId << " remains PENDING.\n";
        } else {
            std::cout << "  Shortest Path (" << (path.size() - 1) << " links): ";
            for (size_t i = 0; i < path.size(); ++i) {
                if (i > 0) std::cout << " -> ";
                std::cout << path[i];
            }
            std::cout << "\n";
        }
    }

    // 7. Dispatch top request if reachable
    void menuDispatch() {
        if (requestHeap.empty()) {
            std::cout << "  [Empty Queue] No pending requests to dispatch.\n";
            return;
        }
        const ServiceRequest& nextReq = requestHeap.top();
        size_t idx = static_cast<size_t>(bst.search(nextReq.assetId));
        Asset& targetAsset = assets[idx];

        std::vector<std::string> path = graph.findShortestRoute(DEPOT, targetAsset.location);
        if (path.empty()) {
            std::cout << "  [Dispatch Blocked] Destination " << targetAsset.location << " ("
                      << graph.getLocationName(targetAsset.location) << ") is UNREACHABLE from Depot "
                      << DEPOT << ". Request " << nextReq.reqId << " stays PENDING.\n";
            return;
        }

        ServiceRequest served = requestHeap.pop();
        targetAsset.status = "Dispatched (Req " + served.reqId + ")";

        std::cout << "\n--- DISPATCH CONFIRMED ---\n";
        std::cout << "  Served Request : " << served.reqId << " (Urgency " << served.urgency
                  << ", Arrival " << served.arrivalOrder << ")\n";
        std::cout << "  Asset Updated  : " << targetAsset.id << " (" << targetAsset.name
                  << ") -> Status: " << targetAsset.status << "\n";
        std::cout << "  Route (" << (path.size() - 1) << " links): ";
        for (size_t i = 0; i < path.size(); ++i) {
            if (i > 0) std::cout << " -> ";
            std::cout << path[i];
        }
        std::cout << "\n";
    }

    // 8. Sorted report (Merge sort by name, tie-break by ID)
    void menuSortedReport() {
        auto sorted = AssetReportSorter::sortByNameThenId(assets);
        std::cout << "\n--- SORTED ASSET REPORT (BY NAME, TIE-BREAK ID) ---\n";
        for (const auto& a : sorted) {
            std::cout << "  " << std::left << std::setw(26) << a.name
                      << " | ID: " << std::right << std::setw(4) << a.id
                      << " | Loc: " << a.location
                      << " | Status: " << a.status << "\n";
        }
    }

    void run() {
        loadInitialDataset("data");
        int choice = 0;
        while (true) {
            std::cout << "\n=== SMART CITY MENU (GROUP 03: PUBLIC FACILITY MAINTENANCE) ===\n"
                      << "1. Asset entry\n"
                      << "2. Lookup\n"
                      << "3. Ordered listing\n"
                      << "4. Request entry\n"
                      << "5. Queue preview\n"
                      << "6. Route preview\n"
                      << "7. Dispatch\n"
                      << "8. Sorted report\n"
                      << "9. Exit\n"
                      << "Select option (1-9): ";

            if (!(std::cin >> choice)) {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cout << "  [Invalid Input] Please enter a number from 1 to 9.\n";
                continue;
            }

            switch (choice) {
                case 1: menuAssetEntry(); break;
                case 2: menuLookup(); break;
                case 3: menuOrderedListing(); break;
                case 4: menuRequestEntry(); break;
                case 5: menuQueuePreview(); break;
                case 6: menuRoutePreview(); break;
                case 7: menuDispatch(); break;
                case 8: menuSortedReport(); break;
                case 9:
                    std::cout << "Exiting Smart City System. Clean memory release complete.\n";
                    return;
                default:
                    std::cout << "  [Invalid Option] Choose 1 through 9.\n";
            }
        }
    }
};

int main() {
    SmartCitySystem app;
    app.run();
    return 0;
}

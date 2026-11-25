
#ifndef DISPATCH_ENGINE_H
#define DISPATCH_ENGINE_H

#include "Models.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <queue>
#include <algorithm>
#include <iostream>

// --- Binary Max-Heap (Highest urgency first; earlier arrivalOrder breaks ties) ---
class RequestMaxHeap {
private:
    std::vector<ServiceRequest> heap;

    bool higherPriority(const ServiceRequest& a, const ServiceRequest& b) const {
        if (a.urgency != b.urgency) return a.urgency > b.urgency;
        return a.arrivalOrder < b.arrivalOrder;
    }

    void siftUp(size_t idx) {
        while (idx > 0) {
            size_t parent = (idx - 1) / 2;
            if (higherPriority(heap[idx], heap[parent])) {
                std::swap(heap[idx], heap[parent]);
                idx = parent;
            } else {
                break;
            }
        }
    }

    void siftDown(size_t idx) {
        size_t n = heap.size();
        while (true) {
            size_t best = idx;
            size_t left = 2 * idx + 1;
            size_t right = 2 * idx + 2;
            if (left < n && higherPriority(heap[left], heap[best])) best = left;
            if (right < n && higherPriority(heap[right], heap[best])) best = right;
            if (best != idx) {
                std::swap(heap[idx], heap[best]);
                idx = best;
            } else {
                break;
            }
        }
    }

public:
    bool empty() const { return heap.empty(); }
    size_t size() const { return heap.size(); }

    void push(const ServiceRequest& req) {
        heap.push_back(req);
        siftUp(heap.size() - 1);
    }

    const ServiceRequest& top() const {
        return heap.front();
    }

    ServiceRequest pop() {
        ServiceRequest topItem = heap.front();
        heap.front() = heap.back();
        heap.pop_back();
        if (!heap.empty()) siftDown(0);
        return topItem;
    }

    std::vector<ServiceRequest> getOrderedPreview() const {
        RequestMaxHeap copy = *this;
        std::vector<ServiceRequest> ordered;
        while (!copy.empty()) {
            ordered.push_back(copy.pop());
        }
        return ordered;
    }
};

// --- Adjacency List Graph + BFS with FIFO Queue ---
class CityRoadGraph {
private:
    std::unordered_map<std::string, std::string> locationNames;
    std::unordered_map<std::string, std::vector<std::string>> adj;

public:
    void addLocation(const std::string& code, const std::string& name) {
        locationNames[code] = name;
        adj[code]; // Ensure isolated vertices (such as G) exist in adjacency list
    }

    bool hasLocation(const std::string& code) const {
        return locationNames.find(code) != locationNames.end();
    }

    std::string getLocationName(const std::string& code) const {
        auto it = locationNames.find(code);
        return (it != locationNames.end()) ? it->second : "Unknown";
    }

    bool addTwoWayLink(const std::string& u, const std::string& v) {
        if (!hasLocation(u) || !hasLocation(v)) return false;
        adj[u].push_back(v);
        adj[v].push_back(u);
        return true;
    }

    std::vector<std::string> findShortestRoute(const std::string& start, const std::string& target) const {
        if (!hasLocation(start) || !hasLocation(target)) return {};
        if (start == target) return {start};

        std::unordered_map<std::string, bool> visited;
        std::unordered_map<std::string, std::string> parent;
        std::queue<std::string> q;

        visited[start] = true;
        q.push(start);

        bool found = false;
        while (!q.empty()) {
            std::string curr = q.front();
            q.pop();

            if (curr == target) {
                found = true;
                break;
            }

            for (const std::string& neighbor : adj.at(curr)) {
                if (!visited[neighbor]) {
                    visited[neighbor] = true;
                    parent[neighbor] = curr;
                    q.push(neighbor);
                }
            }
        }

        if (!found) return {};

        std::vector<std::string> path;
        for (std::string at = target; !at.empty(); ) {
            path.push_back(at);
            if (at == start) break;
            at = parent[at];
        }
        std::reverse(path.begin(), path.end());
        return path;
    }
};

#endif

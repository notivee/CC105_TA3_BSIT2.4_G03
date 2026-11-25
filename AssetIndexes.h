#ifndef ASSET_INDEXES_H
#define ASSET_INDEXES_H

#include "Models.h"
#include <vector>
#include <list>
#include <memory>
#include <iostream>
#include <iomanip>

// --- Binary Search Tree (RAII unique_ptr ownership for leak-free cleanup) ---
class AssetBST {
private:
    struct Node {
        int assetId;
        size_t recordIndex;
        std::unique_ptr<Node> left;
        std::unique_ptr<Node> right;
        Node(int id, size_t idx) : assetId(id), recordIndex(idx) {}
    };

    std::unique_ptr<Node> root;

    void insertNode(std::unique_ptr<Node>& curr, int id, size_t idx) {
        if (!curr) {
            curr = std::make_unique<Node>(id, idx);
            return;
        }
        if (id < curr->assetId) {
            insertNode(curr->left, id, idx);
        } else if (id > curr->assetId) {
            insertNode(curr->right, id, idx);
        }
    }

    int searchNode(const Node* curr, int id) const {
        if (!curr) return -1;
        if (id == curr->assetId) return static_cast<int>(curr->recordIndex);
        if (id < curr->assetId) return searchNode(curr->left.get(), id);
        return searchNode(curr->right.get(), id);
    }

    void inOrderTraverse(const Node* curr, const std::vector<Asset>& records) const {
        if (!curr) return;
        inOrderTraverse(curr->left.get(), records);
        const Asset& a = records[curr->recordIndex];
        std::cout << "  ID: " << std::right << std::setw(4) << a.id
                  << " | Name: " << std::left << std::setw(26) << a.name
                  << " | Loc: " << a.location
                  << " | Status: " << a.status << "\n";
        inOrderTraverse(curr->right.get(), records);
    }

public:
    void insert(int id, size_t recordIndex) {
        insertNode(root, id, recordIndex);
    }

    int search(int id) const {
        return searchNode(root.get(), id);
    }

    void printInOrder(const std::vector<Asset>& records) const {
        if (!root) {
            std::cout << "  [Empty BST] No assets indexed.\n";
            return;
        }
        inOrderTraverse(root.get(), records);
    }
};

// --- Separate-Chaining Hash Table (Mod 7 for clear collision demonstration) ---
class AssetHashTable {
private:
    static const int TABLE_SIZE = 7;
    struct Entry {
        int assetId;
        size_t recordIndex;
    };
    std::vector<std::list<Entry>> buckets;

public:
    AssetHashTable() : buckets(TABLE_SIZE) {}

    int hashFn(int id) const {
        int h = id % TABLE_SIZE;
        return (h < 0) ? (h + TABLE_SIZE) : h;
    }

    void insert(int id, size_t recordIndex) {
        int b = hashFn(id);
        buckets[static_cast<size_t>(b)].push_back({id, recordIndex});
    }

    int search(int id, size_t& bucketChecked, size_t& chainDepth) const {
        bucketChecked = static_cast<size_t>(hashFn(id));
        chainDepth = 0;
        for (const auto& entry : buckets[bucketChecked]) {
            chainDepth++;
            if (entry.assetId == id) {
                return static_cast<int>(entry.recordIndex);
            }
        }
        return -1;
    }

    void printBuckets() const {
        std::cout << "  Hash Table Buckets (size = " << TABLE_SIZE << ", h(id) = id % 7):\n";
        for (int i = 0; i < TABLE_SIZE; ++i) {
            std::cout << "    Bucket [" << i << "]: ";
            const auto& chain = buckets[static_cast<size_t>(i)];
            if (chain.empty()) {
                std::cout << "empty";
            } else {
                for (auto it = chain.begin(); it != chain.end(); ++it) {
                    if (it != chain.begin()) std::cout << " -> ";
                    std::cout << it->assetId;
                }
                if (chain.size() > 1) {
                    std::cout << "  <-- COLLISION CHAIN";
                }
            }
            std::cout << "\n";
        }
    }
};

// --- Comparison Sort (Merge Sort by Name ascending, ID ascending tie-breaker) ---
class AssetReportSorter {
private:
    static bool comesBefore(const Asset& a, const Asset& b) {
        if (a.name != b.name) return a.name < b.name;
        return a.id < b.id;
    }

    static void merge(std::vector<Asset>& arr, int left, int mid, int right) {
        int n1 = mid - left + 1;
        int n2 = right - mid;
        std::vector<Asset> L(arr.begin() + left, arr.begin() + mid + 1);
        std::vector<Asset> R(arr.begin() + mid + 1, arr.begin() + right + 1);

        int i = 0, j = 0, k = left;
        while (i < n1 && j < n2) {
            if (comesBefore(L[static_cast<size_t>(i)], R[static_cast<size_t>(j)])) {
                arr[static_cast<size_t>(k++)] = L[static_cast<size_t>(i++)];
            } else {
                arr[static_cast<size_t>(k++)] = R[static_cast<size_t>(j++)];
            }
        }
        while (i < n1) arr[static_cast<size_t>(k++)] = L[static_cast<size_t>(i++)];
        while (j < n2) arr[static_cast<size_t>(k++)] = R[static_cast<size_t>(j++)];
    }

    static void mergeSort(std::vector<Asset>& arr, int left, int right) {
        if (left >= right) return;
        int mid = left + (right - left) / 2;
        mergeSort(arr, left, mid);
        mergeSort(arr, mid + 1, right);
        merge(arr, left, mid, right);
    }

public:
    static std::vector<Asset> sortByNameThenId(const std::vector<Asset>& records) {
        std::vector<Asset> copy = records;
        if (!copy.empty()) {
            mergeSort(copy, 0, static_cast<int>(copy.size()) - 1);
        }
        return copy;
    }
};

#endif

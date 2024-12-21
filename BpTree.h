#ifndef BPTREE_H
#define BPTREE_H

#include <algorithm>
#include <memory>
#include <vector>
#include <array>
#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>          
#include <queue>       
#include <cmath>      
#include "hnswlib/hnswlib.h"

constexpr int Dimension = 102; 

using VectorType = std::array<float, Dimension>;

template<typename KeyType, int Order = 64>
class BpTree {
public:
    using ValueType = VectorType;

    class Node {
    public:
        bool is_leaf_;
        int count_; 
        Node(bool leaf) : is_leaf_(leaf), count_(0) {}
        virtual ~Node() = default;
    };

    class InternalNode : public Node {
    public:
        KeyType keys_[Order];
        std::shared_ptr<Node> children_[Order + 1];

        InternalNode() : Node(false) {}
    
        int find_child_index(const KeyType &key) const {
            int idx = 0;
            while (idx < this->count_ && key >= keys_[idx]) {
                ++idx;
            }
            return idx;
        }
    };

    class LeafNode : public Node {
    public:
        KeyType keys_[Order];
        std::vector<ValueType> values_[Order];
        std::shared_ptr<LeafNode> next_; 
        LeafNode() : Node(true), next_(nullptr) {}

        int find_key_index(const KeyType& key) const {
            int low = 0;
            int high = this->count_ - 1;
            while (low <= high) {
                int mid = (low + high) / 2;
                if (keys_[mid] == key) return mid;
                else if (keys_[mid] < key) low = mid + 1;
                else high = mid - 1;
            }
            return -1; 
        }

        void insert_in_leaf(const KeyType &key, const VectorType &vector) {
            int pos = 0;
            while (pos < this->count_ && keys_[pos] < key) pos++;

            if (pos < this->count_ && keys_[pos] == key) {
                values_[pos].push_back(vector);
            } else {
                for (int i = this->count_; i > pos; i--) {
                    keys_[i] = keys_[i-1];
                    values_[i] = values_[i-1];
                }

                keys_[pos] = key;
                values_[pos].emplace_back(vector);

                this->count_++;
            }
        }
    };

public:
    // Constructor
    BpTree() : root_(std::make_shared<LeafNode>()) {

    }



    void Insert(const KeyType& key, const VectorType& vector) {
        if (root_->is_leaf_) {
            auto leaf = std::static_pointer_cast<LeafNode>(root_);
            if (leaf->count_ < Order) {
                leaf->insert_in_leaf(key, vector);
                data_multimap.emplace(key, vector);
            } else {
                SplitLeafAndGrowRoot(leaf, key, vector);
                data_multimap.emplace(key, vector);
            }
        } else {
            InsertInternal(root_, key, vector);
            data_multimap.emplace(key, vector);
        }
    }

    bool Search(const KeyType& key, std::vector<ValueType>& out_vectors) const {
        auto node = root_;
        while (!node->is_leaf_) {
            auto internal = std::static_pointer_cast<InternalNode>(node);
            int idx = internal->find_child_index(key);
            node = internal->children_[idx];
        }
        auto leaf = std::static_pointer_cast<LeafNode>(node);
        int idx = leaf->find_key_index(key);
        if (idx >= 0) {
            out_vectors = leaf->values_[idx];
            return true;
        }
        return false;
    }

    void FindRangeIds(const KeyType& c_min, const KeyType& c_max, std::vector<hnswlib::labeltype>& ids) const {
        auto it_low = data_multimap.lower_bound(c_min);
        auto it_up = data_multimap.upper_bound(c_max);
        for(auto it = it_low; it != it_up; ++it){
            ids.push_back(static_cast<hnswlib::labeltype>(it->second[0]));
        }
    }

    int FindRange(const KeyType& c_min, const KeyType& c_max) const {
        auto it_low = data_multimap.lower_bound(c_min);
        auto it_up = data_multimap.upper_bound(c_max);
        return std::distance(it_low, it_up);
    }

private:
    std::shared_ptr<Node> root_;

    void InsertInternal(std::shared_ptr<Node> node, const KeyType& key, const VectorType& vector) {
        if (node->is_leaf_) {
            auto leaf = std::static_pointer_cast<LeafNode>(node);
            if (leaf->count_ < Order) {
                leaf->insert_in_leaf(key, vector);
            } else {
                SplitLeafAndGrowUp(node, key, vector);
            }
        } else {
            auto internal = std::static_pointer_cast<InternalNode>(node);
            int idx = internal->find_child_index(key);
            auto child = internal->children_[idx];
            InsertInternal(child, key, vector);
            if (internal->count_ == Order) {
                SplitInternalNode(internal);
            }
        }
    }

    void SplitLeafAndGrowRoot(std::shared_ptr<LeafNode> leaf, const KeyType& key, const VectorType& vector) {
        KeyType temp_keys[Order + 1];
        std::vector<ValueType> temp_values[Order + 1];

        for (int i = 0; i < leaf->count_; i++) {
            temp_keys[i] = leaf->keys_[i];
            temp_values[i] = leaf->values_[i];
        }

        int pos = 0;
        while (pos < leaf->count_ && temp_keys[pos] < key) pos++;

        if (pos < leaf->count_ && temp_keys[pos] == key) {
            temp_values[pos].push_back(vector);
        } else {
            for (int i = leaf->count_; i > pos; i--) {
                temp_keys[i] = temp_keys[i - 1];
                temp_values[i] = temp_values[i - 1];
            }
            temp_keys[pos] = key;
            temp_values[pos].emplace_back(vector);
            leaf->count_++;
        }

        int new_count = leaf->count_;
        int mid = new_count / 2;

        leaf->count_ = mid;
        for (int i = 0; i < mid; i++) {
            leaf->keys_[i] = temp_keys[i];
            leaf->values_[i] = temp_values[i];
        }

        auto new_leaf = std::make_shared<LeafNode>();
        new_leaf->count_ = new_count - mid;
        for (int i = 0; i < new_leaf->count_; i++) {
            new_leaf->keys_[i] = temp_keys[mid + i];
            new_leaf->values_[i] = temp_values[mid + i];
        }
        new_leaf->next_ = leaf->next_;
        leaf->next_ = new_leaf;
        // Create new root
        auto new_root = std::make_shared<InternalNode>();
        new_root->keys_[0] = new_leaf->keys_[0];
        new_root->children_[0] = leaf;
        new_root->children_[1] = new_leaf;
        new_root->count_ = 1;
        root_ = new_root;
    }

    void SplitLeafAndGrowUp(std::shared_ptr<Node> node, const KeyType &key, const VectorType &vector) {
        auto leaf = std::static_pointer_cast<LeafNode>(node);

        KeyType temp_keys[Order + 1];
        std::vector<ValueType> temp_values[Order + 1];
        for (int i = 0; i < leaf->count_; i++) {
            temp_keys[i] = leaf->keys_[i];
            temp_values[i] = leaf->values_[i];
        }

        int pos = 0;
        while(pos<leaf->count_&&temp_keys[pos]<key) pos++;
        if (pos < leaf->count_ && temp_keys[pos] == key) {
            temp_values[pos].push_back(vector);
        } else {
            for (int i = leaf->count_; i > pos; i--) {
                temp_keys[i] = temp_keys[i - 1];
                temp_values[i] = temp_values[i - 1];
            }
            temp_keys[pos] = key;
            temp_values[pos].emplace_back(vector);
            leaf->count_++;
        }

        int new_count = leaf->count_;
        int mid = new_count / 2;

        leaf->count_ = mid;
        for (int i = 0; i < mid; i++) {
            leaf->keys_[i] = temp_keys[i];
            leaf->values_[i] = temp_values[i];
        }

        auto new_leaf = std::make_shared<LeafNode>();
        new_leaf->count_ = new_count - mid;
        for (int i = 0; i < new_leaf->count_; i++) {
            new_leaf->keys_[i] = temp_keys[mid + i];
            new_leaf->values_[i] = temp_values[mid + i];
        }

        new_leaf->next_ = leaf->next_;
        leaf->next_ = new_leaf;

        if (node == root_) {
            // Create new root
            auto new_root = std::make_shared<InternalNode>();
            new_root->keys_[0] = new_leaf->keys_[0];
            new_root->children_[0] = leaf;
            new_root->children_[1] = new_leaf;
            new_root->count_ = 1;
            root_ = new_root;
        } else {
            InsertIntoParent(node, new_leaf->keys_[0], new_leaf);
        }
    }

    void SplitInternalNode(std::shared_ptr<InternalNode> internal) {
        int mid = internal->count_ / 2;

        auto new_internal = std::make_shared<InternalNode>();
        int move_count = internal->count_ - mid - 1;
        
        for (int i = 0; i < move_count; i++) {
            new_internal->keys_[i] = internal->keys_[mid + 1 + i];
            new_internal->children_[i] = internal->children_[mid + 1 + i];
        }
        new_internal->children_[move_count] = internal->children_[internal->count_];
        new_internal->count_ = move_count;

        KeyType up_key = internal->keys_[mid]; // key to move up
        internal->count_ = mid;

        if (internal == root_) {
            auto new_root = std::make_shared<InternalNode>();
            new_root->keys_[0] = up_key;
            new_root->children_[0] = internal;
            new_root->children_[1] = new_internal;
            new_root->count_ = 1;
            root_ = new_root;
        } else {
            InsertIntoParent(internal, up_key, new_internal);
        }
    }


    void InsertIntoParent(std::shared_ptr<Node> old_node, const KeyType &key, std::shared_ptr<Node> new_node) {
        std::vector<std::shared_ptr<Node>> path;
        FindPathToNode(root_, old_node, path);
        if (path.empty()) {
            auto new_root = std::make_shared<InternalNode>();
            new_root->keys_[0] = key;
            new_root->children_[0] = old_node;
            new_root->children_[1] = new_node;
            new_root->count_ = 1;
            root_ = new_root;
            return;
        }
        auto parent = std::static_pointer_cast<InternalNode>(path.back());
        int pos = 0;
        while (pos < parent->count_ && parent->children_[pos] != old_node) pos++;
        for (int i = parent->count_; i > pos; i--) {
            parent->keys_[i] = parent->keys_[i - 1];
            parent->children_[i + 1] = parent->children_[i];
        }

        parent->keys_[pos] = key;
        parent->children_[pos + 1] = new_node;
        parent->count_++;

        if (parent->count_ == Order) {
            SplitInternalNode(parent);
        }
    }

     bool FindPathToNode(std::shared_ptr<Node> current, std::shared_ptr<Node> target, std::vector<std::shared_ptr<Node>> &path) const {
         if (current == target) {
             return true;
         }
         if (current->is_leaf_) {
             return false;
         }
         auto internal = std::static_pointer_cast<InternalNode>(current);
         for (int i = 0; i <= internal->count_; i++) {
             if (FindPathToNode(internal->children_[i], target, path)) {
                 path.push_back(current);
                 return true;
             }
         }
         return false;
     }

public:
    std::shared_ptr<Node> get_root() const {
        return root_;
    }
    std::multimap<KeyType, ValueType> data_multimap;

};

#endif

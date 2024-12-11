#ifndef BPTREE_H
#define BPTREE_H

#include <algorithm>
#include <memory>
#include <vector>
#include <cassert>
#include <iostream>

template<typename KeyType, typename IDType = uint32_t, int Order = 64>
class BpTree {
public:
    using ValueType = std::vector<IDType>;

    // Base node class
    class Node {
    public:
        bool is_leaf_;
        int count_; // number of keys currently stored
        Node(bool leaf) : is_leaf_(leaf), count_(0) {}
        virtual ~Node() = default;
    };

    // Internal node
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

    // Leaf node
    class LeafNode : public Node {
    public:
        KeyType keys_[Order];
        ValueType values_[Order];
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
            return -1; // not found
        }

        void insert_in_leaf(const KeyType &key, const IDType &id) {
            int pos = find_key_index(key);
            if (pos != -1) {
                // Key exists, append ID
                values_[pos].push_back(id);
                return;
            }
            // Key not found, find insert position
            pos = 0;
            while (pos < this->count_ && keys_[pos] < key) pos++;

            for (int i = this->count_; i > pos; i--) {
                keys_[i] = keys_[i-1];
                values_[i] = values_[i-1];
            }

            keys_[pos] = key;
            values_[pos].clear();
            values_[pos].push_back(id);

            this->count_++;
        }
    };

public:
    BpTree() : root_(std::make_shared<LeafNode>()) {}

    void Insert(const KeyType& key, const IDType& id) {
        if (root_->is_leaf_) {
            auto leaf = std::static_pointer_cast<LeafNode>(root_);
            if (leaf->count_ < Order) {
                leaf->insert_in_leaf(key, id);
            } else {
                // Leaf is full, handle overflow and create a new root
                SplitLeafAndGrowRoot(leaf, key, id);
            }
        } else {
            InsertInternal(root_, key, id);
        }
    }

    bool Search(const KeyType& key, ValueType& out_ids) const {
        auto node = root_;
        while (!node->is_leaf_) {
            auto internal = std::static_pointer_cast<InternalNode>(node);
            int idx = internal->find_child_index(key);
            node = internal->children_[idx];
        }
        auto leaf = std::static_pointer_cast<LeafNode>(node);
        int idx = leaf->find_key_index(key);
        if (idx >= 0) {
            out_ids = leaf->values_[idx];
            return true;
        }
        return false;
    }

private:
    std::shared_ptr<Node> root_;

    void InsertInternal(std::shared_ptr<Node> node, const KeyType& key, const IDType& id) {
        if (node->is_leaf_) {
            auto leaf = std::static_pointer_cast<LeafNode>(node);
            if (leaf->count_ < Order) {
                leaf->insert_in_leaf(key, id);
            } else {
                SplitLeafAndGrowUp(node, key, id);
            }
        } else {
            auto internal = std::static_pointer_cast<InternalNode>(node);
            int idx = internal->find_child_index(key);
            auto child = internal->children_[idx];
            InsertInternal(child, key, id);
            if (internal->count_ == Order) {
                SplitInternalNode(node);
            }
        }
    }

    void SplitLeafAndGrowRoot(std::shared_ptr<LeafNode> leaf, const KeyType& key, const IDType& id) {
        // Temporarily hold all keys/values + new key
        KeyType temp_keys[Order+1];
        ValueType temp_values[Order+1];

        for (int i = 0; i < leaf->count_; i++) {
            temp_keys[i] = leaf->keys_[i];
            temp_values[i] = leaf->values_[i];
        }

        // Insert the new key into the temp arrays
        int pos = 0;
        while (pos < leaf->count_ && temp_keys[pos] < key) pos++;

        for (int i = leaf->count_; i > pos; i--) {
            temp_keys[i] = temp_keys[i-1];
            temp_values[i] = temp_values[i-1];
        }
        temp_keys[pos] = key;
        temp_values[pos].clear();
        temp_values[pos].push_back(id);

        int new_count = leaf->count_ + 1;
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

        // New root
        auto new_root = std::make_shared<InternalNode>();
        new_root->keys_[0] = new_leaf->keys_[0];
        new_root->children_[0] = leaf;
        new_root->children_[1] = new_leaf;
        new_root->count_ = 1;
        root_ = new_root;
    }

    void SplitLeafAndGrowUp(std::shared_ptr<Node> node, const KeyType &key, const IDType &id) {
        auto leaf = std::static_pointer_cast<LeafNode>(node);

        KeyType temp_keys[Order+1];
        ValueType temp_values[Order+1];
        for (int i = 0; i < leaf->count_; i++) {
            temp_keys[i] = leaf->keys_[i];
            temp_values[i] = leaf->values_[i];
        }

        int pos = 0;
        while (pos < leaf->count_ && temp_keys[pos] < key) pos++;
        for (int i = leaf->count_; i > pos; i--) {
            temp_keys[i] = temp_keys[i-1];
            temp_values[i] = temp_values[i-1];
        }

        temp_keys[pos] = key;
        temp_values[pos].clear();
        temp_values[pos].push_back(id);

        int new_count = leaf->count_ + 1;
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

    void SplitInternalNode(std::shared_ptr<Node> node) {
        auto internal = std::static_pointer_cast<InternalNode>(node);
        int mid = internal->count_ / 2;

        auto new_internal = std::make_shared<InternalNode>();
        int move_count = internal->count_ - mid - 1;
        
        for (int i = 0; i < move_count; i++) {
            new_internal->keys_[i] = internal->keys_[mid+1+i];
            new_internal->children_[i] = internal->children_[mid+1+i];
        }
        new_internal->children_[move_count] = internal->children_[internal->count_];
        new_internal->count_ = move_count;

        KeyType up_key = internal->keys_[mid]; // key to move up
        internal->count_ = mid;

        if (node == root_) {
            auto new_root = std::make_shared<InternalNode>();
            new_root->keys_[0] = up_key;
            new_root->children_[0] = internal;
            new_root->children_[1] = new_internal;
            new_root->count_ = 1;
            root_ = new_root;
        } else {
            InsertIntoParent(node, up_key, new_internal);
        }
    }

    void InsertIntoParent(std::shared_ptr<Node> old_node, const KeyType &key, std::shared_ptr<Node> new_node) {
        std::vector<std::shared_ptr<Node>> path;
        FindPathToNode(root_, old_node, path);
        if (path.empty()) {
            // old_node was root
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
            parent->keys_[i] = parent->keys_[i-1];
            parent->children_[i+1] = parent->children_[i];
        }

        parent->keys_[pos] = key;
        parent->children_[pos+1] = new_node;
        parent->count_++;

        if (parent->count_ == Order) {
            SplitInternalNode(parent);
        }
    }

    bool FindPathToNode(std::shared_ptr<Node> current, std::shared_ptr<Node> target, std::vector<std::shared_ptr<Node>> &path) {
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
};

#endif // BPTREE_H

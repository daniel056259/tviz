#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include "tree.hpp"
#include "node.hpp"
#include "../visualizer/visualizer.hpp"

using namespace std;

template <typename T> class BPlusTree;

template <typename T>
class BPlusTreeNode : public DataNode<T> {
    int t; // Minimum degree
    BPlusTreeNode<T>* next; // 리프 노드 연결을 위한 포인터

public:
    BPlusTreeNode(int _t, bool leaf);

    bool search(T k, Visualizer& vis);
    bool insertNonFull(T k, Visualizer& vis);
    bool remove(T k, Visualizer& vis);
    
    void rangeSearchInLeaf(T end, Visualizer& vis, bool& found_any);

    void splitChild(int i, BPlusTreeNode* y, Visualizer& vis);
    int findKey(T k);
    
    void removeFromLeaf(int idx, Visualizer& vis);
    void removeFromInternal(int idx, Visualizer& vis);
    T getPred(int idx);
    T getSucc(int idx);
    void fill(int idx, Visualizer& vis);
    void borrowFromPrev(int idx, Visualizer& vis);
    void borrowFromNext(int idx, Visualizer& vis);
    void merge(int idx, Visualizer& vis);

    bool is_leaf_node();
    void draw(Visualizer& vis);

    friend class BPlusTree<T>;
};

template <typename T>
class BPlusTree : public DataTree<T> {
    int t;
public:
    BPlusTree(int _t);

    bool search(T k);
    bool insert(T k);
    bool remove(T k);
    bool rangeSearch(T begin, T end);
};

// ---------------- Implementation ----------------

template <typename T>
BPlusTreeNode<T>::BPlusTreeNode(int _t, bool leaf) {
    t = _t;
    this->key.resize(2 * t); 
    this->children.resize(2 * t + 1, nullptr);
    this->key_count = 0;
    this->children_count = 0;
    this->next = nullptr;
}

template <typename T>
bool BPlusTreeNode<T>::is_leaf_node() {
    return this->children[0] == nullptr;
}

// ---------------- Search ----------------

template <typename T>
bool BPlusTreeNode<T>::search(T k, Visualizer& vis) {
    int i = 0;
    vis.setMessage("Searching " + DataNode<T>::toString(k) + " in " + (is_leaf_node() ? "Leaf" : "Internal") + " Node");
    vis.setColor(this, Color::YELLOW);
    vis.render();

    while (i < this->key_count && k >= this->key[i]) {
        i++;
    }

    if (is_leaf_node()) {
        for (int j = 0; j < this->key_count; j++) {
             vis.setColor(this, j, Color::YELLOW);
             if (this->key[j] == k) {
                 vis.setMessage("Key found in leaf node!");
                 vis.setColor(this, j, Color::GREEN);
                 vis.render();
                 return true;
             }
             if (this->key[j] > k) break;
        }

        vis.setMessage("Key not found in leaf.");
        vis.setColor(this, Color::RED);
        vis.render();
        return false;
    } 
    else {
        vis.setMessage("Target " + DataNode<T>::toString(k) + " in range.\n-> Moving to child " + DataNode<int>::toString(i));
        vis.setColor(this, Color::RESET);
        vis.render();
        return dynamic_cast<BPlusTreeNode<T>*>(this->children[i])->search(k, vis);
    }
}

// ---------------- Insert ----------------

template <typename T>
void BPlusTreeNode<T>::splitChild(int i, BPlusTreeNode<T>* y, Visualizer& vis) {
    vis.setMessage("Splitting " + string(y->is_leaf_node() ? "Leaf" : "Internal") + " child at index " + DataNode<int>::toString(i));
    vis.setColor(y, Color::RED);
    vis.render();

    BPlusTreeNode<T>* z = new BPlusTreeNode<T>(y->t, y->is_leaf_node());
    
    if (y->is_leaf_node()) {
        z->key_count = t;
        y->key_count = t - 1;

        for (int j = 0; j < t; j++) {
            z->key[j] = y->key[j + t - 1]; 
        }
        
        z->next = y->next;
        y->next = z;

        for (int j = this->key_count; j >= i + 1; j--) {
            this->children[j + 1] = this->children[j];
        }
        this->children[i + 1] = z;
        this->children_count++;

        for (int j = this->key_count - 1; j >= i; j--) {
            this->key[j + 1] = this->key[j];
        }
        
        this->key[i] = z->key[0];
        this->key_count++;
    } 
    else {
        z->key_count = t - 1;

        for (int j = 0; j < t - 1; j++) {
            z->key[j] = y->key[j + t];
        }

        if (!y->is_leaf_node()) {
            for (int j = 0; j < t; j++) {
                z->children[j] = y->children[j + t];
                if(z->children[j]) z->children_count++;
                y->children[j + t] = nullptr;
            }
            y->children_count -= t;
        }

        y->key_count = t - 1;

        for (int j = this->key_count; j >= i + 1; j--) {
            this->children[j + 1] = this->children[j];
        }
        this->children[i + 1] = z;
        this->children_count++;

        for (int j = this->key_count - 1; j >= i; j--) {
            this->key[j + 1] = this->key[j];
        }

        this->key[i] = y->key[t - 1]; 
        this->key_count++;
    }

    vis.setMessage("Split Complete. Key " + DataNode<T>::toString(this->key[i]) + " added to parent.");
    vis.setColor(this, i, Color::MAGENTA);
    vis.setColor(y, Color::RESET);
    vis.setColor(z, Color::RESET);
    vis.render();
}

template <typename T>
bool BPlusTreeNode<T>::insertNonFull(T k, Visualizer& vis) {
    int i = this->key_count - 1;
    vis.setColor(this, Color::YELLOW);
    vis.render();

    if (is_leaf_node()) {
        vis.setMessage("Inserting " + DataNode<T>::toString(k) + " into Leaf.");
        
        for(int idx=0; idx<this->key_count; ++idx) {
            if (this->key[idx] == k) {
                vis.setMessage("Duplicate key in leaf.");
                vis.setColor(this, idx, Color::RED);
                vis.render();
                return false;
            }
        }

        while (i >= 0 && this->key[i] > k) {
            this->key[i + 1] = this->key[i];
            i--;
        }
        this->key[i + 1] = k;
        this->key_count++;
        
        vis.setColor(this, i + 1, Color::GREEN);
        vis.render();
        vis.setColor(this, Color::RESET);
        return true;
    } 
    else {
        while (i >= 0 && this->key[i] > k) {
            i--;
        }
        i++;

        vis.setMessage("Routing to child " + DataNode<int>::toString(i));
        BPlusTreeNode<T>* child = dynamic_cast<BPlusTreeNode<T>*>(this->children[i]);
        
        if (child->key_count == 2 * t - 1) {
            vis.setMessage("Child is full. Splitting.");
            vis.render();
            splitChild(i, child, vis);
            
            if (this->key[i] < k) {
                i++;
            } else if (this->key[i] == k) {
                i++; 
            }
        }
        return dynamic_cast<BPlusTreeNode<T>*>(this->children[i])->insertNonFull(k, vis);
    }
}

// ---------------- Remove ----------------

template <typename T>
int BPlusTreeNode<T>::findKey(T k) {
    int idx = 0;
    while (idx < this->key_count && this->key[idx] < k) idx++;
    return idx;
}

template <typename T>
bool BPlusTreeNode<T>::remove(T k, Visualizer& vis) {
    vis.setColor(this, Color::YELLOW);
    vis.setMessage("Visiting node...");
    vis.render();

    int idx = findKey(k);

    if (is_leaf_node()) {
        if (idx < this->key_count && this->key[idx] == k) {
            removeFromLeaf(idx, vis);
            return true;
        } else {
            vis.setMessage("Key not found in leaf.");
            vis.setColor(this, Color::RED);
            vis.render();
            return false;
        }
    } 
    else {
        if (idx < this->key_count && this->key[idx] == k) {
            idx++; 
        }
        
        BPlusTreeNode<T>* child = dynamic_cast<BPlusTreeNode<T>*>(this->children[idx]);
        bool flag = (idx == this->key_count);

        if (child->key_count < t) {
            vis.setMessage("Child " + DataNode<int>::toString(idx) + " might underflow. Filling...");
            vis.render();
            fill(idx, vis);
        }
        
        if (flag && idx > this->key_count) {
             return dynamic_cast<BPlusTreeNode<T>*>(this->children[idx - 1])->remove(k, vis);
        } else {
             return dynamic_cast<BPlusTreeNode<T>*>(this->children[idx])->remove(k, vis);
        }
    }
}

template <typename T>
void BPlusTreeNode<T>::removeFromLeaf(int idx, Visualizer& vis) {
    vis.setMessage("Removing " + DataNode<T>::toString(this->key[idx]) + " from Leaf.");
    vis.setColor(this, idx, Color::MAGENTA);
    vis.render();

    for (int i = idx + 1; i < this->key_count; ++i)
        this->key[i - 1] = this->key[i];
    this->key_count--;
    
    vis.setColor(this, Color::RESET);
}

template <typename T>
void BPlusTreeNode<T>::fill(int idx, Visualizer& vis) {
    if (idx != 0 && dynamic_cast<BPlusTreeNode<T>*>(this->children[idx - 1])->key_count >= t)
        borrowFromPrev(idx, vis);
    else if (idx != this->key_count && dynamic_cast<BPlusTreeNode<T>*>(this->children[idx + 1])->key_count >= t)
        borrowFromNext(idx, vis);
    else {
        if (idx != this->key_count)
            merge(idx, vis); 
        else
            merge(idx - 1, vis); 
    }
}

template <typename T>
void BPlusTreeNode<T>::borrowFromPrev(int idx, Visualizer& vis) {
    vis.setMessage("Borrowing from Left Sibling.");
    vis.render();

    BPlusTreeNode<T>* child = dynamic_cast<BPlusTreeNode<T>*>(this->children[idx]);
    BPlusTreeNode<T>* sibling = dynamic_cast<BPlusTreeNode<T>*>(this->children[idx - 1]);

    for (int i = child->key_count - 1; i >= 0; --i)
        child->key[i + 1] = child->key[i];
    if (!child->is_leaf_node()) {
        for (int i = child->children_count - 1; i >= 0; --i)
            child->children[i + 1] = child->children[i];
    }

    if (child->is_leaf_node()) {
        child->key[0] = sibling->key[sibling->key_count - 1];
        this->key[idx - 1] = child->key[0];
    } else {
        child->key[0] = this->key[idx - 1];
        this->key[idx - 1] = sibling->key[sibling->key_count - 1];
        
        child->children[0] = sibling->children[sibling->children_count - 1];
        if(child->children[0]) child->children_count++;
        if(sibling->children[sibling->children_count - 1]) sibling->children_count--;
    }

    child->key_count++;
    sibling->key_count--;

    vis.setMessage("Borrow Complete.");
    vis.render();
}

template <typename T>
void BPlusTreeNode<T>::borrowFromNext(int idx, Visualizer& vis) {
    vis.setMessage("Borrowing from Right Sibling.");
    vis.render();

    BPlusTreeNode<T>* child = dynamic_cast<BPlusTreeNode<T>*>(this->children[idx]);
    BPlusTreeNode<T>* sibling = dynamic_cast<BPlusTreeNode<T>*>(this->children[idx + 1]);

    if (child->is_leaf_node()) {
        child->key[child->key_count] = sibling->key[0];
        
        for (int i = 1; i < sibling->key_count; ++i)
            sibling->key[i - 1] = sibling->key[i];
            
        this->key[idx] = sibling->key[0];
    } else {
        child->key[child->key_count] = this->key[idx];
        this->key[idx] = sibling->key[0];
        
        child->children[child->key_count + 1] = sibling->children[0];
        if(child->children[child->key_count + 1]) child->children_count++;
        
        for (int i = 1; i < sibling->key_count; ++i)
            sibling->key[i - 1] = sibling->key[i];
        for (int i = 1; i <= sibling->key_count; ++i) 
             sibling->children[i - 1] = sibling->children[i];
        if(sibling->children_count > 0) sibling->children_count--;
    }

    child->key_count++;
    sibling->key_count--;

    vis.setMessage("Borrow Complete.");
    vis.render();
}

template <typename T>
void BPlusTreeNode<T>::merge(int idx, Visualizer& vis) {
    vis.setMessage("Merging children at index " + DataNode<int>::toString(idx));
    vis.render();

    BPlusTreeNode<T>* child = dynamic_cast<BPlusTreeNode<T>*>(this->children[idx]);
    BPlusTreeNode<T>* sibling = dynamic_cast<BPlusTreeNode<T>*>(this->children[idx + 1]);

    if (child->is_leaf_node()) {
        for (int i = 0; i < sibling->key_count; ++i)
            child->key[i + child->key_count] = sibling->key[i];
        child->key_count += sibling->key_count;
        
        child->next = sibling->next;
    } else {
        child->key[t - 1] = this->key[idx];
        
        for (int i = 0; i < sibling->key_count; ++i)
            child->key[i + t] = sibling->key[i];
            
        for (int i = 0; i <= sibling->key_count; ++i) {
             child->children[i + t] = sibling->children[i];
             if(child->children[i+t]) child->children_count++;
        }
        child->key_count += sibling->key_count + 1;
    }

    for (int i = idx + 1; i < this->key_count; ++i)
        this->key[i - 1] = this->key[i];
    for (int i = idx + 2; i <= this->key_count; ++i)
        this->children[i - 1] = this->children[i];

    this->key_count--;
    this->children_count--;

    delete sibling;
    vis.setMessage("Merge Complete.");
    vis.render();
}

// ---------------- Range Search (Linked List) ----------------

template <typename T>
void BPlusTreeNode<T>::rangeSearchInLeaf(T end, Visualizer& vis, bool& found_any) {
    BPlusTreeNode<T>* current = this;
    
    while (current != nullptr) {
        vis.setColor(current, Color::YELLOW);
        vis.setMessage("Scanning Leaf Node...");
        vis.render();
        
        bool stop = false;
        for (int i = 0; i < current->key_count; i++) {
            T k = current->key[i];
            if (k > end) {
                stop = true;
                break;
            }
            vis.setColor(current, i, Color::GREEN);
            vis.setMessage("Key " + DataNode<T>::toString(k) + " in range!");
            found_any = true;
        }
        vis.render();
        
        if (stop) {
            vis.setColor(current, Color::RESET); 
            break;
        }
        
        if (current->next != nullptr) {
            vis.setMessage("Moving to next Leaf via Linked List ->");
            vis.render();
            vis.setColor(current, Color::RESET); 
            current = current->next;
        } else {
            vis.setMessage("End of Linked List.");
            vis.setColor(current, Color::RESET);
            break;
        }
    }
}

// ---------------- Draw ----------------

template <typename T>
void BPlusTreeNode<T>::draw(Visualizer& vis) {
    int n = this->key_count;
    int mid = n / 2;
    
    if (this->is_leaf_node()) {
        for(int i = n-1; i >=0; --i) {
            if (i == mid) vis.printKey(Pos::MID_NORM, DataNode<T>::toString(this->key[i]), this, i);
            else if (i > mid) vis.printKey(Pos::UP, DataNode<T>::toString(this->key[i]), this, i);
            else vis.printKey(Pos::DOWN, DataNode<T>::toString(this->key[i]), this, i);
        }
        return;
    }

    bool is_odd = (n % 2 != 0);
    if (is_odd) {
        if (this->children[n]) vis.printChild(this->children[n], Pos::UP, this, Pos::UP);
        for (int i = n - 1; i > mid; --i) {
            vis.printKey(Pos::UP, DataNode<T>::toString(this->key[i]), this, i);
            if (this->children[i]) vis.printChild(this->children[i], Pos::MID_NORM, this, Pos::UP);
        }
        vis.printKey(Pos::MID_NORM, DataNode<T>::toString(this->key[mid]), this, mid);
        for (int i = mid - 1; i >= 0; --i) {
            if (this->children[i + 1]) vis.printChild(this->children[i + 1], Pos::MID_NORM, this, Pos::DOWN);
            vis.printKey(Pos::DOWN, DataNode<T>::toString(this->key[i]), this, i);
        }
        if (this->children[0]) vis.printChild(this->children[0], Pos::DOWN, this, Pos::DOWN);
    }
    else {
        if (this->children[n]) vis.printChild(this->children[n], Pos::UP, this, Pos::UP);
        for (int i = n - 1; i >= mid + 1; --i) {
            vis.printKey(Pos::UP, DataNode<T>::toString(this->key[i]), this, i);
            if (this->children[i]) vis.printChild(this->children[i], Pos::MID_NORM, this, Pos::UP);
        }
        vis.printKey(Pos::UP, DataNode<T>::toString(this->key[mid]), this, mid);
        if (this->children[mid]) vis.printChild(this->children[mid], Pos::MID_EVEN, this, Pos::MID_EVEN);
        for (int i = mid - 1; i > 0; --i) {
            vis.printKey(Pos::DOWN, DataNode<T>::toString(this->key[i]), this, i);
            if (this->children[i]) vis.printChild(this->children[i], Pos::MID_NORM, this, Pos::DOWN);
        }
        vis.printKey(Pos::DOWN, DataNode<T>::toString(this->key[0]), this, 0);
        if (this->children[0]) vis.printChild(this->children[0], Pos::DOWN, this, Pos::DOWN);
    }
}


// ---------------- BPlusTree Class ----------------

template <typename T>
BPlusTree<T>::BPlusTree(int _t) : t(_t) {}

template <typename T>
bool BPlusTree<T>::search(T k) {
    this->vis->clear();
    this->vis->setTitle("Searching: " + DataNode<T>::toString(k));
    if (!this->root_ptr) {
        this->vis->setMessage("Tree is Empty.");
        this->vis->render();
        return false;
    }
    return dynamic_cast<BPlusTreeNode<T>*>(this->root_ptr)->search(k, *(this->vis));
}

template <typename T>
bool BPlusTree<T>::insert(T k) {
    this->vis->clear();
    this->vis->setTitle("Inserting: " + DataNode<T>::toString(k));
    
    if (this->root_ptr == nullptr) {
        this->vis->setMessage("Empty Tree. Creating Root Leaf.");
        this->vis->render();
        
        BPlusTreeNode<T>* root = new BPlusTreeNode<T>(t, true);
        root->key[0] = k;
        root->key_count = 1;
        this->setRoot(root);
        
        this->vis->setColor(this->root_ptr, 0, Color::GREEN);
        this->vis->render();
        return true;
    } else {
        BPlusTreeNode<T>* r = dynamic_cast<BPlusTreeNode<T>*>(this->root_ptr);
        if (r->key_count == 2 * t - 1) {
            this->vis->setMessage("Root is full. Splitting.");
            this->vis->render();
            
            BPlusTreeNode<T>* s = new BPlusTreeNode<T>(t, false);
            s->children[0] = r;
            s->children_count = 1;
            
            s->splitChild(0, r, *(this->vis));
            this->setRoot(s);
            
            int i = 0;
            if (s->key[0] < k) i++;
            
            return dynamic_cast<BPlusTreeNode<T>*>(s->children[i])->insertNonFull(k, *(this->vis));
        } else {
            return r->insertNonFull(k, *(this->vis));
        }
    }
}

template <typename T>
bool BPlusTree<T>::remove(T k) {
    this->vis->clear();
    this->vis->setTitle("Removing: " + DataNode<T>::toString(k));
    if (!this->root_ptr) {
        this->vis->setMessage("Tree is Empty.");
        this->vis->render();
        return false;
    }
    
    BPlusTreeNode<T>* root = dynamic_cast<BPlusTreeNode<T>*>(this->root_ptr);
    bool result = root->remove(k, *(this->vis));
    
    if (root->key_count == 0 && !root->is_leaf_node()) {
        this->vis->setMessage("Root is empty. Shrinking height.");
        this->vis->render();
        
        BPlusTreeNode<T>* new_root = dynamic_cast<BPlusTreeNode<T>*>(root->children[0]);
        this->setRoot(new_root);
        delete root;
    } else if (root->key_count == 0 && root->is_leaf_node()) {
        this->setRoot(nullptr);
        delete root;
    }
    
    this->vis->clear();
    this->vis->setMessage(result ? "Removal Complete." : "Key not found.");
    this->vis->render();
    return result;
}

template <typename T>
bool BPlusTree<T>::rangeSearch(T begin, T end) {
    this->vis->clear();
    this->vis->setTitle("Range Search [" + DataNode<T>::toString(begin) + ", " + DataNode<T>::toString(end) + "]");
    if (!this->root_ptr) return false;

    this->vis->setMessage("Locating starting Leaf Node for " + DataNode<T>::toString(begin));
    this->vis->render();
    
    BPlusTreeNode<T>* curr = dynamic_cast<BPlusTreeNode<T>*>(this->root_ptr);
    while (!curr->is_leaf_node()) {
        int i = 0;
        while (i < curr->key_count && begin >= curr->key[i]) i++;
        curr = dynamic_cast<BPlusTreeNode<T>*>(curr->children[i]);
    }
    
    bool found_any = false;
    BPlusTreeNode<T>* leaf = curr;
    while (leaf != nullptr) {
        bool stop = false;
        this->vis->setColor(leaf, Color::YELLOW);
        this->vis->render();
        
        for (int i = 0; i < leaf->key_count; i++) {
            T k = leaf->key[i];
            if (k > end) {
                stop = true; 
                break;
            }
            if (k >= begin) {
                this->vis->setColor(leaf, i, Color::GREEN);
                this->vis->setMessage("Found " + DataNode<T>::toString(k));
                found_any = true;
            }
        }
        this->vis->render();
        
        if (stop) break;
        leaf = leaf->next;
        if (leaf) {
            this->vis->setMessage("Following Linked List ->");
            this->vis->render();
            this->vis->setColor(dynamic_cast<BPlusTreeNode<T>*>(this->root_ptr), Color::RESET); 
        }
    }
    
    this->vis->setMessage(found_any ? "Range Search Done." : "No keys in range.");
    this->vis->render();
    return found_any;
}
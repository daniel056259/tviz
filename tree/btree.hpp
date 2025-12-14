#pragma once
#include <string>
#include "tree.hpp"
#include "node.hpp"
#include "../visualizer/visualizer.hpp"

template <typename T> class BTree;

template <typename T>
class BTreeNode : public DataNode<T> {
    int t; // Minimum degree
public:
    BTreeNode(int _t, bool leaf);

    bool search(T k, Visualizer& vis);
    bool insertNonFull(T k, Visualizer& vis);
    bool remove(T k, Visualizer &vis);
    void rangeSearch(T begin, T end, Visualizer &vis, bool &found_any);

    void splitChild(int i, BTreeNode* y, Visualizer& vis);

    int findKey(T k);
    void removeFromLeaf(int idx, Visualizer &vis);
    void removeFromNonLeaf(int idx, Visualizer &vis);
    T getPredecessor(int idx);
    T getSuccessor(int idx);
    void fill(int idx, Visualizer &vis);
    void borrowFromPrev(int idx, Visualizer &vis);
    void borrowFromNext(int idx, Visualizer &vis);
    void merge(int idx, Visualizer &vis);

    bool is_leaf_node();

    void draw(Visualizer& vis);

    friend class BTree<T>;
};

template <typename T>
class BTree : public DataTree<T> {
    int t; // Minimum degree
public:
    BTree(int _t);

    bool search(T k);
    bool insert(T k);
    bool remove(T k);
    bool rangeSearch(T begin, T end);
};


template <typename T>
BTreeNode<T>::BTreeNode(int _t, bool leaf) {
    t = _t;
    // B-Tree 노드의 최대 키 개수: 2*t - 1
    // 최대 자식 개수: 2*t
    this->key.resize(2 * t - 1);
    this->children.resize(2 * t, nullptr);
    this->key_count = 0;
    
    // 리프 노드일 경우 children_count는 0 (is_leaf() 판단용)
    // 내부 노드일 경우 나중에 자식이 추가되면 children_count 업데이트
    this->children_count = 0; 
}

template <typename T>
bool BTreeNode<T>::is_leaf_node() {
    for(auto c : this->children) {
        if(c != nullptr) return false;
    }
    return true;
}

template <typename T>
bool BTreeNode<T>::search(T k, Visualizer& vis) {
    int i = 0;
    
    vis.setMessage("Searching for " + DataNode<T>::toString(k) + " in current node...");
    vis.setColor(this, Color::YELLOW);
    vis.render();

    while (i < this->key_count && k > this->key[i]) {
        vis.setColor(this, i, Color::CYAN);
        i++;
    }
    vis.render();

    if (i < this->key_count && this->key[i] == k) {
        vis.setMessage("Key " + DataNode<T>::toString(k) + " found!");
        vis.setColor(this, i, Color::GREEN);
        vis.render();
        return true;
    }

    if (is_leaf_node()) {
        vis.setMessage("Reached leaf node. Key not found.");
        vis.setColor(this, Color::RED);
        vis.render();
        return false;
    }

    vis.setMessage("Key > " + (i > 0 ? DataNode<T>::toString(this->key[i-1]) : "-INF") + 
                   " and Key < " + (i < this->key_count ? DataNode<T>::toString(this->key[i]) : "INF") + 
                   "\n-> Moving to child index " + DataNode<int>::toString(i));
    vis.setColor(this, Color::RESET);
    vis.render();

    return dynamic_cast<BTreeNode*>(this->children[i])->search(k, vis);
}

template <typename T>
void BTreeNode<T>::splitChild(int i, BTreeNode<T>* y, Visualizer& vis) {
    vis.setMessage("Splitting full child node at index " + DataNode<int>::toString(i));
    vis.setColor(y, Color::RED);
    vis.render();

    BTreeNode<T>* z = new BTreeNode<T>(y->t, y->is_leaf_node());
    z->key_count = t - 1;

    for (int j = 0; j < t - 1; j++) {
        z->key[j] = y->key[j + t];
    }

    if (!y->is_leaf_node()) {
        for (int j = 0; j < t; j++) {
            z->children[j] = y->children[j + t];
            if (z->children[j]) z->children_count++;
            y->children[j + t] = nullptr;
            if (y->children_count > 0) y->children_count--;
        }
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

    vis.setMessage("Split complete. Median " + DataNode<T>::toString(this->key[i]) + " moved up.");
    vis.setColor(this, i, Color::MAGENTA);
    vis.setColor(y, Color::RESET);
    vis.render();
}

template <typename T>
bool BTreeNode<T>::insertNonFull(T k, Visualizer& vis) {
    int i = this->key_count - 1;

    vis.setColor(this, Color::YELLOW);
    vis.render();

    int check_idx = 0;
    while (check_idx < this->key_count && this->key[check_idx] < k) check_idx++;
    if (check_idx < this->key_count && this->key[check_idx] == k) {
        vis.setMessage("Key " + DataNode<T>::toString(k) + " already exists.");
        vis.setColor(this, check_idx, Color::RED);
        vis.render();
        return false;
    }

    if (is_leaf_node()) {
        vis.setMessage("Inserting " + DataNode<T>::toString(k) + " into leaf node.");
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
    } else {
        while (i >= 0 && this->key[i] > k) {
            i--;
        }
        i++;

        vis.setMessage("Moving down to child " + DataNode<int>::toString(i));
        
        BTreeNode<T>* child = dynamic_cast<BTreeNode<T>*>(this->children[i]);
        
        if (child->key_count == 2 * t - 1) {
            vis.setMessage("Child is full. Splitting first.");
            vis.render();
            
            splitChild(i, child, vis);

            if (this->key[i] < k) {
                i++;
            }
        }
        
        return dynamic_cast<BTreeNode<T>*>(this->children[i])->insertNonFull(k, vis);
    }
}

template <typename T>
int BTreeNode<T>::findKey(T k) {
    int idx = 0;
    while (idx < this->key_count && this->key[idx] < k) ++idx;
    return idx;
}

template <typename T>
bool BTreeNode<T>::remove(T k, Visualizer& vis) {
    vis.setColor(this, Color::YELLOW);
    vis.setMessage("Visiting node to remove " + DataNode<T>::toString(k));
    vis.render();

    int idx = findKey(k);

    // Case 1: The key k is in this node
    if (idx < this->key_count && this->key[idx] == k) {
        vis.setMessage("Found key " + DataNode<T>::toString(k) + " in this node.");
        vis.setColor(this, idx, Color::MAGENTA);
        vis.render();

        if (is_leaf_node()) {
            removeFromLeaf(idx, vis);
        } else {
            removeFromNonLeaf(idx, vis);
        }
        vis.setColor(this, Color::RESET);
        return true;
    }
    else {
        // Case 2: The key k is not in this node
        if (is_leaf_node()) {
            vis.setMessage("Reached leaf and key " + DataNode<T>::toString(k) + " not found.");
            vis.setColor(this, Color::RED);
            vis.render();
            return false;
        }

        // Flag to indicate if the key is present in the sub-tree rooted at the last child
        bool flag = (idx == this->key_count);
        
        BTreeNode<T>* child = dynamic_cast<BTreeNode<T>*>(this->children[idx]);

        if (child->key_count < t) {
            vis.setMessage("Child " + DataNode<int>::toString(idx) + " has too few keys. Filling...");
            vis.render();
            fill(idx, vis);
        }

        // If the last child has been merged, it must have merged with the previous child
        // so we recurse on the (idx-1)th child. Else, we recurse on the (idx)th child
        if (flag && idx > this->key_count) {
             return dynamic_cast<BTreeNode<T>*>(this->children[idx - 1])->remove(k, vis);
        } else {
             return dynamic_cast<BTreeNode<T>*>(this->children[idx])->remove(k, vis);
        }
    }
}

template <typename T>
void BTreeNode<T>::removeFromLeaf(int idx, Visualizer& vis) {
    vis.setMessage("Removing " + DataNode<T>::toString(this->key[idx]) + " from leaf.");
    vis.render();
    for (int i = idx + 1; i < this->key_count; ++i)
        this->key[i - 1] = this->key[i];
    this->key_count--;
}

template <typename T>
void BTreeNode<T>::removeFromNonLeaf(int idx, Visualizer& vis) {
    T k = this->key[idx];
    BTreeNode<T>* leftChild = dynamic_cast<BTreeNode<T>*>(this->children[idx]);
    BTreeNode<T>* rightChild = dynamic_cast<BTreeNode<T>*>(this->children[idx + 1]);

    if (leftChild->key_count >= t) {
        vis.setMessage("Left child has enough keys. Finding predecessor.");
        vis.render();
        T pred = getPredecessor(idx);
        this->key[idx] = pred;
        vis.setMessage("Replaced " + DataNode<T>::toString(k) + " with predecessor " + DataNode<T>::toString(pred));
        vis.render();
        leftChild->remove(pred, vis);
    }
    else if (rightChild->key_count >= t) {
        vis.setMessage("Right child has enough keys. Finding successor.");
        vis.render();
        T succ = getSuccessor(idx);
        this->key[idx] = succ;
        vis.setMessage("Replaced " + DataNode<T>::toString(k) + " with successor " + DataNode<T>::toString(succ));
        vis.render();
        rightChild->remove(succ, vis);
    }
    else {
        vis.setMessage("Both children have t-1 keys. Merging them.");
        vis.render();
        merge(idx, vis);
        leftChild->remove(k, vis);
    }
}

template <typename T>
T BTreeNode<T>::getPredecessor(int idx) {
    BTreeNode<T>* cur = dynamic_cast<BTreeNode<T>*>(this->children[idx]);
    while (!cur->is_leaf_node())
        cur = dynamic_cast<BTreeNode<T>*>(cur->children[cur->key_count]);
    return cur->key[cur->key_count - 1];
}

template <typename T>
T BTreeNode<T>::getSuccessor(int idx) {
    BTreeNode<T>* cur = dynamic_cast<BTreeNode<T>*>(this->children[idx + 1]);
    while (!cur->is_leaf_node())
        cur = dynamic_cast<BTreeNode<T>*>(cur->children[0]);
    return cur->key[0];
}

template <typename T>
void BTreeNode<T>::fill(int idx, Visualizer& vis) {
    if (idx != 0 && dynamic_cast<BTreeNode<T>*>(this->children[idx - 1])->key_count >= t)
        borrowFromPrev(idx, vis);
    else if (idx != this->key_count && dynamic_cast<BTreeNode<T>*>(this->children[idx + 1])->key_count >= t)
        borrowFromNext(idx, vis);
    else {
        if (idx != this->key_count)
            merge(idx, vis);
        else
            merge(idx - 1, vis);
    }
}

template <typename T>
void BTreeNode<T>::borrowFromPrev(int idx, Visualizer& vis) {
    vis.setMessage("Borrowing from left sibling.");
    vis.render();

    BTreeNode<T>* child = dynamic_cast<BTreeNode<T>*>(this->children[idx]);
    BTreeNode<T>* sibling = dynamic_cast<BTreeNode<T>*>(this->children[idx - 1]);

    for (int i = child->key_count - 1; i >= 0; --i)
        child->key[i + 1] = child->key[i];

    if (!child->is_leaf_node()) {
        for (int i = child->key_count; i >= 0; --i)
            child->children[i + 1] = child->children[i];
    }

    child->key[0] = this->key[idx - 1];

    if (!child->is_leaf_node())
        child->children[0] = sibling->children[sibling->key_count];

    this->key[idx - 1] = sibling->key[sibling->key_count - 1];

    child->key_count += 1;
    sibling->key_count -= 1;
    
    // Update children counts if needed for consistency, though key_count is primary
    if(!child->is_leaf_node()) child->children_count++;
    if(!sibling->is_leaf_node()) sibling->children_count--;

    vis.setMessage("Borrow complete.");
    vis.render();
}

template <typename T>
void BTreeNode<T>::borrowFromNext(int idx, Visualizer& vis) {
    vis.setMessage("Borrowing from right sibling.");
    vis.render();

    BTreeNode<T>* child = dynamic_cast<BTreeNode<T>*>(this->children[idx]);
    BTreeNode<T>* sibling = dynamic_cast<BTreeNode<T>*>(this->children[idx + 1]);

    child->key[child->key_count] = this->key[idx];

    if (!child->is_leaf_node())
        child->children[child->key_count + 1] = sibling->children[0];

    this->key[idx] = sibling->key[0];

    for (int i = 1; i < sibling->key_count; ++i)
        sibling->key[i - 1] = sibling->key[i];

    if (!sibling->is_leaf_node()) {
        for (int i = 1; i <= sibling->key_count; ++i)
            sibling->children[i - 1] = sibling->children[i];
    }

    child->key_count += 1;
    sibling->key_count -= 1;
    
    if(!child->is_leaf_node()) child->children_count++;
    if(!sibling->is_leaf_node()) sibling->children_count--;

    vis.setMessage("Borrow complete.");
    vis.render();
}

template <typename T>
void BTreeNode<T>::merge(int idx, Visualizer& vis) {
    vis.setMessage("Merging children at index " + DataNode<int>::toString(idx));
    vis.render();

    BTreeNode<T>* child = dynamic_cast<BTreeNode<T>*>(this->children[idx]);
    BTreeNode<T>* sibling = dynamic_cast<BTreeNode<T>*>(this->children[idx + 1]);

    child->key[t - 1] = this->key[idx];

    for (int i = 0; i < sibling->key_count; ++i)
        child->key[i + t] = sibling->key[i];

    if (!child->is_leaf_node()) {
        for (int i = 0; i <= sibling->key_count; ++i)
            child->children[i + t] = sibling->children[i];
    }

    for (int i = idx + 1; i < this->key_count; ++i)
        this->key[i - 1] = this->key[i];

    for (int i = idx + 2; i <= this->key_count; ++i)
        this->children[i - 1] = this->children[i];

    child->key_count += sibling->key_count + 1;
    child->children_count += sibling->children_count; // Rough update
    this->key_count--;
    this->children_count--;

    delete sibling;
    vis.setMessage("Merge complete.");
    vis.render();
}

template <typename T>
void BTreeNode<T>::rangeSearch(T begin, T end, Visualizer& vis, bool& found_any) {
    int i = 0;
    
    while (i < this->key_count) {
        T current_key = this->key[i];
        bool in_range = (current_key >= begin && current_key <= end);

        if (!this->is_leaf_node() && current_key > begin) {
            vis.setMessage("Key " + DataNode<T>::toString(current_key) + " > Begin (" + DataNode<T>::toString(begin) + ")\n-> Exploring child " + DataNode<int>::toString(i));
            vis.render();
            
            dynamic_cast<BTreeNode<T>*>(this->children[i])->rangeSearch(begin, end, vis, found_any);
            
            vis.setColor(this, i, Color::YELLOW);
            vis.setMessage("Back to key " + DataNode<T>::toString(current_key));
            vis.render();
        }

        vis.setColor(this, i, Color::YELLOW);
        vis.setMessage("Visiting key " + DataNode<T>::toString(current_key));
        vis.render();

        if (in_range) {
            vis.setColor(this, i, Color::GREEN);
            vis.setMessage(DataNode<T>::toString(current_key) + " is in range [" + DataNode<T>::toString(begin) + ", " + DataNode<T>::toString(end) + "]");
            found_any = true;
        } else {
            vis.setColor(this, i, Color::RESET);
            vis.setMessage(DataNode<T>::toString(current_key) + " is out of range.");
        }
        vis.render();

        if (current_key > end) {
            return; 
        }

        i++;
    }

    if (!this->is_leaf_node() && this->key[i - 1] < end) {
        vis.setMessage("Last key < End (" + DataNode<T>::toString(end) + ")\n-> Exploring last child " + DataNode<int>::toString(i));
        vis.render();

        dynamic_cast<BTreeNode<T>*>(this->children[i])->rangeSearch(begin, end, vis, found_any);

        vis.setMessage("Back from last child of node...");
        vis.render();
    }
}

template <typename T>
void BTreeNode<T>::draw(Visualizer& vis) {
    int n = this->key_count;
    int mid = n / 2;
    bool is_odd = (n % 2 != 0);

    if (is_odd) {
        if (!this->is_leaf() && this->children[n]) vis.printChild(this->children[n], Pos::UP, this, Pos::UP);
        for (int i = n - 1; i > mid; --i) {
            vis.printKey(Pos::UP, DataNode<T>::toString(this->key[i]), this, i);

            if (!this->is_leaf() && this->children[i]) 
                vis.printChild(this->children[i], Pos::MID_NORM, this, Pos::UP);
            else {
                vis.printKeyConnection(this, Pos::UP);
            }
        }

        vis.printKey(Pos::MID_NORM, DataNode<T>::toString(this->key[mid]), this, mid);

        for (int i = mid - 1; i >= 0; --i) {
            if (!this->is_leaf() && this->children[i + 1])
                vis.printChild(this->children[i + 1], Pos::MID_NORM, this, Pos::DOWN);
            else
                vis.printKeyConnection(this, Pos::DOWN);

            vis.printKey(Pos::DOWN, DataNode<T>::toString(this->key[i]), this, i);
        }
        if (!this->is_leaf() && this->children[0])
            vis.printChild(this->children[0], Pos::DOWN, this, Pos::DOWN);
    }
    else {
        if (!this->is_leaf() && this->children[n]) vis.printChild(this->children[n], Pos::UP, this, Pos::UP);
        for (int i = n - 1; i >= mid + 1; --i) {
            vis.printKey(Pos::UP, DataNode<T>::toString(this->key[i]), this, i);
            
            if (!this->is_leaf() && this->children[i])
                vis.printChild(this->children[i], Pos::MID_NORM, this, Pos::UP);
            else if (i != mid)
                vis.printKeyConnection(this, Pos::UP);
        }
        vis.printKey(Pos::UP, DataNode<T>::toString(this->key[mid]), this, mid);

        if (!this->is_leaf() && this->children[mid]) {
            vis.printChild(this->children[mid], Pos::MID_EVEN, this, Pos::MID_EVEN);
        }
        else 
            vis.printKeyConnection(this, Pos::MID_EVEN);

        for (int i = mid - 1; i > 0; --i) {
            vis.printKey(Pos::DOWN, DataNode<T>::toString(this->key[i]), this, i);

            if (!this->is_leaf() && this->children[i])
                vis.printChild(this->children[i], Pos::MID_NORM, this, Pos::DOWN);
            else
                vis.printKeyConnection(this, Pos::MID_EVEN);
        }
        vis.printKey(Pos::DOWN, DataNode<T>::toString(this->key[0]), this, 0);
        if (!this->is_leaf() && this->children[0])
            vis.printChild(this->children[0], Pos::DOWN, this, Pos::DOWN);
    }
}

template <typename T>
BTree<T>::BTree(int _t) : t(_t) {}

template <typename T>
bool BTree<T>::search(T k) {
    this->vis->clear();
    this->vis->setTitle("Searching for: " + DataNode<T>::toString(k));
    if (this->root_ptr == nullptr) {
        this->vis->setMessage("Tree is empty.");
        this->vis->render();
        return false;
    }
    return dynamic_cast<BTreeNode<T>*>(this->root_ptr)->search(k, *(this->vis));
}

template <typename T>
bool BTree<T>::insert(T k) {
    this->vis->clear();
    this->vis->setTitle("Inserting: " + DataNode<T>::toString(k));

    bool inserted = false;

    if (this->root_ptr == nullptr) {
        this->vis->setMessage("Tree is empty. Creating root.");
        this->vis->render();

        BTreeNode<T>* root = new BTreeNode<T>(t, true);
        root->key[0] = k;
        root->key_count = 1;
        this->setRoot(root);

        this->vis->setColor(this->root_ptr, 0, Color::GREEN);
        this->vis->render();
        inserted = true;
    } else {
        BTreeNode<T>* r = dynamic_cast<BTreeNode<T>*>(this->root_ptr);
        
        if (r->key_count == 2 * t - 1) {
            this->vis->setMessage("Root is full. Growing tree height.");
            this->vis->setColor(this->root_ptr, Color::RED);
            this->vis->render();

            BTreeNode<T>* s = new BTreeNode<T>(t, false);
            
            s->children[0] = r;
            s->children_count = 1; 
            
            s->splitChild(0, r, *(this->vis));

            int i = 0;
            if (s->key[0] < k) {
                i++;
            }
            
            this->setRoot(s);
            
            inserted = dynamic_cast<BTreeNode<T>*>(s->children[i])->insertNonFull(k, *(this->vis));

        } else {
            inserted = r->insertNonFull(k, *(this->vis));
        }
    }
    this->vis->clear();
    this->vis->setMessage("Insertion complete.");
    this->vis->render();

    return inserted;
}

template <typename T>
bool BTree<T>::remove(T k) {
    this->vis->clear();
    this->vis->setTitle("Removing: " + DataNode<T>::toString(k));
    
    if (this->root_ptr == nullptr) {
        this->vis->setMessage("Tree is empty.");
        this->vis->render();
        return false;
    }

    BTreeNode<T>* root = dynamic_cast<BTreeNode<T>*>(this->root_ptr);
    bool result = root->remove(k, *(this->vis));

    if (root->key_count == 0) {
        if (root->is_leaf_node()) {
            this->setRoot(nullptr);
        } else {
            this->setRoot(dynamic_cast<BTreeNode<T>*>(root->children[0]));
        }
        delete root;
    }

    this->vis->clear();
    if(result) this->vis->setMessage("Removal complete.");
    else this->vis->setMessage("Key not found.");
    this->vis->render();

    return result;
}

template <typename T>
bool BTree<T>::rangeSearch(T begin, T end) {
    this->vis->clear();
    this->vis->setTitle("Range Search [" + DataNode<T>::toString(begin) + " ~ " + DataNode<T>::toString(end) + "]");
    this->vis->render();

    if (this->root_ptr == nullptr) {
        this->vis->setMessage("Tree is empty.");
        this->vis->render();
        return false;
    }

    bool found_any = false;
    dynamic_cast<BTreeNode<T>*>(this->root_ptr)->rangeSearch(begin, end, *(this->vis), found_any);

    if (found_any) {
        this->vis->setMessage("Range search finished.\nGreen nodes are in the range.");
    } else {
        this->vis->setMessage("Range search finished.\nNo nodes found in the range.");
    }
    this->vis->render();
    return found_any;
}
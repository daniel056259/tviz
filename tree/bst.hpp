#pragma once
#include <vector>
#include <string>
#include "tree.hpp"
#include "node.hpp"
#include "../visualizer/visualizer.hpp"

enum class Color;

template <typename T>
class BSTNode : public DataNode<T> {
public:
    BSTNode(T k);

    bool search(T target, Visualizer& vis);
    bool insert(T entry, Visualizer& vis);
    DataNode<T>* remove(T target, Visualizer& vis);

    void rangeSearch(T begin, T end, Visualizer &vis, bool &found_any);

    void draw(Visualizer& vis);

private:
    BSTNode<T>* minValueNode(BSTNode<T>* node);
};

template <typename T>
class BST : public DataTree<T> {
public:
    bool search(T target);
    bool insert(T entry);
    bool remove(T target);
    bool rangeSearch(T begin, T end);
};

template <typename T>
BSTNode<T>::BSTNode(T k) {
    this->key.resize(1);
    this->key[0] = k;
    this->key_count = 1;
    this->children.resize(2);
    this->children_count = 0;
}

template <typename T>
bool BSTNode<T>::search(T target, Visualizer& vis) {
    vis.setMessage("Comparing key with target");
    vis.setColor(this, Color::YELLOW);
    vis.render();

    if (target == this->key[0]) {
        vis.setMessage("Found target!");
        vis.setColor(this, Color::GREEN);
        vis.render();
        return true;
    }
    if (this->is_leaf()) {
        vis.setMessage("Target not found until reaching leaf node");
        vis.setColor(this, Color::RED);
        vis.render();
        return false;
    }
    if (target < this->key[0]) {
        if (this->children[0] == nullptr) {
            vis.setMessage("Target not found. End of left path.");
            vis.setColor(this, Color::RED);
            vis.render();
            return false;
        }
        vis.setMessage("Target < Key \n-> Moving to left child");
        vis.setColor(this, Color::CYAN);
        vis.render();
        return dynamic_cast<BSTNode<T> *>(this->children[0])->search(target, vis);
    }
    else {
        if (this->children[1] == nullptr) {
            vis.setMessage("Target not found. End of right path.");
            vis.setColor(this, Color::RED);
            vis.render();
            return false;
        }
        vis.setMessage("Target > Key \n-> Moving to right child");
        vis.setColor(this, Color::CYAN);
        vis.render();
        return dynamic_cast<BSTNode<T> *>(this->children[1])->search(target, vis);
    }
}

template <typename T>
bool BSTNode<T>::insert(T entry, Visualizer& vis) {
    vis.setColor(this, Color::YELLOW);
    vis.setMessage("Comparing entry " + this->toString(entry) + " with key " + this->toString(this->key[0]));
    vis.render();

    if (entry == this->key[0]) {
        vis.setMessage("Entry " + this->toString(entry) + " already exists. Insertion failed.");
        vis.setColor(this, Color::RED);
        vis.render();
        return false;
    }
    
    if (entry < this->key[0]) {
        if (this->children[0] == nullptr) {
            this->children[0] = new BSTNode<T>{entry};
            this->children_count++;
            
            vis.setMessage("Inserting " + this->toString(entry) + " as the left child.");
            vis.setColor(this, Color::CYAN);
            vis.setColor(this->children[0], Color::GREEN);
            vis.render();
            
            return true;
        }
        
        vis.setMessage("Key > Entry\n-> Moving left.");
        vis.setColor(this, Color::CYAN);
        vis.render();
        
        return dynamic_cast<BSTNode<T> *>(this->children[0])->insert(entry, vis);
    }
    else {
        if (this->children[1] == nullptr) {
            this->children[1] = new BSTNode<T>{entry};
            this->children_count++;
            
            vis.setMessage("Inserting " + this->toString(entry) + " as the right child.");
            vis.setColor(this, Color::CYAN); 
            vis.setColor(this->children[1], Color::GREEN);
            vis.render();

            return true;
        }
        
        vis.setMessage("Entry > Key\n-> Moving right.");
        vis.setColor(this, Color::CYAN);
        vis.render();
        
        return dynamic_cast<BSTNode<T> *>(this->children[1])->insert(entry, vis);
    }
}

template <typename T>
DataNode<T>* BSTNode<T>::remove(T target, Visualizer& vis) {
    vis.setColor(this, Color::YELLOW);
    vis.setMessage("Visiting node " + this->toString(this->key[0]) + " to find target " + this->toString(target));
    vis.render();

    if (target < this->key[0]) {
        if (this->children[0] == nullptr) {
            vis.setMessage("Target not found.");
            vis.setColor(this, Color::RED);
            vis.render();
            return this;
        }
        vis.setMessage("Target < Key\n-> Moving left");
        vis.setColor(this, Color::CYAN);
        vis.render();
        
        this->children[0] = dynamic_cast<BSTNode<T>*>(this->children[0])->remove(target, vis);
        return this;
    }
    else if (target > this->key[0]) {
        if (this->children[1] == nullptr) {
            vis.setMessage("Target not found.");
            vis.setColor(this, Color::RED);
            vis.render();
            return this; 
        }
        vis.setMessage("Target > Key\n-> Moving right.");
        vis.setColor(this, Color::CYAN);
        vis.render();

        this->children[1] = dynamic_cast<BSTNode<T>*>(this->children[1])->remove(target, vis);
        return this;
    }
    
    else {
        vis.setMessage("Target " + this->toString(target) + " found!");
        vis.setColor(this, Color::MAGENTA);
        vis.render();

        if (this->children[0] == nullptr && this->children[1] == nullptr) {
            vis.setMessage("Node is a leaf. Removing.");
            vis.render();
            delete this;
            return nullptr;
        }

        else if (this->children[0] == nullptr) {
            vis.setMessage("Node has only right child. Replacing with right child.");
            vis.render();
            DataNode<T>* temp = this->children[1];
            delete this;
            return temp;
        }
        else if (this->children[1] == nullptr) {
            vis.setMessage("Node has only left child. Replacing with left child.");
            vis.render();
            DataNode<T>* temp = this->children[0];
            delete this;
            return temp;
        }

        else {
            vis.setMessage("Node has two children.\nFinding successor (min value in right subtree).");
            vis.render();

            BSTNode<T>* temp = minValueNode(dynamic_cast<BSTNode<T>*>(this->children[1]));
            
            vis.setMessage("Successor found: " + this->toString(temp->key[0]) + ".\nReplacing " + this->toString(this->key[0]) + " with " + this->toString(temp->key[0]));
            vis.setColor(this, Color::MAGENTA);
            vis.render();

            this->key[0] = temp->key[0];

            vis.setMessage("Removing duplicate successor from right subtree.");
            vis.render();
            this->children[1] = dynamic_cast<BSTNode<T>*>(this->children[1])->remove(temp->key[0], vis);
            
            vis.setColor(this, Color::RESET); 
            return this;
        }
    }
}

template <typename T>
void BSTNode<T>::rangeSearch(T begin, T end, Visualizer& vis, bool& found_any) {
    vis.setColor(this, Color::YELLOW);
    vis.setMessage("Visiting " + this->toString(this->key[0]));
    vis.render();

    T val = this->key[0];
    bool in_range = (val >= begin && val <= end);

    if (val > begin && this->children[0] != nullptr) {
        vis.setMessage("Key > Begin (" + this->toString(begin) + ")\n-> Exploring Left.");
        vis.render();
        
        dynamic_cast<BSTNode<T>*>(this->children[0])->rangeSearch(begin, end, vis, found_any);
        
        vis.setColor(this, Color::YELLOW);
        vis.setMessage("Back to " + this->toString(val));
        vis.render();
    }

    if (in_range) {
        vis.setColor(this, Color::GREEN);
        vis.setMessage(this->toString(val) + " is in range [" + this->toString(begin) + ", " + this->toString(end) + "]");
        found_any = true;
    } else {
        vis.setColor(this, Color::RESET); 
        vis.setMessage(this->toString(val) + " is out of range.");
    }
    vis.render();

    if (val < end && this->children[1] != nullptr) {
        vis.setMessage("Key < End (" + this->toString(end) + ")\n-> Exploring Right.");
        vis.render();

        dynamic_cast<BSTNode<T>*>(this->children[1])->rangeSearch(begin, end, vis, found_any);

        if (in_range) vis.setColor(this, Color::GREEN);
        else vis.setColor(this, Color::RESET);
        
        vis.setMessage("Back to " + this->toString(val));
        vis.render();
    }
}

template <typename T>
void BSTNode<T>::draw(Visualizer& vis) {
    if (this->children[1]) {
        vis.printChild(this->children[1], Pos::UP, this, Pos::UP);
    }
    vis.printKey(Pos::MID_NORM, this->toString(this->key[0]), this, 0);
    if (this->children[0]) {
        vis.printChild(this->children[0], Pos::DOWN, this, Pos::DOWN);
    }
}

template <typename T>
BSTNode<T>* BSTNode<T>::minValueNode(BSTNode<T>* node) {
    BSTNode<T>* current = node;
    while (current && current->children[0] != nullptr)
        current = dynamic_cast<BSTNode<T>*>(current->children[0]);
    return current;
}

template <typename T>
bool BST<T>::search(T target) {
    this->vis->clear();
    this->vis->setTitle("Searching for target: " + DataNode<T>::toString(target));
    return dynamic_cast<BSTNode<T>*>(this->root_ptr)->search(target, *(this->vis));
}

template <typename T>
bool BST<T>::insert(T entry) {
    this->vis->clear();
    this->vis->setTitle("Inserting entry: " + DataNode<T>::toString(entry));
    this->vis->render();

    if (this->root_ptr == nullptr) {
        this->vis->setMessage("Tree is empty. \nSetting " + DataNode<T>::toString(entry) + " as the root.");
        this->vis->render();

        this->setRoot(new BSTNode<T>{entry});

        this->vis->setColor(this->root_ptr, Color::GREEN);
        this->vis->setMessage("New root node created successfully.");
        this->vis->render();

        return true;
    }
    return dynamic_cast<BSTNode<T>*>(this->root_ptr)->insert(entry, *(this->vis));
}

template <typename T>
bool BST<T>::remove(T target) {
    this->vis->clear();
    this->vis->setTitle("Removing target: " + DataNode<T>::toString(target));
    this->vis->render();

    if (this->root_ptr == nullptr) {
        this->vis->setMessage("Tree is empty. Cannot remove.");
        this->vis->render();
        return false;
    }

    this->root_ptr = dynamic_cast<BSTNode<T>*>(this->root_ptr)->remove(target, *(this->vis));
    
    this->vis->clear();
    this->vis->setMessage("Removal operation finished.");
    this->vis->render();
    return true;
}

template <typename T>
bool BST<T>::rangeSearch(T begin, T end) {
    this->vis->clear();
    this->vis->setTitle("Range Search [" + DataNode<T>::toString(begin) + " ~ " + DataNode<T>::toString(end) + "]");
    this->vis->render();

    if (this->root_ptr == nullptr) {
        this->vis->setMessage("Tree is empty.");
        this->vis->render();
        return false;
    }

    bool found_any = false;
    
    dynamic_cast<BSTNode<T>*>(this->root_ptr)->rangeSearch(begin, end, *(this->vis), found_any);

    if (found_any) {
        this->vis->setMessage("Range search finished.\nGreen nodes are in the range.");
    } else {
        this->vis->setMessage("Range search finished.\nNo nodes found in the range.");
    }
    this->vis->render();

    return found_any;
}
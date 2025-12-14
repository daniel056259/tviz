#pragma once
#include <chrono>

class Node;

template <typename T> class DataNode;
class Visualizer;

class Tree {
public:
    virtual ~Tree() {};

    Node* root() { return root_ptr; };

protected:
    Visualizer* vis;
    Node* root_ptr = nullptr;
};

template <typename T>
class DataTree : public Tree {
public:
    DataTree();

    DataNode<T>* root();

    void setRoot(DataNode<T>* node);

    virtual bool insert(T entry) = 0;
    virtual bool search(T target) = 0;
    virtual bool remove(T target) = 0;
    virtual bool rangeSearch(T begin, T end) = 0;
};

template <typename T>
DataTree<T>::DataTree() {
    this->vis = new Visualizer{this};
}

template <typename T>
DataNode<T>* DataTree<T>::root() {
    return dynamic_cast<DataNode<T>*>(this->root_ptr);
}

template <typename T>
void DataTree<T>::setRoot(DataNode<T>* node) {
    this->root_ptr = node;
}
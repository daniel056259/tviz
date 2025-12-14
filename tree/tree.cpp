#include <iostream>
// #include "tree.hpp"
#include "node.hpp"
#include "visualizer.cpp"

using namespace std;

Tree::Tree() {
    vis = new Visualizer{this};
}

DataNode* Tree::root() {
    return root_ptr;
}

void Tree::setRoot(Node* node) {
    root_ptr = node;
}
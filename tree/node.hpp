#pragma once
#include <iostream>
#include <vector>
#include <sstream>
using namespace std;

class Visualizer;

class Node {
public:
    virtual ~Node() {};

    virtual int getKeyCount() const = 0;

    virtual void draw(Visualizer& vis) {};

    template <typename T>
    static string toString(T data) {
        stringstream ss;
        ss << data;
        return ss.str();
    }
};

template <typename T>
class DataNode : public Node {
public:
    virtual ~DataNode() {};
    bool is_leaf() { return children_count == 0; }
    int getKeyCount() const { return key_count; }

    static string toString(T data) {
        stringstream ss;
        ss << data;
        return ss.str();
    }

protected:
    vector<T> key;
    vector<DataNode *> children;
    int key_count = 0;
    int children_count = 0;

    friend class Visualizer;
};
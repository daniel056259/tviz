#include <iostream>
#include "bplustree.hpp"

int main() {
    VisualizerConfig::setDelayDuration(chrono::milliseconds(500));
    BPlusTree<int> t(2);

    for (int i = 1; i <= 12; ++i) {
        t.insert(i);
    }
    
    t.insert(0);
    t.insert(-1);
    
    t.search(6);
    t.search(12);
    t.search(99); // 없는 값

    
    t.remove(0);
    t.remove(10);

    t.rangeSearch(0, 10);

}
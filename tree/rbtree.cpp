#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include "rbtree.hpp"

using namespace std;

int main() {
    VisualizerConfig::setDelayMode(DelayMode::TIME);
    VisualizerConfig::setDelayDuration(chrono::milliseconds{500});
    // VisualizerConfig::setDelayMode(DelayMode::ON_PRESS_ENTER);
    
    
    RBTree<int> t;

    for (int i = 0; i < 6; i++)
        t.insert(i);

    t.search(0);
    t.search(5);
    t.search(99);

    t.remove(5);
    t.remove(0);
    t.remove(99);

    t.rangeSearch(2, 4);

    // RBTree<char> t;
    // for (char c = 'a'; c <= 'k'; c++)
    //     t.insert(c);

    // RBTree<string> t;
    // t.insert("cherry");
    // t.insert("apple");
    // t.insert("grape");
    // t.insert("banana");
    // t.insert("fig");
    // t.insert("elderberry");

    // t.search("cherry");     // Root (있음)
    // t.search("fig");        // 자식 노드 (있음)
    // t.search("watermelon"); // 없는 노드 (없음)

    // // 자식 0개 노드 삭제
    // t.remove("elderberry");

    // // 자식 1개 노드 삭제
    // t.remove("grape");

    // // 자식 2개 노드 삭제 (Root 삭제)
    // t.remove("cherry");

    // t.search("banana"); // 삭제 후 남아있어야 함
    // t.search("cherry"); // 삭제되었으므로 없음

    return 0;
}
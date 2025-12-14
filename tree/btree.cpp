#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include "btree.hpp"

using namespace std;

int main() {
    VisualizerConfig::setDelayDuration(chrono::milliseconds{500});

    BTree<int> t(2); 
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

    // BTree<char> t(2);
    // t.insert('e');
    // t.insert('a');
    // t.insert('c');
    // t.insert('d');
    // t.insert('k');
    // t.insert('l');

    // t.search('e');
    // t.search('k');
    // t.search('c');

    // t.remove('e');
    // t.remove('k');
    // t.remove('c');

    // BTree<string> t(2);
    // t.insert("cherry");
    // t.insert("apple");
    // t.insert("grape");
    // t.insert("banana");
    // t.insert("fig");
    // t.insert("elderberry");

    // t.search("cherry");     // Root (있음)
    // t.search("fig");        // 자식 노드 (있음)
    // t.search("watermelon"); // 없는 노드 (없음)

    // // // 자식 0개 노드 삭제
    // // t.remove("elderberry");

    // // // 자식 1개 노드 삭제
    // // t.remove("grape");

    // // // 자식 2개 노드 삭제 (Root 삭제)
    // // t.remove("cherry");

    // t.search("banana"); // 삭제 후 남아있어야 함
    // t.search("cherry"); // 삭제되었으므로 없음

    return 0;
}
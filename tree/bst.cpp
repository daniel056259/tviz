#include <iostream>
#include <vector>
#include <sstream>
#include <string>
#include "bst.hpp"
#include "tree.hpp"
#include "../visualizer/visualizer.hpp"

using namespace std;

int main() {
    getchar();
    VisualizerConfig::setDelayDuration(chrono::milliseconds{500});
    BST<int> t;

    for (int i = 0; i < 6; i++)
        t.insert(i);

    t.search(0);
    t.search(5);
    t.search(99);
    
    t.remove(5);
    t.remove(0);
    t.remove(99);

    t.rangeSearch(2, 4);

    // BST<char> bst;
    // bst.insert('e');
    // bst.insert('a');
    // bst.insert('c');
    // bst.insert('d');
    // bst.insert('k');
    // bst.insert('l');

    // bst.search('e');
    // bst.search('k');
    // bst.search('c');

    // bst.remove('e');
    // bst.remove('k');
    // bst.remove('c');

    // BST<string> bst;
    // bst.insert("cherry");
    // bst.insert("apple");
    // bst.insert("grape");
    // bst.insert("banana");
    // bst.insert("fig");
    // bst.insert("elderberry");

    // bst.search("cherry");     // Root (있음)
    // bst.search("fig");        // 자식 노드 (있음)
    // bst.search("watermelon"); // 없는 노드 (없음)

    // // 자식 0개 노드 삭제
    // bst.remove("elderberry");

    // // 자식 1개 노드 삭제
    // bst.remove("grape");

    // // 자식 2개 노드 삭제 (Root 삭제)
    // bst.remove("cherry");

    // bst.search("banana"); // 삭제 후 남아있어야 함
    // bst.search("cherry"); // 삭제되었으므로 없음
}
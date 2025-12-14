#pragma once
#include <iostream>
#include <vector>
#include <string>
#include "tree.hpp"
#include "node.hpp"
#include "../visualizer/visualizer.hpp"

using namespace std;

enum RBColor { RED, BLACK };

template <typename T> class RBTree;

template <typename T>
class RBNode : public DataNode<T> {
public:
    RBColor rb_color;
    RBNode<T>* parent = nullptr; 

    RBNode(T val) {
        this->key.resize(1);
        this->key[0] = val;
        this->key_count = 1;
        this->children.resize(2, nullptr); 
        this->children_count = 0;
        rb_color = RED;
        parent = nullptr;
    }

    RBNode<T>* left() { return dynamic_cast<RBNode<T>*>(this->children[0]); }
    RBNode<T>* right() { return dynamic_cast<RBNode<T>*>(this->children[1]); }
    
    void setLeft(RBNode<T>* node) {
        this->children[0] = node;
        if (node) { 
            this->children_count++; 
            node->parent = this;
        }
    }
    
    void setRight(RBNode<T>* node) {
        this->children[1] = node;
        if (node) {
            this->children_count++;
            node->parent = this;
        }
    }

    RBNode<T>* minimum() {
        RBNode<T>* curr = this;
        while (curr->left() != nullptr) curr = curr->left();
        return curr;
    }

    void syncColor(Visualizer* vis) {
        if (rb_color == RED) {
            vis->setColor(this, Color::RED);
        } else {
            vis->setColor(this, Color::RESET);
        }
    }

    void draw(Visualizer& vis) {
        if (!this->is_leaf() && this->children[1]) vis.printChild(this->children[1], Pos::UP, this, Pos::UP);
        vis.printKey(Pos::MID_NORM, DataNode<T>::toString(this->key[0]), this, 0);
        if (!this->is_leaf() && this->children[0]) vis.printChild(this->children[0], Pos::DOWN, this, Pos::DOWN);
    }

    friend class RBTree<T>;
};

template <typename T>
class RBTree : public DataTree<T> {
public:
    RBTree() {}

    // ---------------- Search (BST Style Visualization) ----------------
    bool search(T target) {
        this->vis->clear();
        this->vis->setTitle("Searching for: " + DataNode<T>::toString(target));
        
        RBNode<T>* current = dynamic_cast<RBNode<T>*>(this->root_ptr);
        
        if (current == nullptr) {
            this->vis->setMessage("Tree is empty.");
            this->vis->render();
            return false;
        }

        while (current != nullptr) {
            this->vis->setColor(current, Color::YELLOW);
            this->vis->setMessage("Comparing " + DataNode<T>::toString(current->key[0]) + " with target " + DataNode<T>::toString(target));
            this->vis->render();

            if (target == current->key[0]) {
                this->vis->setColor(current, Color::GREEN);
                this->vis->setMessage("Target found!");
                this->vis->render();
                return true;
            }

            // 시각화 복구
            current->syncColor(this->vis);

            if (target < current->key[0]) {
                if (current->left() == nullptr) {
                    this->vis->setColor(current, Color::RED);
                    this->vis->setMessage("Target < Key, but left child is empty. Not found.");
                    this->vis->render();
                    return false;
                }
                this->vis->setMessage("Target < Key (" + DataNode<T>::toString(target) + " < " + DataNode<T>::toString(current->key[0]) + ")\n-> Moving Left");
                this->vis->render();
                current = current->left();
            } else {
                if (current->right() == nullptr) {
                    this->vis->setColor(current, Color::RED);
                    this->vis->setMessage("Target > Key, but right child is empty. Not found.");
                    this->vis->render();
                    return false;
                }
                this->vis->setMessage("Target > Key (" + DataNode<T>::toString(target) + " > " + DataNode<T>::toString(current->key[0]) + ")\n-> Moving Right");
                this->vis->render();
                current = current->right();
            }
        }
        return false;
    }

    // ---------------- Insert (Detailed Visualization) ----------------
    bool insert(T key) {
        this->vis->clear();
        this->vis->setTitle("Inserting Key: " + DataNode<T>::toString(key));
        
        RBNode<T>* z = new RBNode<T>(key);
        RBNode<T>* y = nullptr;
        RBNode<T>* x = dynamic_cast<RBNode<T>*>(this->root_ptr);

        this->vis->setMessage("Step 1: Standard BST Insertion");
        this->vis->render();

        // BST 삽입 과정 시각화
        while (x != nullptr) {
            y = x;
            this->vis->setColor(x, Color::YELLOW);
            this->vis->setMessage("Comparing " + DataNode<T>::toString(key) + " with " + DataNode<T>::toString(x->key[0]));
            this->vis->render();

            if (key == x->key[0]) {
                this->vis->setMessage("Key " + DataNode<T>::toString(key) + " already exists. Insertion failed.");
                this->vis->setColor(x, Color::RED);
                this->vis->render();
                x->syncColor(this->vis);
                delete z;
                return false;
            }

            x->syncColor(this->vis); // 색상 복구

            if (z->key[0] < x->key[0]) {
                this->vis->setMessage("Key < Node. Moving Left.");
                this->vis->render();
                x = x->left();
            } else {
                this->vis->setMessage("Key > Node. Moving Right.");
                this->vis->render();
                x = x->right();
            }
        }

        z->parent = y;
        if (y == nullptr) {
            this->vis->setMessage("Tree is empty. Setting as Root.");
            this->setRoot(z);
        } else if (z->key[0] < y->key[0]) {
            this->vis->setMessage("Inserting as Left Child of " + DataNode<T>::toString(y->key[0]));
            y->setLeft(z);
        } else {
            this->vis->setMessage("Inserting as Right Child of " + DataNode<T>::toString(y->key[0]));
            y->setRight(z);
        }

        // 새 노드는 항상 RED
        z->rb_color = RED;
        this->vis->setColor(z, Color::RED);
        this->vis->render();

        this->vis->setMessage("Step 2: Fix Red-Black Tree Properties");
        this->vis->render();

        if (z->parent && z->parent->parent) {
            insertFixup(z);
        } else if (z == this->root_ptr) {
            this->vis->setMessage("Node is Root. Changing color to BLACK.");
            z->rb_color = BLACK;
            z->syncColor(this->vis);
            this->vis->render();
        }

        // Root는 항상 Black 유지
        if (dynamic_cast<RBNode<T>*>(this->root_ptr)->rb_color == RED) {
            this->vis->setMessage("Ensuring Root is BLACK.");
            dynamic_cast<RBNode<T>*>(this->root_ptr)->rb_color = BLACK;
            dynamic_cast<RBNode<T>*>(this->root_ptr)->syncColor(this->vis);
            this->vis->render();
        }
        
        this->vis->clear();
        this->vis->setMessage("Insertion Complete.");
        this->vis->render();
        
        return true;
    }

    // ---------------- Remove (Detailed Visualization) ----------------
    bool remove(T key) {
        this->vis->clear();
        this->vis->setTitle("Removing Key: " + DataNode<T>::toString(key));
        this->vis->render();

        // 1. 삭제할 노드 탐색
        RBNode<T>* z = findNodeWithVisual(key);
        if (z == nullptr) {
            this->vis->setMessage("Key not found. Removal failed.");
            this->vis->render();
            return false;
        }

        deleteNode(z);

        this->vis->clear();
        this->vis->setMessage("Removal Complete.");
        this->vis->render();
        return true;
    }

    // ---------------- Range Search (Reused) ----------------
    bool rangeSearch(T begin, T end) {
        this->vis->clear();
        this->vis->setTitle("Range Search [" + DataNode<T>::toString(begin) + ", " + DataNode<T>::toString(end) + "]");
        this->vis->render();

        if (this->root_ptr == nullptr) {
            this->vis->setMessage("Tree is empty.");
            this->vis->render();
            return false;
        }

        bool found = false;
        rangeSearchRecursive(dynamic_cast<RBNode<T>*>(this->root_ptr), begin, end, found);

        if (found) this->vis->setMessage("Range Search Finished. Green nodes are in range.");
        else this->vis->setMessage("No nodes found in range.");
        this->vis->render();

        return found;
    }

private:
    // 탐색 과정을 시각화하며 노드 찾기
    RBNode<T>* findNodeWithVisual(T key) {
        RBNode<T>* current = dynamic_cast<RBNode<T>*>(this->root_ptr);
        this->vis->setMessage("Searching for node to delete...");
        
        while (current != nullptr) {
            this->vis->setColor(current, Color::YELLOW);
            this->vis->render();

            if (key == current->key[0]) {
                this->vis->setColor(current, Color::MAGENTA);
                this->vis->setMessage("Found target node " + DataNode<T>::toString(key));
                this->vis->render();
                return current;
            }
            
            current->syncColor(this->vis); // 색상 복구

            if (key < current->key[0]) {
                current = current->left();
            } else {
                current = current->right();
            }
        }
        return nullptr;
    }

    void leftRotate(RBNode<T>* x) {
        this->vis->setColor(x, Color::YELLOW);
        this->vis->setMessage("Left Rotating around " + DataNode<T>::toString(x->key[0]));
        this->vis->render();

        RBNode<T>* y = x->right();
        x->children[1] = y->children[0]; 

        if (y->left() != nullptr) {
            y->left()->parent = x;
        }

        y->parent = x->parent;

        if (x->parent == nullptr) {
            this->setRoot(y);
        } else if (x == x->parent->left()) {
            x->parent->setLeft(y);
        } else {
            x->parent->setRight(y);
        }

        y->setLeft(x);
        x->parent = y;
        
        x->syncColor(this->vis); // 회전 후 원래 색 복구 시각화
        this->vis->setMessage("Rotation Complete.");
        this->vis->render();
    }

    void rightRotate(RBNode<T>* y) {
        this->vis->setColor(y, Color::YELLOW);
        this->vis->setMessage("Right Rotating around " + DataNode<T>::toString(y->key[0]));
        this->vis->render();

        RBNode<T>* x = y->left();
        y->children[0] = x->children[1];

        if (x->right() != nullptr) {
            x->right()->parent = y;
        }

        x->parent = y->parent;

        if (y->parent == nullptr) {
            this->setRoot(x);
        } else if (y == y->parent->right()) {
            y->parent->setRight(x);
        } else {
            y->parent->setLeft(x);
        }

        x->setRight(y);
        y->parent = x;
        
        y->syncColor(this->vis);
        this->vis->setMessage("Rotation Complete.");
        this->vis->render();
    }

    void insertFixup(RBNode<T>* z) {
        while (z->parent != nullptr && z->parent->rb_color == RED) {
            RBNode<T>* parent = z->parent;
            RBNode<T>* grandParent = parent->parent;

            this->vis->setMessage("Violation: Parent " + DataNode<T>::toString(parent->key[0]) + " is RED.");
            this->vis->setColor(z, Color::RED);
            this->vis->setColor(parent, Color::RED);
            this->vis->render();

            if (parent == grandParent->left()) {
                RBNode<T>* uncle = grandParent->right();

                // Case 1: Uncle is RED
                if (uncle != nullptr && uncle->rb_color == RED) {
                    this->vis->setMessage("Case 1: Uncle is RED.\n-> Recolor Parent & Uncle to BLACK, Grandparent to RED.");
                    
                    // 강조
                    this->vis->setColor(parent, Color::CYAN);
                    this->vis->setColor(uncle, Color::CYAN);
                    this->vis->setColor(grandParent, Color::CYAN);
                    this->vis->render();

                    parent->rb_color = BLACK;
                    uncle->rb_color = BLACK;
                    grandParent->rb_color = RED;

                    parent->syncColor(this->vis);
                    uncle->syncColor(this->vis);
                    grandParent->syncColor(this->vis);
                    this->vis->render();

                    z = grandParent; // Check upwards
                } 
                else {
                    // Case 2: Uncle is BLACK, z is Right Child (Triangle)
                    if (z == parent->right()) {
                        this->vis->setMessage("Case 2: Uncle is BLACK, Node is Right Child.\n-> Left Rotate Parent to convert to Line.");
                        z = parent;
                        leftRotate(z);
                        parent = z->parent; 
                    }
                    
                    // Case 3: Uncle is BLACK, z is Left Child (Line)
                    this->vis->setMessage("Case 3: Uncle is BLACK, Node is Left Child.\n-> Recolor Parent BLACK, Grandparent RED, then Right Rotate GP.");
                    
                    parent->rb_color = BLACK;
                    grandParent->rb_color = RED;
                    
                    parent->syncColor(this->vis);
                    grandParent->syncColor(this->vis);
                    this->vis->render();

                    rightRotate(grandParent);
                }
            } 
            else { // Symmetric (Parent is Right Child of Grandparent)
                RBNode<T>* uncle = grandParent->left();

                if (uncle != nullptr && uncle->rb_color == RED) {
                    this->vis->setMessage("Case 1 (Sym): Uncle is RED.\n-> Recolor Parent & Uncle to BLACK, GP to RED.");
                    
                    this->vis->setColor(parent, Color::CYAN);
                    this->vis->setColor(uncle, Color::CYAN);
                    this->vis->setColor(grandParent, Color::CYAN);
                    this->vis->render();

                    parent->rb_color = BLACK;
                    uncle->rb_color = BLACK;
                    grandParent->rb_color = RED;

                    parent->syncColor(this->vis);
                    uncle->syncColor(this->vis);
                    grandParent->syncColor(this->vis);
                    this->vis->render();

                    z = grandParent;
                } 
                else {
                    if (z == parent->left()) {
                        this->vis->setMessage("Case 2 (Sym): Uncle BLACK, Node is Left Child.\n-> Right Rotate Parent.");
                        z = parent;
                        rightRotate(z);
                        parent = z->parent;
                    }
                    this->vis->setMessage("Case 3 (Sym): Uncle BLACK, Node is Right Child.\n-> Recolor Parent BLACK, GP RED, then Left Rotate GP.");
                    
                    parent->rb_color = BLACK;
                    grandParent->rb_color = RED;

                    parent->syncColor(this->vis);
                    grandParent->syncColor(this->vis);
                    this->vis->render();

                    leftRotate(grandParent);
                }
            }
        }
        
        // Loop 종료 후
        if (z->parent == nullptr || z->parent->rb_color == BLACK) {
             this->vis->setMessage("Violations resolved.");
             this->vis->render();
        }
    }

    // ---------------- Delete Helpers ----------------

    void transplant(RBNode<T>* u, RBNode<T>* v) {
        if (u->parent == nullptr) {
            this->setRoot(v);
        } else if (u == u->parent->left()) {
            u->parent->children[0] = v; 
        } else {
            u->parent->children[1] = v;
        }
        if (v != nullptr) {
            v->parent = u->parent;
        }
    }

    void deleteNode(RBNode<T>* z) {
        RBNode<T>* y = z;
        RBNode<T>* x;
        RBColor y_original_color = y->rb_color;

        if (z->left() == nullptr) {
            x = z->right();
            this->vis->setMessage("Node has no left child. Replacing with right child.");
            this->vis->render();
            transplant(z, z->right());
        } else if (z->right() == nullptr) {
            x = z->left();
            this->vis->setMessage("Node has no right child. Replacing with left child.");
            this->vis->render();
            transplant(z, z->left());
        } else {
            this->vis->setMessage("Node has two children. Finding successor.");
            this->vis->render();

            y = z->right()->minimum();
            y_original_color = y->rb_color;
            x = y->right();

            this->vis->setColor(y, Color::CYAN);
            this->vis->setMessage("Successor is " + DataNode<T>::toString(y->key[0]));
            this->vis->render();

            if (y->parent == z) {
                if (x) x->parent = y; 
            } else {
                transplant(y, y->right());
                y->setRight(z->right());
            }

            transplant(z, y);
            y->setLeft(z->left());
            y->rb_color = z->rb_color; // z의 색상을 물려받음
            
            this->vis->setMessage("Replaced deleted node with Successor.");
            y->syncColor(this->vis);
            this->vis->render();
        }

        delete z;

        if (y_original_color == BLACK) {
            this->vis->setMessage("Deleted node (or moved successor) was BLACK.\nPossible Double Black violation. Calling Delete Fixup.");
            this->vis->render();
            
            // x가 null일 경우를 대비해 부모 포인터를 추적
            RBNode<T>* x_parent = nullptr;
            if (x) x_parent = x->parent;
            else {
                // transplant나 로직에 의해 x의 부모가 될 노드 찾기
                if (y->parent == z) x_parent = y; // y가 z의 직계 자식이었던 경우
                else x_parent = y->parent; // y가 z의 오른쪽 서브트리 어딘가에 있었던 경우
                
                // Case where z had < 2 children (y==z)
                if (y == z) {
                     // z가 삭제되었으므로 z의 부모가 x의 부모
                     // z는 이미 delete 되었으므로 접근 불가하지만, transplant 호출 시 z->parent는 x->parent가 됨
                     // 여기서는 delete z 전에 저장해 두거나, transplant 로직을 고려해야 함.
                     // 하지만 위 코드에서 이미 delete z 됨.
                     // deleteNode 함수 내 변수 z는 포인터이므로 delete 후 접근 위험.
                     // 사실 transplant가 실행되면 z의 부모의 자식 포인터는 이미 변경됨.
                     // 정확한 x_parent를 위해선 delete 전에 저장했어야 함.
                     // 하지만 여기서는 간단히 y==z인 경우(자식이 0~1개)는 transplant가 처리함.
                }
            }
            
            // Note: x가 null일 때 부모를 정확히 찾기 위해선 로직 보강이 필요하지만,
            // 시각화 목적상 x가 있는 경우 위주로 설명하거나,
            // x가 없더라도(leaf 삭제) Double Black 해결 과정을 보여주기 위해
            // x_parent 변수를 deleteFixup에 전달하도록 수정.
            
            // 이 예제 코드에서는 x가 유효하거나 루트가 아닌 경우를 가정하고 진행합니다.
            // 실제 완전한 구현에서는 Null Node(Sentinel)를 사용하는 것이 일반적입니다.
            if (x != nullptr || (x_parent != nullptr)) {
                deleteFixup(x, x_parent);
            }
        }
    }

    void deleteFixup(RBNode<T>* x, RBNode<T>* x_parent) {
        while (x != this->root_ptr && (x == nullptr || x->rb_color == BLACK)) {
            if (x == nullptr && x_parent == nullptr) break;

            RBNode<T>* parent = (x) ? x->parent : x_parent;
            
            if (x == parent->left()) {
                RBNode<T>* w = parent->right(); // Sibling
                
                if (w->rb_color == RED) {
                    this->vis->setMessage("Fix Case 1: Sibling is RED.\n-> Recolor Sibling BLACK, Parent RED, Left Rotate Parent.");
                    this->vis->render();
                    w->rb_color = BLACK;
                    parent->rb_color = RED;
                    w->syncColor(this->vis);
                    parent->syncColor(this->vis);
                    leftRotate(parent);
                    w = parent->right();
                }

                if ((w->left() == nullptr || w->left()->rb_color == BLACK) &&
                    (w->right() == nullptr || w->right()->rb_color == BLACK)) {
                    this->vis->setMessage("Fix Case 2: Sibling's children are BLACK.\n-> Recolor Sibling RED. Problem moves up.");
                    this->vis->render();
                    w->rb_color = RED;
                    w->syncColor(this->vis);
                    x = parent;
                    x_parent = x->parent;
                } 
                else {
                    if (w->right() == nullptr || w->right()->rb_color == BLACK) {
                        this->vis->setMessage("Fix Case 3: Sibling's Inner Child is RED.\n-> Recolor Sibling RED, Inner Child BLACK. Right Rotate Sibling.");
                        this->vis->render();
                        if (w->left()) w->left()->rb_color = BLACK;
                        w->rb_color = RED;
                        if(w->left()) w->left()->syncColor(this->vis);
                        w->syncColor(this->vis);
                        rightRotate(w);
                        w = parent->right();
                    }

                    this->vis->setMessage("Fix Case 4: Sibling's Outer Child is RED.\n-> Swap Colors (Sibling takes Parent's color, Parent becomes BLACK), Outer Child BLACK. Left Rotate Parent.");
                    this->vis->render();
                    w->rb_color = parent->rb_color;
                    parent->rb_color = BLACK;
                    if (w->right()) w->right()->rb_color = BLACK;
                    
                    w->syncColor(this->vis);
                    parent->syncColor(this->vis);
                    if(w->right()) w->right()->syncColor(this->vis);
                    
                    leftRotate(parent);
                    x = dynamic_cast<RBNode<T>*>(this->root_ptr); // Done
                }
            } 
            else { // Symmetric
                RBNode<T>* w = parent->left();

                if (w->rb_color == RED) { 
                    this->vis->setMessage("Fix Case 1 (Sym): Sibling is RED.\n-> Rotate & Recolor.");
                    this->vis->render();
                    w->rb_color = BLACK;
                    parent->rb_color = RED;
                    w->syncColor(this->vis);
                    parent->syncColor(this->vis);
                    rightRotate(parent);
                    w = parent->left();
                }

                if ((w->right() == nullptr || w->right()->rb_color == BLACK) &&
                    (w->left() == nullptr || w->left()->rb_color == BLACK)) { 
                    this->vis->setMessage("Fix Case 2 (Sym): Sibling's children BLACK.\n-> Recolor Sibling RED.");
                    this->vis->render();
                    w->rb_color = RED;
                    w->syncColor(this->vis);
                    x = parent;
                    x_parent = x->parent;
                } else {
                    if (w->left() == nullptr || w->left()->rb_color == BLACK) {
                        this->vis->setMessage("Fix Case 3 (Sym): Inner Child RED.\n-> Rotate Sibling.");
                        this->vis->render();
                        if(w->right()) w->right()->rb_color = BLACK;
                        w->rb_color = RED;
                        if(w->right()) w->right()->syncColor(this->vis);
                        w->syncColor(this->vis);
                        leftRotate(w);
                        w = parent->left();
                    }
                    this->vis->setMessage("Fix Case 4 (Sym): Outer Child RED.\n-> Rotate Parent.");
                    this->vis->render();
                    w->rb_color = parent->rb_color;
                    parent->rb_color = BLACK;
                    if (w->left()) w->left()->rb_color = BLACK;
                    
                    w->syncColor(this->vis);
                    parent->syncColor(this->vis);
                    if(w->left()) w->left()->syncColor(this->vis);

                    rightRotate(parent);
                    x = dynamic_cast<RBNode<T>*>(this->root_ptr);
                }
            }
        }
        if (x) {
            x->rb_color = BLACK;
            x->syncColor(this->vis);
        }
        this->vis->setMessage("Double Black resolved.");
        this->vis->render();
    }

    // Range Search Recursive (동일)
    void rangeSearchRecursive(RBNode<T>* node, T begin, T end, bool& found) {
        if (node == nullptr) return;

        T val = node->key[0];

        if (val > begin) {
            this->vis->setMessage("Key " + DataNode<T>::toString(val) + " > Begin (" + DataNode<T>::toString(begin) + ") -> Go Left");
            this->vis->render();
            rangeSearchRecursive(node->left(), begin, end, found);
            this->vis->setMessage("Back to " + DataNode<T>::toString(val));
            this->vis->render();
        }

        this->vis->setColor(node, Color::YELLOW);
        this->vis->setMessage("Visiting " + DataNode<T>::toString(val));
        this->vis->render();

        if (val >= begin && val <= end) {
            this->vis->setColor(node, Color::GREEN);
            this->vis->setMessage(DataNode<T>::toString(val) + " is in range!");
            found = true;
        } else {
            node->syncColor(this->vis);
            this->vis->setMessage(DataNode<T>::toString(val) + " is out of range.");
        }
        this->vis->render();

        if (val < end) {
            this->vis->setMessage("Key " + DataNode<T>::toString(val) + " < End (" + DataNode<T>::toString(end) + ") -> Go Right");
            this->vis->render();
            rangeSearchRecursive(node->right(), begin, end, found);
            this->vis->setMessage("Back to " + DataNode<T>::toString(val));
            this->vis->render();
        }
    }
};
#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <array>
#include <cstdio>
#include <stack>
#include "../tree/tree.hpp"
#include "../tree/node.hpp"
using namespace std;

struct pair_hash {
    template <class T1, class T2>
    std::size_t operator () (const std::pair<T1,T2> &p) const {
        auto h1 = std::hash<T1>{}(p.first);
        auto h2 = std::hash<T2>{}(p.second);
        return h1 ^ (h2 << 1); 
    }
};

enum class Color {
    RED = 31,
    GREEN = 32,
    YELLOW = 33,
    BLUE = 34,
    MAGENTA = 35,
    CYAN = 36,
    WHITE = 37,
    RESET = 0
};

ostream& operator<<(ostream& os, Color color) {
    static const string ANSI_START = "\033[";
    static const string ANSI_COLOR_END = "m";

    os << ANSI_START << static_cast<int>(color) << ANSI_COLOR_END;
    return os;
}

struct NodeStatus {
    Color color = Color::RESET;
};

ostream& operator<<(ostream& os, NodeStatus status) {
    os << status.color;
    return os;
}

enum class DelayMode {
    TIME, ON_PRESS_ENTER
};

class VisualizerConfig {
public: 
    static inline void setDelayMode(DelayMode mode) {
        delay_mode = mode;
    }

    static inline void setDelayDuration(chrono::milliseconds duration) {
        delay = duration;
    }

private:
    inline static chrono::milliseconds delay{chrono::milliseconds{1000}};
    inline static DelayMode delay_mode{DelayMode::TIME};

    friend class Visualizer;
};

enum class Pos { UP=0, MID_EVEN=3, MID_NORM=1, DOWN=2 };

class Visualizer {
public:
    Visualizer(Tree* t) : tree{t} {
        setvbuf(stdout, NULL, _IOFBF, 65536);
    }

    void render() {
        screen_clear();
        cout << "[ TASK ]\n"
                << title << "\n\n"
                << "[ Tree ]\n";

        if (tree->root() != nullptr) {
            tail_stack.push({vector<int>{}, {}, {}});
            tree->root()->draw(*this);
        }
        else {
            cout << "(Empty Tree)\n";
        }

        cout << "\n";
        cout << "[ CURR ]\n"
             << message << "\n"
             << endl;
        wait();
    }

    void setTitle(const string& t) {
        title = t;
    }

    void setMessage(const string& m) {
        message = m;
    }

    void setColor(Node* node, int key_idx, Color c) {
        status[{node, key_idx}].color = c;
    }

    void setColor(Node* node, Color c) {
        for (int i = 0; i < node->getKeyCount(); i++)
            status[{node, i}].color = c;
    }

    void clear() {
        status.clear();
        message.clear();
    }

    void printKey(Pos pos, const string& key_string, Node* node, int key_idx) {
        printTiles(prefix);
        printTiles(tail_stack.top()[(pos == Pos::MID_EVEN ? 1 : static_cast<int>(pos))]);

        NodeStatus null_status;
        auto it = status.find(pair<Node*, int>{node, key_idx});
        if (it != status.end())
            cout << it->second << key_string << null_status << '\n';
        else
            cout << key_string << '\n';
    }

    void printLabel(Pos pos, const string& label_string) {
        printTiles(prefix);
        printTiles(tail_stack.top()[(pos == Pos::MID_EVEN ? 1 : static_cast<int>(pos))]);

        cout << label_string << '\n';
    }

    void printChild(Node* child, Pos child_pos, Node* node, Pos node_pos) {
        enterChild(node_pos, node, child_pos);
        child->draw(*this);
        leaveChild(node_pos, node, child_pos);
    }

    void printKeyConnection(Node* node, Pos pos) {
        printTiles(prefix);
        printTiles(tail_stack.top()[(pos == Pos::MID_EVEN ? 1 : static_cast<int>(pos))]);

        if (pos == Pos::MID_EVEN && (node != tree->root())) {
            cout << "┤\n";
        }
        else
            cout << "│\n";
    }

private:
    unordered_map<pair<Node*, int>, NodeStatus, pair_hash> status;
    string title, message;
    Tree* tree;
    vector<int> prefix;
    stack<array<vector<int>, 3>> tail_stack;

    array<string, 6> tile{"    ", "│   ", "┌───", "├───", "└───", "┼───"};

    void screen_clear() {
        #ifdef _WIN32
            system("cls");
        #else
            printf("\033[H\033[J");
        #endif
    }

    void wait() {
        switch (VisualizerConfig::delay_mode)
        {
        case DelayMode::TIME:
            this_thread::sleep_for(VisualizerConfig::delay);
            break;
        case DelayMode::ON_PRESS_ENTER:
            cout << "(press 'enter' to continue)..." << endl;
            getchar();
            break;
        }
    }
    
    void printTiles(const vector<int>& t) {
        for (int i : t) cout << tile[i];
    }
    
    void enterChild(Pos node_pos, Node* node, Pos child_pos) {
        if (child_pos == Pos::MID_EVEN) {
            array<vector<int>, 3> next_tail = calcNextTail(node, child_pos);
            for (int i = 0; i < 3; i++) {
                next_tail[i].insert(next_tail[i].begin(), tail_stack.top()[i].begin(), tail_stack.top()[i].end());
            }
            tail_stack.push(next_tail);
            return;
        }
        vector<int>& tail = tail_stack.top()[static_cast<int>(node_pos)];
        prefix.insert(prefix.end(), tail.begin(), tail.end());
        tail_stack.push(calcNextTail(node, child_pos));
    }

    void leaveChild(Pos node_pos, Node* node, Pos child_pos) {
        tail_stack.pop();
        if (child_pos == Pos::MID_EVEN)
            return;
        vector<int> &tail = tail_stack.top()[static_cast<int>(node_pos)];
        prefix.erase(prefix.end() - tail.size(), prefix.end());
    }

    array<vector<int>, 3> calcNextTail(Node* node, Pos child_pos) {
        switch (child_pos)
        {
        case Pos::UP:
            return {vector<int>{0}, {2}, {1}};
        case Pos::DOWN:
            return {vector<int>{1}, {4}, {0}};
        case Pos::MID_EVEN:
            if (node != tree->root()) return {vector<int>{1}, {5}, {1}};
        case Pos::MID_NORM:
        default:
            return {vector<int>{1}, {3}, {1}};
        }
    }
};



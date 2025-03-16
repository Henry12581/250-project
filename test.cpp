#include <iostream>
#include <vector>
#include <algorithm>
#include <map>
#include <cmath>
#include <sstream>
#include <string>

static const int M = 8;
static const int MAX_ID = 1 << M;

class FingerTable;

class Node {
public:
    explicit Node(int node_id);
    ~Node();

    void update_finger_table();
    Node* get_successor();
    Node* closest_preceding_finger(int key);

    std::pair<Node*, std::vector<int>> find_key(int key);
    void insert_key(int key, int value = -1);
    void remove_key(int key);

    void join(Node* contact);
    void leave();

    void print_finger_table();

    int id;
    FingerTable* finger;
    std::map<int,int> keys;
};

static std::vector<Node*> DHT_NODES;


bool in_interval(int x, int a, int b, bool inclusive=false);
void update_all_finger_tables();
Node* get_successor_for(int key);
Node* get_next_node(Node* node);
Node* get_predecessor(Node* node);


class FingerTable {
public:
    explicit FingerTable(Node* node) : node(node) {
        entries.resize(M, nullptr);
    }

    void update();
    void pretty_print();

    std::vector<Node*> entries;
    Node* node;
};

Node::Node(int node_id)
    : id(node_id), finger(new FingerTable(this)) {}

Node::~Node() {
    if (finger) {
        delete finger;
    }
}

void Node::update_finger_table() {
    finger->update();
}

Node* Node::get_successor() {
    return finger->entries[0];
}

Node* Node::closest_preceding_finger(int key) {
    for (int i = M - 1; i >= 0; --i) {
        Node* candidate = finger->entries[i];
        if (candidate &&
            candidate != this &&
            in_interval(candidate->id, this->id, key, false)) {
            return candidate;
        }
    }
    return this;
}

std::pair<Node*, std::vector<int>> Node::find_key(int key) {
    std::vector<int> path;
    path.push_back(this->id);

    Node* current = this;
    while (true) {
        Node* succ = current->get_successor();
        if (in_interval(key, current->id, succ->id, true)) {
            path.push_back(succ->id);
            return {succ, path};
        } else {
            Node* next_node = current->closest_preceding_finger(key);
            if (next_node == current) {
                current = current->get_successor();
                path.push_back(current->id);
                return {current, path};
            } else {
                current = next_node;
                path.push_back(current->id);
            }
        }
    }
}

void Node::insert_key(int key, int value) {
    auto result = find_key(key);
    Node* responsible = result.first;
    responsible->keys[key] = value;
}

void Node::remove_key(int key) {
    auto result = find_key(key);
    Node* responsible = result.first;
    if (responsible->keys.find(key) != responsible->keys.end()) {
        responsible->keys.erase(key);
    }
}

void Node::join(Node* contact) {
    if (!contact) {
        DHT_NODES.push_back(this);
        update_all_finger_tables();
    } else {
        DHT_NODES.push_back(this);
        update_all_finger_tables();
        Node* pred = get_predecessor(this);
        Node* succ = get_next_node(this);

        std::vector<int> migrated;
        std::vector<int> succKeys;
        for (auto& kv : succ->keys) {
            succKeys.push_back(kv.first);
        }
        for (int k : succKeys) {
            if (in_interval(k, pred->id, this->id, true)) {
                this->keys[k] = succ->keys[k];
                migrated.push_back(k);
                succ->keys.erase(k);
            }
        }
        if (!migrated.empty()) {
            std::sort(migrated.begin(), migrated.end());
            std::cout << "Migrated keys from node "
                      << succ->id << " to node " << this->id << ": ";
            for (size_t i = 0; i < migrated.size(); i++) {
                std::cout << migrated[i];
                if (i + 1 < migrated.size()) {
                    std::cout << " ";
                }
            }
            std::cout << std::endl;
        }
    }
}

void Node::leave() {
    Node* succ = get_next_node(this);
    for (auto& kv : this->keys) {
        succ->keys[kv.first] = kv.second;
    }
    auto it = std::find(DHT_NODES.begin(), DHT_NODES.end(), this);
    if (it != DHT_NODES.end()) {
        DHT_NODES.erase(it);
    }
    update_all_finger_tables();
}

void Node::print_finger_table() {
    finger->pretty_print();
}

void FingerTable::update() {
    for (int i = 0; i < M; i++) {
        int start = (node->id + (1 << i)) % MAX_ID;
        entries[i] = get_successor_for(start);
    }
}

void FingerTable::pretty_print() {
    std::cout << "Finger table of node " << node->id << ":" << std::endl;
    for (int i = 0; i < M; i++) {
        int start = (node->id + (1 << i)) % MAX_ID;
        std::cout << "start " << start << " -> " << entries[i]->id << std::endl;
    }
}

bool in_interval(int x, int a, int b, bool inclusive) {
    if (a < b) {
        return inclusive ? (a < x && x <= b) : (a < x && x < b);
    } else if (a > b) {
        return inclusive ? (x > a || x <= b) : (x > a || x < b);
    } else {
        return true;
    }
}

void update_all_finger_tables() {
    for (Node* node : DHT_NODES) {
        node->update_finger_table();
    }
}

Node* get_successor_for(int key) {
    std::vector<Node*> sorted_nodes(DHT_NODES.begin(), DHT_NODES.end());
    std::sort(sorted_nodes.begin(), sorted_nodes.end(),
              [](Node* a, Node* b){ return a->id < b->id; });

    for (Node* node : sorted_nodes) {
        if (node->id >= key) {
            return node;
        }
    }
    return sorted_nodes[0];
}

Node* get_next_node(Node* node) {
    std::vector<Node*> sorted_nodes(DHT_NODES.begin(), DHT_NODES.end());
    std::sort(sorted_nodes.begin(), sorted_nodes.end(),
              [](Node* a, Node* b){ return a->id < b->id; });

    auto it = std::find(sorted_nodes.begin(), sorted_nodes.end(), node);
    if (it == sorted_nodes.end()) {
        return nullptr;
    }

    size_t idx = std::distance(sorted_nodes.begin(), it);
    return sorted_nodes[(idx + 1) % sorted_nodes.size()];
}

Node* get_predecessor(Node* node) {
    std::vector<Node*> sorted_nodes(DHT_NODES.begin(), DHT_NODES.end());
    std::sort(sorted_nodes.begin(), sorted_nodes.end(),
              [](Node* a, Node* b){ return a->id < b->id; });

    auto it = std::find(sorted_nodes.begin(), sorted_nodes.end(), node);
    if (it == sorted_nodes.end()) {
        return nullptr;
    }

    size_t idx = std::distance(sorted_nodes.begin(), it);
    if (idx == 0) {
        return sorted_nodes[sorted_nodes.size() - 1];
    } else {
        return sorted_nodes[idx - 1];
    }
}

int main() {
    DHT_NODES.clear();

    Node* n0 = new Node(0);
    Node* n1 = new Node(30);
    Node* n2 = new Node(65);
    Node* n3 = new Node(110);
    Node* n4 = new Node(160);
    Node* n5 = new Node(230);

    n0->join(nullptr);
    n1->join(n0);
    n2->join(n1);
    n3->join(n2);
    n4->join(n3);
    n5->join(n4);

    std::cout << "Finger Tables:" << std::endl;
    {
        std::vector<Node*> sorted_nodes(DHT_NODES.begin(), DHT_NODES.end());
        std::sort(sorted_nodes.begin(), sorted_nodes.end(),
                  [](Node* a, Node* b){ return a->id < b->id; });
        for (Node* node : sorted_nodes) {
            node->print_finger_table();
            std::cout << std::endl;
        }
    }

    n0->insert_key(3, 3);
    n1->insert_key(200);
    n2->insert_key(123);
    n3->insert_key(45, 3);
    n4->insert_key(99);
    n2->insert_key(60, 10);
    n0->insert_key(50, 8);
    n3->insert_key(100, 5);
    n3->insert_key(101, 4);
    n3->insert_key(102, 6);
    n5->insert_key(240, 8);
    n5->insert_key(250, 10);

    std::cout << "\nKeys Distribution:" << std::endl;
    {
        std::vector<Node*> sorted_nodes(DHT_NODES.begin(), DHT_NODES.end());
        std::sort(sorted_nodes.begin(), sorted_nodes.end(),
                  [](Node* a, Node* b){ return a->id < b->id; });
        for (Node* node : sorted_nodes) {
            std::stringstream ss;
            for (auto& kv : node->keys) {
                ss << kv.first << ":" << kv.second << " ";
            }
            std::string keys_str = ss.str();
            if (!keys_str.empty() && keys_str.back() == ' ') {
                keys_str.pop_back();
            }
            std::cout << "Node " << node->id << ": " << keys_str << std::endl;
        }
        std::cout << std::endl;
    }

    Node* n6 = new Node(100);
    n6->join(n0);

    std::cout << "\nKeys Distribution after node 100 joins:" << std::endl;
    {
        std::vector<Node*> sorted_nodes(DHT_NODES.begin(), DHT_NODES.end());
        std::sort(sorted_nodes.begin(), sorted_nodes.end(),
                  [](Node* a, Node* b){ return a->id < b->id; });
        for (Node* node : sorted_nodes) {
            std::stringstream ss;
            for (auto& kv : node->keys) {
                ss << kv.first << ":" << kv.second << " ";
            }
            std::string keys_str = ss.str();
            if (!keys_str.empty() && keys_str.back() == ' ') {
                keys_str.pop_back();
            }
            std::cout << "Node " << node->id << ": " << keys_str << std::endl;
        }
        std::cout << std::endl;
    }

    std::vector<int> lookup_keys{3, 200, 123, 45, 99, 60, 50,
                                 100, 101, 102, 240, 250};
    std::vector<Node*> start_nodes{n0, n2, n6};

    for (Node* start_node : start_nodes) {
        std::cout << "\n----- node " << start_node->id << " lookups -----" << std::endl;
        for (int key : lookup_keys) {
            auto result = start_node->find_key(key);
            Node* responsible_node = result.first;
            std::vector<int> path = result.second;

            int value = -1; // default if not found
            auto it = responsible_node->keys.find(key);
            if (it != responsible_node->keys.end()) {
                value = it->second;
            }
            std::cout << "Look-up result of key " << key
                      << " from node " << start_node->id
                      << " with path [";
            for (size_t i = 0; i < path.size(); i++) {
                std::cout << path[i];
                if (i + 1 < path.size()) {
                    std::cout << ",";
                }
            }
            std::cout << "] value is " << value << std::endl;
        }
    }
    std::cout << std::endl;

    n2->leave();

    std::cout << "Updated Finger Tables after node 65 leaves:" << std::endl;
    {
        std::vector<Node*> sorted_nodes(DHT_NODES.begin(), DHT_NODES.end());
        std::sort(sorted_nodes.begin(), sorted_nodes.end(),
                  [](Node* a, Node* b){ return a->id < b->id; });
        for (Node* node : sorted_nodes) {
            if (node->id == 0 || node->id == 30) {
                node->print_finger_table();
                std::cout << std::endl;
            }
        }
    }

    std::cout << "Keys Distribution after node 65 leaves:" << std::endl;
    {
        std::vector<Node*> sorted_nodes(DHT_NODES.begin(), DHT_NODES.end());
        std::sort(sorted_nodes.begin(), sorted_nodes.end(),
                  [](Node* a, Node* b){ return a->id < b->id; });
        for (Node* node : sorted_nodes) {
            std::stringstream ss;
            for (auto& kv : node->keys) {
                ss << kv.first << ":" << kv.second << " ";
            }
            std::string keys_str = ss.str();
            if (!keys_str.empty() && keys_str.back() == ' ') {
                keys_str.pop_back();
            }
            std::cout << "Node " << node->id << ": " << keys_str << std::endl;
        }
    }
    return 0;
}



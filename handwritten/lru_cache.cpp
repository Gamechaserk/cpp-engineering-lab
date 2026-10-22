#include <iostream>
#include <unordered_map>
#include <mutex>


//缺锁
struct Node {
    Node* prev = nullptr;
    Node* next = nullptr;
    int m_key = 0;
    int m_value = 0;

    Node(int key=0,int value=0):m_key(key),m_value(value){}
};

class LRUCache {
public:
    LRUCache(const LRUCache&) = delete; 
    LRUCache& operator=(const LRUCache&) = delete;
    ~LRUCache(){}//析构待完善
    explicit LRUCache(int capacity):m_capacity(capacity){
        guard = new Node();
        guard->next = guard;
        guard->prev = guard;
    }
    int  get(int key) {
        //std::lock_guard<std::mutex> lock(m_mutex);
        Node* node = getnode(key);
        if (node) {
            return node->m_value;
        }
        return -1;
    }// 未命中返回 -1
    void put(int key, int value) {
        //std::lock_guard<std::mutex> lock(m_mutex);
        Node* node = getnode(key);
        if (node) {
            node->m_value = value;
        }
        else {
            if (hashmap.size() >= m_capacity) {
                Node* back_node = guard->prev;
                remove(back_node);
                hashmap.erase(back_node->m_key);
                delete back_node;
            }
            Node* newnode = new Node(key, value);
            hashmap[key] = newnode;
            push_front(newnode);
        }
    }

    //// 4) 校验函数也必须是线程安全的 ⇒ 也要加锁
    //bool validate() {                 // ← 不是 const：加锁 + 读结构
    //    std::lock_guard<std::mutex> lock(m_mutex);
    //}

private:
    Node* getnode(int key){
        auto it = hashmap.find(key);
        if (it != hashmap.end()) {
            remove(it->second);
            push_front(it->second);
            return it->second;
        }
        return nullptr;
    }
    void remove(Node* node) {
        if (m_capacity == 0) return;
        node->prev->next = node->next;
        node->next->prev = node->prev;
        
    }
    void push_front(Node* node) {
        node->prev = guard;
        node->next = guard->next;
        node->next->prev = node;
        node->prev->next = node;
    }

    int m_capacity = 0;
    Node* guard = nullptr;
    std::unordered_map<int, Node*> hashmap;
    std::mutex m_mutex;
};

int main() {

    /*int failures = 0;
    auto check = [&failures](const char* name, int got, int want) {
        const bool ok = (got == want);
        if (!ok) { ++failures; }
        std::cout << (ok ? "OK  " : "FAIL") << " | " << name
            << " | got=" << got << " want=" << want << "\n";
        };*/
    // 用法：check("get(1) 命中", lru.get(1), 1);

    LRUCache lru(2);
    lru.put(1, 1);
    lru.put(2, 2);
    std::cout<<"lru.get(1)=" << lru.get(1) << ",预期：1" << std::endl;
    lru.put(3, 3);
    std::cout << "lru.get(2)=" << lru.get(2) << ",预期：-1" << std::endl;
    lru.put(4, 4);
    std::cout << "lru.get(1)=" << lru.get(1) << ",预期：-1" << std::endl;
    std::cout << "lru.get(3)=" << lru.get(3) << ",预期：3" << std::endl;
    std::cout << "lru.get(4)=" << lru.get(4) << ",预期：4" << std::endl;
    return 0;
}
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
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
    ~LRUCache(){
        for (Node* cur = guard->next; cur != guard;) {
            Node* ptr = cur->next;
            delete cur;
            cur = ptr;
        }
        delete guard;
    }
    explicit LRUCache(int capacity):m_capacity(capacity){
        guard = new Node();
        guard->next = guard;
        guard->prev = guard;
    }
    int  get(int key) {
        //std::lock_guard<std::mutex> lock(m_mutex);
        Node* node = findAndPromote(key);
        if (node) {
            return node->m_value;
        }
        return -1;
    }// 未命中返回 -1
    void put(int key, int value) {
        //std::lock_guard<std::mutex> lock(m_mutex);
        Node* node = findAndPromote(key);
        if (node) {
            node->m_value = value;
        }
        else {
            if (hashmap.size() >= m_capacity) {
                if (m_capacity == 0) return;
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
    Node* findAndPromote(int key){
        auto it = hashmap.find(key);
        if (it != hashmap.end()) {
            remove(it->second);
            push_front(it->second);
            return it->second;
        }
        return nullptr;
    }
    void remove(Node* node) {
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
    // ── 自测用例（2026-10-23 起测试由教练产出；本文件是手撕练习件，用例内嵌在 main）──

    int failures = 0;
    auto check = [&failures](const char* name, int got, int want) {
        const bool ok = (got == want);
        if (!ok) { ++failures; }
        std::cout << (ok ? "OK  " : "FAIL") << " | " << name
            << " | got=" << got << " want=" << want << "\n";
        };
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    // ── 用例 1：容量 2 的经典淘汰序列 ──
    {
        LRUCache lru(2);
        lru.put(1, 1);
        lru.put(2, 2);
        check("get(1) 命中", lru.get(1), 1);                        // 1 被提升为最近使用
        lru.put(3, 3);                                             // ⇒ 淘汰最久未用的 2
        check("get(2) 未命中（已被 3 淘汰）", lru.get(2), -1);
        lru.put(4, 4);                                             // 此时 LRU 是 1 ⇒ 淘汰 1
        check("get(1) 未命中（已被 4 淘汰）", lru.get(1), -1);
        check("get(3) 命中", lru.get(3), 3);
        check("get(4) 命中", lru.get(4), 4);
    }
    // ── 用例 2：容量 0 —— put 必须【静默忽略】，且绝不能走到 delete guard（否则 UAF）──
    {
        LRUCache lru(0);
        lru.put(1, 1);
        // ★ 教训：字符串字面量里只能放【执行字符集(936)】可表示的字符。
        //   `⇒`(U+21D2) 不在 GBK 里 ⇒ warning C4566，且运行时被降级成 `?`。
        //   注释里随便写 ⇒ ★ → ≈ 都没事（注释在预处理前就被剥掉了），字符串里不行。
        check("容量 0：put 后仍为空 => get(1) 未命中", lru.get(1), -1);
    }
    // ── 用例 3：容量 1 —— 每次 put 新 key 都必须挤掉上一个 ──
    {
        LRUCache lru(1);
        lru.put(1, 1);
        check("容量 1：get(1) 命中", lru.get(1), 1);
        lru.put(2, 2);
        check("容量 1：get(1) 未命中（已被 2 挤掉）", lru.get(1), -1);
        check("容量 1：get(2) 命中", lru.get(2), 2);
    }
    // ── 用例 4：重复 put 同一 key ⇒ 只更新值，不新建节点、不触发淘汰 ──
    {
        LRUCache lru(1);
        lru.put(1, 1);
        lru.put(1, 2);                                             // 同一个 key：只改值
        check("重复 put：get(1) 应是最新值 2（不是先写的 1）", lru.get(1), 2);
        lru.put(1, 3);
        check("重复 put 后 key 1 仍在：get(1) == 3", lru.get(1), 3);
        check("重复 put 不得把 key 挤成别的：get(2) 未命中", lru.get(2), -1);
    }
    return (failures == 0) ? 0 : 1;
}
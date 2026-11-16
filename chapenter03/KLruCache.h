#pragma once

#include <unordered_map>
#include <cassert>
#include <cstddef>
#include <memory>
#include <mutex>

#include "KICachePolicy.h"

namespace KamaCache
{
    
template<typename Key,typename Value>
class KLruCache : public KICachePolicy<Key, Value>//模板类参数也要继承
{
private:
    //单个节点
    struct Node
    {
        /* data */
        Key key;
        Value value;
        std::weak_ptr<Node> pre;
        std::shared_ptr<Node> next;
        Node(const Key& key,const Value& value)
            :Key(key),Value(value)
        {}
    };

    using NodePtr = std::shared_ptr<Node>;
    using NodeMap = std::unordered_map<Key,NodePtr>;

public:

    explicit KLruCache(int capacity) : capacity_(capacity)
    {
        init();
    }

    ~KLruCache() override = default;

    KLruCache(const KLruCache&)= delete;
    KLruCache& operator=(const KLruCache&) = delete;

    void put(Key key,Value value) override
    {
        if(capacity_<=0)
            return;
        
        std::lock_guard<std::mutex> guard(mutex_);//上互斥锁

        auto it = nodeMap_.find(key);
        if(it != nodeMap_.end())
        {
            it->second->value = value;
            moveMostRecent(it->second);
            return;
        }

        if (nodeMap_.size() >= static_cast<std::size_t>(capacity_))
        {
            evictLeastRecent();
        }

        auto node = std::make_shared<Node>(key,value);//创建新节点
        nodeMap_.emplace(key,node);
        insertNode(node);
    }
    //通过引用value来返回值
    bool get(Key key,Value& value) override
    {
        std::lock_guard<std::mutex> guard(mutex_);

        auto it = nodeMap_.find(key);
        if(it == nodeMap_.end())
        {
            return false;
        }

        value = it->second->value;
        moveMostRecent(it->second);
        return true;
    } 
    //直接返回值
    Value get(Key key) override
    {
        Value value{};
        get(key, value);
        return value;
    }

    void remove(Key key)
    {
        std::lock_guard<std::mutex> guard(mutex_);
        auto it = nodeMap_.find(key);
        if(it == nodeMap_.end())
        {
            return;
        }

        removeNode(it->second);
        nodeMap_.erase(key);
    }

    std::size_t size() const
    {
        std::lock_guard<std::mutex> guard(mutex_);
        return nodeMap_.size();
    }

private:

    void init()
    {
        head = std::make_shared<Node>(Key{},Value{});
        tail = std::make_shared<Node>(Key{},Value{});
        head->next = tail;
        tail->pre = head;
    }

    void removeNode(NodePtr node)
    {
        auto previous = node->pre.lock();
        auto next = node->next;
        assert(previous && next);

        previous->next = next;
        next->pre = previous;

        node->pre.reset();
        node->next.reset();
    }

    void insertNode(NodePtr node)
    {
        auto previous = tail->pre.lock();
        assert(previous);

        node->pre = previous;
        node->next = tail;

        previous->next = node;
        tail->pre = node;
    }
    void moveMostRecent(NodePtr node)
    {
        removeNode(node);
        insertNode(node);
    }

    void evictLeastRecent()
    {
        auto oldest = head->next;
        assert(oldest!=tail);

        removeNode(oldest);
        nodeMap_.erase(oldest->key);
    }
private:
    const int capacity_;
    NodeMap nodeMap_;
    mutable std::mutex mutex_;
    NodePtr head;
    NodePtr tail;
};
} // namespace KamaCache



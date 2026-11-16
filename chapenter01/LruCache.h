#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <assert.h>


class LruCache
{
private:
    struct Node
    {/* data */
        int key;
        std::string value;
        std::weak_ptr<Node> prev;
        std::shared_ptr<Node> next;

        Node(int key,std::string value):key(key),value(value)
        {}
        
    };
    using NodePtr = std::shared_ptr<Node>;
    using NodeMap = std::unordered_map<int,NodePtr>;

    void removeNode(NodePtr node)
    {
        auto pre = node->prev.lock();
        auto next = node->next;
        assert(pre && next);

        pre->next = next;
        next->prev = pre;

        node->prev.reset();
        node->next.reset();
    }

    void insertNode(NodePtr node)//node指向要插在末尾的节点
    {
        auto previous = dummyTail_->prev.lock();
        assert(previous);

        node->prev = previous;
        node->next=dummyTail_;
        previous->next = node;
        dummyTail_->prev = node;

    }

    void moveToMostRecent(NodePtr node)
    {
        removeNode(node);
        insertNode(node);
    }

    void evictLeastRecent()
    {
        auto oldest = dummyHead_->next;
        assert(oldest != dummyTail_);

        removeNode(oldest);
        nodeMap_.erase(oldest->key);
    }
public:

    void init()
    {
        dummyHead_ = std::make_shared<Node>(0,"");
        dummyTail_ = std::make_shared<Node>(0,"");
        dummyHead_->next = dummyTail_;
        dummyTail_->prev = dummyHead_;
    }

    explicit LruCache(int capacity):capacity_(capacity)
    {
        init();
    }
    //删除传实体和等号构造方法
    LruCache(const LruCache&) = delete;
    LruCache& operator=(const LruCache&) = delete;


    void put(int key,std::string value)
    {
        if(capacity_<=0)
        {
            return;
        }

        auto it = nodeMap_.find(key);
        if(it != nodeMap_.end())//在哈希表里面找到了.
        {
            it->second->value = value;
            moveToMostRecent(it->second);
            return;
        }
        if(nodeMap_.size() >= static_cast<std::size_t>(capacity_))//map装的数据量等于设定的缓存区容量了
        {
            evictLeastRecent();//删除最老的
        }
        auto node = std::make_shared<Node>(key,value);//put传入的值新建节点
        nodeMap_.emplace(key,node);//节点加入map替换原来的
        insertNode(node);//节点加入链表末尾
    }

    bool get(int key,std::string& value)
    {
        auto it = nodeMap_.find(key);
        if(it == nodeMap_.end())
        {
            return false;
        }

        value = it->second->value;
        moveToMostRecent(it->second);
        return true;
    }

    void remove(int key)
    {
        auto it = nodeMap_.find(key);
        if( it == nodeMap_.end())
        {
            return;
        }
        removeNode(it->second);
        nodeMap_.erase(it);
    }

    std::size_t size() const
    {
        return nodeMap_.size();
    }
private:

    
    int capacity_;
    NodeMap nodeMap_;
    NodePtr dummyHead_;
    NodePtr dummyTail_;
};


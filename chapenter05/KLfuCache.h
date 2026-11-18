#pragma once
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <limits>
#include <list>
#include <map>
#include <mutex>
#include <unordered_map>
#include "KICachePolicy.h"

namespace KamaCache
{
    
template<typename Key, typename Value>
class KLfuCache : public KICachePolicy
{
private:
    using Count = std::size_t;
    using KeyList = std::list<key>;
    using Position = typename KeyList::iterator;

    struct Entry
    {
        /* 一条缓存记录 */
        Value value;
        Count frequency;//在哪个频次桶里面
        Position bucketPosition;//桶里面的位置
        Position recentPosition;//全局位置
    };

private:
    //添加记录 传入key value就创建
    void addEntry(const Key& key, const Value& value)
    {
        //加入全局记录 然后返回得到迭代器
        auto it = entries_.emplace(key,Entry{value,1,{},{}}).first;
        try{ recency_.push_back(key);}//新建节点加入使用顺序列表
        catch(...){entries_.erase(it);throw;}

        try
        {
            auto& bucket = buckets_[1];//链表引用
            bucket.push_back(key);
            it->second.bucketPosition = std::prev(bucket.end());
        }
        catch(...)
        {
            recency_.pop_back();
            entries_.erase(it);
            auto bucket = buckets_.find(1);
            if (bucket != buckets_.end() && bucket->second.empty()) 
                buckets_.erase(bucket);
            throw;
        }
        
        it->second.recentPosition = std::prev(recency_.end());

    }



private:
    const int capacity_;//最多保存多少条
    const std::size_t ageEvery_;//每多少次成功操作老化一次
    std::size_t accesses_ = 0;//上次老化已经过了多少次成功操作
    std::unordered_map<Key,Entry> entries_;//按键查记录 无序查找快
    std::map<Count,KeyList> buckets_;//按频次找最小桶  有序map,每个频次都是一个list
    KeyList recency_;//所有键的全局使用顺序   list
    mutable std::mutex mutex_;
};


} // namespace KamaCache

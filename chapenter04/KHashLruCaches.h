#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

#include "KLruCache.h"

namespace KamaCache
{
template<typename Key,typename Value,typename Hasher = std::hash<Key>>
class KHashLruCaches : public KICachePolicy<Key,Value>
{
public:
    KHashLruCaches(int capacity,int sliceNum = 0)//总容量 每片容量
    {
        
        const std::size_t total = capacity > 0 ?
            static_cast<std::size_t>(capacity) : 0;//总容量
        std::size_t requested = sliceNum > 0 ?//片数
            static_cast<std::size_t>(sliceNum) :
            static_cast<std::size_t>(std::thread::hardware_concurrency());
                                                //返回建议并发数
        if(requested == 0)//没获取到 或者为0 就默认分成1片
        {
            requested = 1;
        }
        //片数不能超过总容量
        const std::size_t count = total == 0 ? 1 : std::min(total,requested);

        const std::size_t base = total / count;//每片容量
        const std::size_t remainder = total % count;//剩余不够的容量

        slices_.reserve(count);
        capacities_.reserve(count);
        for(std::size_t i = 0;i<count;i++)
        {   
            //让前 remainder 个分片各多分 1 条,后面的不多分
            const int localCapacity = static_cast<int>(base + (i<remainder?1:0));
            slices_.push_back(std::make_unique<KLruCache<Key,Value>(localCapacity)>);
            capacities_.push_back(localCapacity);
        }

    }
    ~KHashLruCaches() override = default;
    KHashLruCaches(const KHashLruCaches&) = delete;
    KHashLruCaches& operator=(const KHashLruCaches&) = delete;

    void put(Key key, Value value) override
    {
        slices_[indexFor(key)]->put(key,value);
    }

    bool get(Key key, Value& value) override
    {
        return slices_[indexFor(key)]->get(key,value);
    }

    Value get(Key key) override
    {
        Value value{};
        get(key,value);
        return value;
    }

    void remove(Key key)
    {
        slices_[indexFor(key)]->remove(key);
    }

    std::size_t sliceCount() const
    {
        return slices_.size();
    }

    int sliceCapacity(std::size_t index) const
    {
        return capacities_.at(index);
    }


private:
    std::size_t indexFor(const Key& key) const
    {
        return hash_(key) % slices_.size();//返回分配到的索引
    }

private:
    std::vector<std::unique_ptr<KLruCache<Key,Value>>> slices_;
    //一个数组 每个都是指向基础lru的指针 是分的片
    std::vector<int> capacities_;
    //每一片的容量
    Hasher hash_{};
    //计算下标使用
};

} // namespace KamaCache

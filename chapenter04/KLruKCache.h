#pragma once

#include <cstddef>
#include <mutex>
#include <optional>
#include <stdexcept>

#include "KLruCache.h"

namespace KamaCache
{
template<typename Key,typename Value>
class KLruKCache : public KICachePolicy<Key,Value>
{
private:
    struct HistoryRecord
    {
        std::size_t count = 0;
        std::optional<Value> value;
    };
    
public:

/*编译器看到 main_(capacity)，做三件事：
找到 main_ 的类型：KLruCache<Key, Value>
找这个类型有没有能接受 int 的构造函数 → 有，KLruCache(int capacity)
生成调用：main_ 用 capacity 构造，等价于 KLruCache<Key, Value> main_(capacity);*/
    KLruKCache(int capacity,int historyCapacity,int k)
        : capacity_(capacity)
        , k_(k)
        , main_(capacity)//用int调用构造函数构造
        , history_(historyCapacity)
    {
        if(k<=0 || historyCapacity <= 0)
        {
            throw std::invalid_argument("k and historyCapacity must be positive");
        }
    }

    ~KLruKCache() override = default;
    KLruKCache(const KLruKCache&) = delete;
    KLruKCache& operator=(const KLruKCache&) = delete;

    void put(Key key,Value value) override
    {
        std::lock_guard<std::mutex> guard(mutex_);
        if(capacity_<=0)
        {
            return;
        }
        Value existing{};//临时变量
        if(main_.get(key,existing))
        {
            main_.put(key,value);
            return;
        }

        HistoryRecord record = recordAccess(key);
        record.value = value;
        saveOrPromote(key,record);
    }

    bool get(Key key,Value& value) override
    {
        std::lock_guard<std::mutex> guard(mutex_);
        if(capacity_ <= 0)
        {
            return false;
        }
        if(main_.get(key,value))//现在缓存里面找
        {
            return true;
        }
        //然后再从历史区里面找
        HistoryRecord record = recordAccess(key);
        if(saveOrPromote(key,record))
        {
            value = *record.value;
            return true;
        }
        return false;
    }

    Value get(Key key) override
    {
        Value value{};
        get(key,value);
        return value;
    }

    void remove(Key key)
    {
        std::lock_guard<std::mutex> guard(mutex_);
        main_.remove(key);
        history_.remove(key);
    }

    std::size_t mainSize() const
    {
        std::lock_guard<std::mutex> guard(mutex_);
        return main_.size();
    }

    std::size_t history_() const
    {
        std::lock_guard<std::mutex> guard(mutex_);
        return history_.size();
    }
private:
    //外部会保证上锁 这里内部不用再加锁

    HistoryRecord recordAccess(Key k)
    {
        HistoryRecord record{};//临时记录
        history_.get(k,record);//在历史区内查找这个 key 并返回信息给record
        if(record.count < static_cast<std::size_t>(k_))
        {
            ++record.count;//没到门槛就继续++
        }
        return record;//返回这个记录 内部是 count 和 optional value
    }

    bool saveOrPromote(Key key,const HistoryRecord& record)
    {
        //判断传进来这个record是否已经达到了门槛 并且 有值
        if(record.count >= static_cast<std::size_t>(k_) && record.value.has_value())
        {
            main_.put(key,*record.value);
            history_.remove(key);
            return true;
        }
        //没达到就写回历史区里
        history_.put(key,record);//写回历史区
        return false;
    }

private:
    const int capacity_;
    const int k_;
    mutable std::mutex mutex_;
    KLruCache<Key,Value> main_;
    KLruCache<Key,HistoryRecord> history_;
};
} // namespace KamaCache
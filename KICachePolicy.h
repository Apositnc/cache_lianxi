#pragma once





namespace KamaCache
{

template<typename Key,typename Value>
class KICachePolicy
{

public:

    virtual ~KICachePolicy() = default;
    virtual bool get(Key key,Value& value) = 0;
    virtual void put(Key key,Value value) = 0;
    virtual Value get(Key key) = 0;

};


} // namespace KamaCache



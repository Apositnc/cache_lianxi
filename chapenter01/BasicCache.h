#pragma once
#include <string>
#include <unordered_map>

class BasicCache
{
private:
    std::unordered_map<int,std::string> data_;
public:
    void put(int key,const std::string& value)
    {
        data_[key]=value;
    }

    bool get(int key,std::string& value) const
    {
        auto it = data_.find(key);
        if(it == data_.end())
        {
            return false;
        }
        value=it->second;
        return true;
    }
};


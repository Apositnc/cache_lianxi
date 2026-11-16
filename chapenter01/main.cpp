#include "BasicCache.h"
#include <assert.h>
#include <string>
#include <iostream>

int main()
{
    BasicCache cache;
    std::string bookName = "unchanged";
    
    bool hit = cache.get(1001,bookName);
    assert(!hit);
    assert(bookName == "unchanged");
    std::cout << "PASS: miss keeps output unchanged\n";

    cache.put(1001,"cpp primer");
    hit = cache.get(1001,bookName);
    assert(hit);
    assert(bookName == "cpp primer");

    std::cout << "PASS: stored value can be read\n";

    cache.put(1001, "C++ Primer, updated");
    hit = cache.get(1001, bookName);
    assert(hit);
    assert(bookName == "C++ Primer, updated");
    std::cout << "PASS: existing value is updated\n";

    // 4. 空字符串是合法值，不能据此判断未命中。
    cache.put(1002, "");
    hit = cache.get(1002, bookName);
    assert(hit);
    assert(bookName.empty());
    std::cout << "PASS: empty string is a valid value\n";

    std::cout << "All chapter 1 checks passed.\n";
    return 0;

}
#include <array>
#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "KLruCache.h"


using KamaCache::KICachePolicy;
using KamaCache::KLruCache;

// Example book ID -> page count; the function depends on the interface.
void storeAndCheck(KICachePolicy<int, int>& cache)
{
    cache.put(7, 70);
    int out = -1;
    bool hit = cache.get(7, out);
    assert(hit && out == 70);
}

void testTypes()
{
    KLruCache<int, std::string> books(2);
    books.put(1001, "C++ Primer");
    std::string book;
    bool hit = books.get(1001, book);
    assert(hit && book == "C++ Primer");

    KLruCache<std::string, int> pagesByCode(2);
    pagesByCode.put("B1001", 300); // Example page count.
    pagesByCode.put("", 0); // Cache boundary case: empty key, zero value.
    int pageCount = -1;
    hit = pagesByCode.get("B1001", pageCount);
    assert(hit && pageCount == 300);
    hit = pagesByCode.get("", pageCount);
    assert(hit && pageCount == 0);

    pageCount = 123;
    hit = pagesByCode.get("missing", pageCount);
    assert(!hit && pageCount == 123);
    std::cout << "PASS: different key and value types\n";
}

void testLruBehavior()
{
    KLruCache<int, int> cache(2);
    cache.put(1, 10);
    cache.put(2, 20);
    int out = -1;
    bool hit = cache.get(1, out);   // 2 -> 1
    assert(hit && out == 10);
    cache.put(3, 30);              // 1 -> 3; evict 2
    hit = cache.get(2, out);
    assert(!hit && out == 10);

    cache.put(1, 11);              // 3 -> 1; update refreshes recency
    cache.put(4, 40);              // 1 -> 4; evict 3
    hit = cache.get(3, out);
    assert(!hit);
    hit = cache.get(1, out);
    assert(hit && out == 11);
    hit = cache.get(4, out);
    assert(hit && out == 40);
    assert(cache.size() == 2);
    std::cout << "PASS: LRU behavior is preserved\n";
}

void testInterface()
{
    KLruCache<int, int> concrete(2);
    storeAndCheck(concrete);

    std::unique_ptr<KICachePolicy<int, int>> owned =
        std::make_unique<KLruCache<int, int>>(2);
    storeAndCheck(*owned);
    int result = owned->get(7);
    assert(result == 70);
    std::cout << "PASS: calls through the policy interface\n";
}

void testValueOnlyGet()
{
    KLruCache<int, int> numbers(2);
    numbers.put(7, 0);
    int present = numbers.get(7);
    int missing = numbers.get(404);
    assert(present == 0 && missing == 0);

    int out = 123;
    bool hit = numbers.get(7, out);
    assert(hit && out == 0);
    out = 123;
    hit = numbers.get(404, out);
    assert(!hit && out == 123);

    KLruCache<int, std::string> words(2);
    std::string absent = words.get(404);
    assert(absent.empty());
    assert(words.size() == 0);
    std::cout << "PASS: value-only get loses hit status\n";
}

void testBoundaryAndRemoval()
{
    KLruCache<int, int> zero(0);
    KLruCache<int, int> negative(-1);
    zero.put(1, 10);
    negative.put(1, 10);
    int out = 123;
    bool hit = zero.get(1, out);
    assert(!hit && out == 123);
    hit = negative.get(1, out);
    assert(!hit && out == 123);
    assert(zero.size() == 0 && negative.size() == 0);

    KLruCache<int, int> one(1);
    one.put(1, 10);
    one.put(2, 20);
    hit = one.get(1, out);
    assert(!hit);
    one.remove(2);
    one.remove(999);
    assert(one.size() == 0);
    one.put(3, 30);
    hit = one.get(3, out);
    assert(hit && out == 30);
    std::cout << "PASS: nonpositive capacity and removal\n";
}

void testConcurrentIndependentKeys()
{
    constexpr int workerCount = 4;
    constexpr int keysPerWorker = 100;
    KLruCache<int, int> cache(workerCount * keysPerWorker);
    std::array<int, workerCount> errors{};
    std::vector<std::thread> workers;

    for (int worker = 0; worker < workerCount; ++worker)
    {
        workers.emplace_back([&, worker]()
        {
            for (int i = 0; i < keysPerWorker; ++i)
            {
                int key = worker * keysPerWorker + i;
                cache.put(key, key * 10);
                int out = -1;
                bool hit = cache.get(key, out);
                if (!hit || out != key * 10)
                {
                    ++errors[worker];
                }

                cache.put(key, key * 10 + 1);
                int updated = cache.get(key);
                if (updated != key * 10 + 1)
                {
                    ++errors[worker];
                }
            }
        });
    }

    for (auto& worker : workers)
    {
        worker.join();
    }
    for (int errorCount : errors)
    {
        assert(errorCount == 0);
    }
    assert(cache.size() == workerCount * keysPerWorker);
    for (int key = 0; key < workerCount * keysPerWorker; ++key)
    {
        int out = -1;
        bool hit = cache.get(key, out);
        assert(hit && out == key * 10 + 1);
    }
    std::cout << "PASS: concurrent independent keys\n";
}

void testConcurrentSharedKeys()
{
    constexpr int workerCount = 4;
    constexpr int capacity = 8;
    constexpr int keyCount = 32;
    KLruCache<int, int> cache(capacity);
    std::array<int, workerCount> errors{};
    std::vector<std::thread> workers;

    for (int worker = 0; worker < workerCount; ++worker)
    {
        workers.emplace_back([&, worker]()
        {
            for (int i = 0; i < 2000; ++i)
            {
                int key = (i + worker * 7) % keyCount;
                if (i % 3 == 0)
                {
                    cache.put(key, key * 10);
                }
                else if (i % 3 == 1)
                {
                    int out = -1;
                    bool hit = cache.get(key, out);
                    if ((hit && out != key * 10) || (!hit && out != -1))
                    {
                        ++errors[worker];
                    }
                }
                else
                {
                    cache.remove(key);
                }

                if (cache.size() > static_cast<std::size_t>(capacity))
                {
                    ++errors[worker];
                }
            }
        });
    }

    for (auto& worker : workers)
    {
        worker.join();
    }
    for (int errorCount : errors)
    {
        assert(errorCount == 0);
    }
    assert(cache.size() <= static_cast<std::size_t>(capacity));
    std::cout << "PASS: concurrent shared keys\n";
}

int main()
{
    testTypes();
    testLruBehavior();
    testInterface();
    testValueOnlyGet();
    testBoundaryAndRemoval();
    testConcurrentIndependentKeys();
    testConcurrentSharedKeys();
    std::cout << "All chapter 3 checks passed.\n";
    return 0;
}
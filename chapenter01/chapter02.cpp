#include <cassert>
#include <iostream>
#include <string>

#include "LruCache.h"

void expectValue(LruCache& cache, int key, const std::string& expected)
{
    std::string out;
    bool hit = cache.get(key, out);
    assert(hit);
    assert(out == expected);
}

void expectMiss(LruCache& cache, int key)
{
    std::string out = "unchanged";
    bool hit = cache.get(key, out);
    assert(!hit);
    assert(out == "unchanged");
}

int main()
{
    {
        LruCache cache(2);
        expectMiss(cache, 1);
        cache.put(1, "A");
        cache.put(2, "B");            // 1 -> 2
        expectValue(cache, 1, "A");   // 2 -> 1
        cache.put(3, "C");            // 1 -> 3; evict 2
        assert(cache.size() == 2);
        expectMiss(cache, 2);
        expectValue(cache, 1, "A");
        expectValue(cache, 3, "C");
        std::cout << "PASS: reads refresh recency\n";
    }

    {
        LruCache cache(2);
        cache.put(1, "A");
        cache.put(2, "B");
        cache.put(1, "A2");           // 2 -> 1; update, no eviction
        assert(cache.size() == 2);
        cache.put(3, "C");            // 1 -> 3; evict 2
        expectMiss(cache, 2);
        expectValue(cache, 1, "A2");
        expectValue(cache, 3, "C");

        // Updating the newest key must not evict the other key.
        LruCache newest(2);
        newest.put(1, "A");
        newest.put(2, "B");
        newest.put(2, "B2");
        assert(newest.size() == 2);
        expectValue(newest, 1, "A");
        expectValue(newest, 2, "B2");
        std::cout << "PASS: updates refresh recency\n";
    }

    {
        LruCache cache(3);
        cache.put(1, "A");
        cache.put(2, "B");
        cache.put(3, "C");            // 1 -> 2 -> 3
        expectValue(cache, 2, "B");   // 1 -> 3 -> 2; move middle
        expectValue(cache, 2, "B");   // moving newest is also valid
        cache.put(4, "D");            // 3 -> 2 -> 4; evict 1
        expectMiss(cache, 1);
        expectValue(cache, 3, "C");   // 2 -> 4 -> 3
        cache.put(5, "E");            // 4 -> 3 -> 5; evict 2
        expectMiss(cache, 2);
        assert(cache.size() == 3);
        expectValue(cache, 4, "D");
        expectValue(cache, 3, "C");
        expectValue(cache, 5, "E");
        std::cout << "PASS: middle and repeated moves work\n";
    }

    {
        LruCache cache(2);
        cache.put(1, "A");
        cache.put(2, "B");
        expectMiss(cache, 999);        // still 1 -> 2
        assert(cache.size() == 2);
        cache.put(3, "C");
        expectMiss(cache, 1);
        expectValue(cache, 2, "B");
        std::cout << "PASS: misses do not insert or refresh\n";
    }

    {
        LruCache cache(1);
        cache.put(1, "A");
        expectValue(cache, 1, "A");
        expectValue(cache, 1, "A");
        cache.put(1, "A2");
        assert(cache.size() == 1);
        expectValue(cache, 1, "A2");
        cache.put(2, "B");
        expectMiss(cache, 1);
        expectValue(cache, 2, "B");
        cache.put(3, "C");
        expectMiss(cache, 2);
        expectValue(cache, 3, "C");
        assert(cache.size() == 1);
        std::cout << "PASS: capacity one works\n";
    }

    {
        LruCache zero(0);
        LruCache negative(-2);
        zero.put(1, "A");
        negative.put(1, "A");
        expectMiss(zero, 1);
        expectMiss(negative, 1);
        zero.remove(1);
        negative.remove(1);
        assert(zero.size() == 0);
        assert(negative.size() == 0);
        std::cout << "PASS: nonpositive capacity stores nothing\n";
    }

    {
        LruCache cache(3);
        cache.put(1, "A");
        cache.put(2, "B");
        cache.put(3, "C");
        cache.remove(2);              // remove middle: 1 -> 3
        cache.remove(999);            // missing removal does nothing
        expectMiss(cache, 2);
        assert(cache.size() == 2);
        cache.remove(1);              // remove oldest: 3
        cache.remove(3);              // remove only node: empty
        assert(cache.size() == 0);
        cache.put(4, "D");
        cache.put(5, "E");
        cache.remove(5);              // remove newest: 4
        expectValue(cache, 4, "D");
        expectMiss(cache, 5);
        assert(cache.size() == 1);
        std::cout << "PASS: removal and reinsertion work\n";
    }

    {
        LruCache cache(2);
        cache.put(0, "");             // same key as sentinel placeholder
        cache.put(-1, "negative key");
        expectValue(cache, 0, "");
        expectValue(cache, -1, "negative key");
        assert(cache.size() == 2);
        std::cout << "PASS: sentinels do not reserve user keys\n";
    }

    std::cout << "All chapter 2 checks passed.\n";
    return 0;
}
#include <wc/count.h>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"


using namespace wc;


TEST_CASE("top_word_collection", "[top_word_collection]") {
    top_word_collection<3> top_words;
    REQUIRE(0 == top_words.get_min_count());

    SECTION("Min count updates after N words are inserted.") {
        top_words.add("cat", 15);
        top_words.add("dog", 14);
        top_words.add ("rat", 13);
        REQUIRE(13 == top_words.get_min_count());
        REQUIRE(3 == top_words.total_words());
    }

    SECTION("Min count doesn't update if there is still room.") {
        top_words.add("cat", 15);
        REQUIRE(0 == top_words.get_min_count());
        REQUIRE(1 == top_words.total_words());
        top_words.add("dog", 14);
        REQUIRE(0 == top_words.get_min_count());
        REQUIRE(2 == top_words.total_words());
    }

    SECTION("Ties are counted as stored words.") {
        top_words.add("cat", 15);
        REQUIRE(0 == top_words.get_min_count());
        REQUIRE(1 == top_words.total_words());
        top_words.add("dog", 15);
        REQUIRE(0 == top_words.get_min_count());
        REQUIRE(2 == top_words.total_words());
        top_words.add("rat", 15);
        REQUIRE(15 == top_words.get_min_count());
        REQUIRE(3 == top_words.total_words());
    }

    SECTION("Ties can increase the total words stored.") {
        top_words.add("cat", 15);
        REQUIRE(0 == top_words.get_min_count());
        REQUIRE(1 == top_words.total_words());
        top_words.add("dog", 14);
        REQUIRE(0 == top_words.get_min_count());
        REQUIRE(2 == top_words.total_words());
        top_words.add("rat", 13);
        REQUIRE(13 == top_words.get_min_count());
        REQUIRE(3 == top_words.total_words());
        // If the same word is added, the total words go up, but the
        // min count stays the same. So we have two ties for third place.
        top_words.add("snail", 13);
        REQUIRE(13 == top_words.get_min_count());
        REQUIRE(4 == top_words.total_words());
        // In theory, the list could grow as large as allowable for ties for
        // third place...
        for (int i = 0; i < 100; ++ i) {
            std::string word = std::to_string(i);
            top_words.add(word, 13);
            REQUIRE(13 == top_words.get_min_count());
            REQUIRE(5 + i == top_words.total_words());
        }

        // However, when a bigger word is inserted, all of the ties get
        // kicked out and the collection is shrunk, keeping just 3 words.
        top_words.add("horse", 14);
        REQUIRE(3 == top_words.total_words());
        REQUIRE(14 == top_words.get_min_count());
    }

    SECTION("Words less than min_count ignored.") {
        top_words.add("cat", 15);
        top_words.add("dog", 14);
        top_words.add ("rat", 13);
        REQUIRE(13 == top_words.get_min_count());
        REQUIRE(3 == top_words.total_words());
        top_words.add ("something-else", 12);
        REQUIRE(13 == top_words.get_min_count());
        REQUIRE(3 == top_words.total_words());
    }

}


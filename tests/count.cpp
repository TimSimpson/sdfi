#include <wc/count.h>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"


using namespace wc;


TEST_CASE("top_word_collection", "[top_word_collection]") {
    top_word_collection<3> top_words;
    REQUIRE(0 == top_words.get_min_count());

    SECTION("Min count updates after N words are inserted.") {
        top_words.insert("cat", 15);
        top_words.insert("dog", 14);
        top_words.insert("rat", 13);
        REQUIRE(13 == top_words.get_min_count());
        REQUIRE(3 == top_words.total_words());
    }

    SECTION("Min count doesn't update if there is still room.") {
        top_words.insert("cat", 15);
        REQUIRE(0 == top_words.get_min_count());
        REQUIRE(1 == top_words.total_words());
        top_words.insert("dog", 14);
        REQUIRE(0 == top_words.get_min_count());
        REQUIRE(2 == top_words.total_words());
    }

    SECTION("Ties are counted as stored words.") {
        top_words.insert("cat", 15);
        REQUIRE(0 == top_words.get_min_count());
        REQUIRE(1 == top_words.total_words());
        top_words.insert("dog", 15);
        REQUIRE(0 == top_words.get_min_count());
        REQUIRE(2 == top_words.total_words());
        top_words.insert("rat", 15);
        REQUIRE(15 == top_words.get_min_count());
        REQUIRE(3 == top_words.total_words());
    }

    SECTION("Ties can increase the total words stored.") {
        top_words.insert("cat", 15);
        REQUIRE(0 == top_words.get_min_count());
        REQUIRE(1 == top_words.total_words());
        top_words.insert("dog", 14);
        REQUIRE(0 == top_words.get_min_count());
        REQUIRE(2 == top_words.total_words());
        top_words.insert("rat", 13);
        REQUIRE(13 == top_words.get_min_count());
        REQUIRE(3 == top_words.total_words());
        top_words.insert("snail", 13);
        REQUIRE(13 == top_words.get_min_count());
        REQUIRE(4 == top_words.total_words());
    }



}

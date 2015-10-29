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




TEST_CASE("word_counter", "[word_counter]") {

    word_counter counter;
    auto add_word = [&counter](const std::string & text) {
        counter(text.begin(), text.end());
    };

    REQUIRE(0 == counter.words().size());


    SECTION("Reading a simple word.") {
        add_word("hello");
        REQUIRE(1 == counter.words().size());
        REQUIRE(1 == counter.words().at("hello"));
    }

    SECTION("Words aren't counted twice.") {
        add_word("hello");
        add_word("hello");
        REQUIRE(1 == counter.words().size());
        REQUIRE(2 == counter.words().at("hello"));
    }

    SECTION("Upper case words aren't counted twice.") {
        add_word("hello");
        add_word("HELLO");
        REQUIRE(1 == counter.words().size());
        REQUIRE(2 == counter.words().at("hello"));
    }

    SECTION("When reading multiple letters.") {
        for (int i = 0; i < 3; ++ i) {
            add_word("a");
            add_word("b");
            add_word("c");
            add_word("d");
            add_word("e");
        }
        REQUIRE(5 == counter.words().size());
        REQUIRE(3 == counter.words().at("a"));
        REQUIRE(3 == counter.words().at("b"));
        REQUIRE(3 == counter.words().at("c"));
        REQUIRE(3 == counter.words().at("d"));
        REQUIRE(3 == counter.words().at("e"));
    }

}

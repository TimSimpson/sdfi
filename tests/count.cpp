#include <wc/count.h>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"


using namespace wc;


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


TEST_CASE("is_word_character", "[word_counter]") {
    REQUIRE(is_word_character('a'));
    REQUIRE(is_word_character('z'));
    REQUIRE(is_word_character('A'));
    REQUIRE(is_word_character('Z'));
    REQUIRE(is_word_character('0'));
    REQUIRE(is_word_character('9'));
    REQUIRE(!is_word_character('@'));
    REQUIRE(!is_word_character(' '));
    REQUIRE(!is_word_character('_'));
}


TEST_CASE("read_blob", "[word_counter]") {
    word_counter counter;
    auto add_text = [&counter](const std::string & text) {
        return read_blob(text.begin(), text.end(), counter);
    };

    REQUIRE(0 == counter.words().size());

    SECTION("Word is not read unless non-word char is seen at the end.") {
        std::string text("hello");
        auto itr = read_blob(text.begin(), text.end(), counter);
        REQUIRE(0 == counter.words().size());
        REQUIRE(counter.words().end() == counter.words().find("hello"));
        // An iterator to the start of the last word it was seeing is returned.
        REQUIRE(text.begin() == itr);
    }

    SECTION("Reading a simple word.") {
        add_text("hello ");
        REQUIRE(1 == counter.words().size());
        REQUIRE(1 == counter.words().at("hello"));
    }

    SECTION("Words aren't counted twice.") {
        add_text("hello hello ");
        REQUIRE(1 == counter.words().size());
        REQUIRE(2 == counter.words().at("hello"));
    }

    SECTION("Upper case words aren't counted twice.") {
        add_text("hello HELLO ");
        REQUIRE(1 == counter.words().size());
        REQUIRE(2 == counter.words().at("hello"));
    }

    SECTION("When reading multiple letters.") {
        add_text("a b c d e A b C D E a B C D E ");
        REQUIRE(5 == counter.words().size());
        REQUIRE(3 == counter.words().at("a"));
        REQUIRE(3 == counter.words().at("b"));
        REQUIRE(3 == counter.words().at("c"));
        REQUIRE(3 == counter.words().at("d"));
        REQUIRE(3 == counter.words().at("e"));
    }

    SECTION("Every nonvalid character is ignored.") {
        add_text("a|b#c!@d#$e*()A~b=C+D _E `a'B{C}D=E/");
        REQUIRE(5 == counter.words().size());
        REQUIRE(3 == counter.words().at("a"));
        REQUIRE(3 == counter.words().at("b"));
        REQUIRE(3 == counter.words().at("c"));
        REQUIRE(3 == counter.words().at("d"));
        REQUIRE(3 == counter.words().at("e"));
    }

}

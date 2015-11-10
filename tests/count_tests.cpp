#include <wc/communication.h>
#include <wc/count.h>
#include <wc/top.h>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"


using std::string;
using std::stringstream;
using namespace wc;


TEST_CASE("word_counter", "[word_counter]") {
    word_counter counter;
    auto add_word = [&counter](const string & text) {
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
    auto add_text = [&counter](const string & text, bool eof) {
        return read_blob(text.begin(), text.end(), eof, counter);
    };

    REQUIRE(0 == counter.words().size());

    SECTION("Word is not read unless non-word char is seen at the end...") {
        string text("hello");
        auto itr = read_blob(text.begin(), text.end(), false, counter);
        REQUIRE(0 == counter.words().size());
        REQUIRE(counter.words().end() == counter.words().find("hello"));
        // An iterator to the start of the last word it was seeing is returned.
        REQUIRE(text.begin() == itr);
    }

    SECTION("... except if this is the last bit of text.") {
        string text("hello");
        auto itr = read_blob(text.begin(), text.end(), true, counter);
        REQUIRE(1 == counter.words().size());
        REQUIRE(1 == counter.words().at("hello"));
        // An iterator to the start of the last word it was seeing is returned.
        REQUIRE(text.end() == itr);
    }

    SECTION("Reading a simple word.") {
        add_text("hello ", false);
        REQUIRE(1 == counter.words().size());
        REQUIRE(1 == counter.words().at("hello"));
    }

    SECTION("Reading a simple word w/ eof.") {
        add_text("hello ", true);
        REQUIRE(1 == counter.words().size());
        REQUIRE(1 == counter.words().at("hello"));
    }

    SECTION("Words aren't counted twice.") {
        add_text("hello hello ", false);
        REQUIRE(1 == counter.words().size());
        REQUIRE(2 == counter.words().at("hello"));
    }

    SECTION("Upper case words aren't counted twice.") {
        add_text("hello HELLO ", false);
        REQUIRE(1 == counter.words().size());
        REQUIRE(2 == counter.words().at("hello"));
    }

    SECTION("When reading multiple letters.") {
        add_text("a b c d e A b C D E a B C D E ", false);
        REQUIRE(5 == counter.words().size());
        REQUIRE(3 == counter.words().at("a"));
        REQUIRE(3 == counter.words().at("b"));
        REQUIRE(3 == counter.words().at("c"));
        REQUIRE(3 == counter.words().at("d"));
        REQUIRE(3 == counter.words().at("e"));
    }

    SECTION("Every nonvalid character is ignored.") {
        add_text("a|b#c!@d#$e*()A~b=C+D _E `a'B{C}D=E/", false);
        REQUIRE(5 == counter.words().size());
        REQUIRE(3 == counter.words().at("a"));
        REQUIRE(3 == counter.words().at("b"));
        REQUIRE(3 == counter.words().at("c"));
        REQUIRE(3 == counter.words().at("d"));
        REQUIRE(3 == counter.words().at("e"));
    }

}


TEST_CASE("read_using_buffer", "[read_using_buffer]") {
    // Rather than pass in a word counter, we pass in a simple function
    // that records every single thing read_blob sends it. That makes it
    // easier to see any random garbage it might be getting called with.

    std::vector<string> results;
    auto record_result = [&results](auto begin_str, auto end_str) {
        results.push_back(string(begin_str, end_str));
    };
    auto processor = [&record_result](auto begin, auto end, bool eof) {
        return read_blob(begin, end, eof, record_result);
    };

    stringstream input;

    SECTION("No cut off problems.") {
        // Goal here is to prove that if the processor returns anything other
        // than the end_itr (the end of the buffer) then the read_using_buffer
        // function will copy this data to the start and let it try again.
        // So for example, with a buffer of 5 let's say we had the text "a taco!".
        //
        // The first pass would load "a tac". The processor function should at
        // that point see the words "a" but nothing else and return that the
        // last place it saw a word as the "t".
        //
        // The function should then copy "tac" to the start, and read in 2 more
        // bytes, leaving us with "taco!" which will then be correctly read.
        input << "a taco!";
        read_using_buffer<5>(input, processor);
        CHECK(2 == results.size());
        CHECK(results[0] == "a");
        CHECK(results[1] == "taco");
    }

    SECTION("No cut off problems 2.") {
        // In the first pass, the buffer will be "a tac". "a" should be
        // recorded, but not "tac". On the next pass, the buffer should be
        // "tac!" and "tac" should be recorded.
        input << "a tac!";
        read_using_buffer<5>(input, processor);
        CHECK(2 == results.size());
        CHECK(results[0] == "a");
        CHECK(results[1] == "tac");
    }

    SECTION("short buffers are identified") {
        // Makes sure a correct exception gets raised if the buffer is too
        // small.
        input << "a burrito!";
        REQUIRE_THROWS_AS({
            read_using_buffer<5>(input, processor);
        },
        std::length_error);
    }

    SECTION("Avoid double counting") {
        // In the first pass, the buffer will be "a tac". "a" should be
        // recorded, but not "tac". On the next pass, the buffer should be
        // "tac!" and "tac" should be recorded.
        input << "a taco taco taco taco taco!";
        read_using_buffer<5>(input, processor);
        CHECK(6 == results.size());
        CHECK(results[0] == "a");
        for (int i = 1; i <= 5; ++ i) {
            CHECK(results[1] == "taco");
        }
    }

    SECTION("Always include words at the end.") {
        // Makes sure words that happen right before the end of the stream
        // aren't ignore.
        input << "a taco";
        read_using_buffer<5>(input, processor);
        CHECK(2 == results.size());
        CHECK(results[0] == "a");
        CHECK(results[1] == "taco");
    }

    SECTION("Ensure interrupted process always sees all input.") {
        // If a processor returns prematurely, there is a risk that it won't
        // see all of the data, especially if the stream is EOF.
        // This test makes sure that doesn't happen.
        int ip_index = -1;
        auto interruptable_processor = [&ip_index, &record_result](
            auto begin, auto end, bool eof)
        {
            ++ ip_index;
            if (0 == ip_index) {
                CHECK(string(begin, end) == "a tac");
                return begin + 3;
            } else if (1 <= ip_index && ip_index <= 10) {
                // The buffered reader needs to call us back repeatedly
                // with this same data.
                CHECK(string(begin, end) == "aco");
                return begin;
            } else if (ip_index == 11) {
                CHECK(string(begin, end) == "aco");
                return begin + 1;
            } else if (ip_index == 12) {
                CHECK(string(begin, end) == "co");
                CHECK(end == begin + 2);
                return end;
            }
            CHECK(ip_index <= 12);
        };

        input << "a taco";
        read_using_buffer<5>(input, interruptable_processor);
        CHECK(ip_index == 12);
    }
}


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
            string word = std::to_string(i);
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


TEST_CASE("communication", "[queue_distributor]") {
    string s;
    typedef decltype(s.begin()) Iterator;

    struct fake_queue {

        int available;
        Iterator begin;
        Iterator end;

        fake_queue()
        :   available(0),
            begin(nullptr),
            end(nullptr)
        {}

        Iterator process_text(Iterator begin, Iterator end) {
            this->begin = begin;
            this->end = end;
            available -= end - begin;
            return end;
        }

        int write_available() const {
            return available;
        }
    };

    fake_queue q1;
    fake_queue q2;
    fake_queue q3;
    std::vector<fake_queue *> queues = {&q1, &q2, &q3};

    using distributor = queue_distributor<std::vector<fake_queue *>, fake_queue>;

    distributor dist(queues);

    REQUIRE(0 == q1.available);
    REQUIRE(0 == q2.available);
    REQUIRE(0 == q3.available);

    const Iterator null_itr(nullptr);

    #define CHECK_INITIAL_STATE(q) { \
        CHECK(q.available == 0); \
        CHECK(q.begin == null_itr); \
        CHECK(q.end == null_itr); \
    }

    SECTION("Checking our fake works.") {
        string text("A dinosaur.");
        q1.available = 300;
        q1.process_text(text.begin(), text.end());
        CHECK(q1.begin == text.begin());
        CHECK(q1.end == text.end());
        CHECK(q1.available == 300 - text.size());
    }

    SECTION("Queue with most space is always selected.") {
        q2.available = 10;
        {
            auto * q = dist.find_available_queue();
            CHECK(q == &q2);
        }
        q3.available = 100;
        {
            auto * q = dist.find_available_queue();
            CHECK(q == &q3);
        }
    }

    SECTION("Valid start is always at the first word.") {
        string text("!#*(#() !* >> cat #&^#!!");
        const auto itr = dist.find_valid_start(text.begin(), text.end());
        CHECK(itr == text.begin() + text.find("cat"));
    }

    SECTION("If no valid start is found, end is returned.") {
        string text("!#*(#() !* > !&*#& #&^#!!");
        const auto itr = dist.find_valid_start(text.begin(), text.end());
        CHECK(itr == text.end());
    }

    SECTION("Valid start can point to begin.") {
        {
            string text("cat #&^#!!");
            const auto itr = dist.find_valid_start(text.begin(), text.end());
            CHECK(itr == text.begin());
        }
        {
            string text("cat");
            const auto itr = dist.find_valid_start(text.begin(), text.end());
            CHECK(itr == text.begin());
        }
    }


    SECTION("Valid end can == end if EOF and desired_max_size is sufficient.") {
        {
            string text("!!?supercalifragilisticexpialidocious");
            const auto itr = dist.find_valid_end(
                text.begin(), text.end(), true,
                text.size());
            CHECK(itr == text.end());
        }
        {
            string text("supercalifragilisticexpialidocious");
            const auto itr = dist.find_valid_end(
                text.begin(), text.end(), true,
                text.size());
            CHECK(itr == text.end());
        }
    }

    SECTION("Valid end must not == end if ending on word and not EOF.") {
        {
            string text("!!?supercalifragilisticexpialidocious");
            const auto itr = dist.find_valid_end(
                text.begin(), text.end(), false,
                text.size());
            CHECK(itr == text.begin() + 3);
        }
        {
            string text("supercalifragilisticexpialidocious");
            const auto itr = dist.find_valid_end(
                text.begin(), text.end(), false,
                text.size());
            CHECK(itr == text.begin());
        }
    }

    SECTION("Valid end must not == end if space insufficient.") {
        string text("!!?supercalifragilisticexpialidocious");
        const auto itr = dist.find_valid_end(
            text.begin(), text.end(), true,
            text.size() - 1);
        CHECK(itr == text.begin() + 3);
    }

    // A bit paranoid...
    SECTION("Valid end must not == end if space insufficient and not EOF!") {
        string text("!!?supercalifragilisticexpialidocious");
        const auto itr = dist.find_valid_end(
            text.begin(), text.end(), false,
            text.size() - 1);
        CHECK(itr == text.begin() + 3);
    }

    SECTION("Should go to end of word if possible") {
        string text("!!?supercalifragilisticexpialidocious.");
        const auto itr = dist.find_valid_end(
            text.begin(), text.end(), false,
            text.size());
        CHECK(itr == text.end());
    }

    SECTION("Valid end must not == end if space insufficient and not EOF!") {
        string text("!!?supercalifragilisticexpialidocious..");
        const auto itr = dist.find_valid_end(
            text.begin(), text.end(), false,
            text.size() - 1);
        CHECK(itr == text.end() - 1);
    }

    // Testing the whole kit and caboodle.

    // The distributor will skip right past all of the letters if it needs to.
    SECTION("If no words present, nothing is sent.") {
        string text("!^&#^* !@#&^__==++//.. .");
        auto end_itr = dist(text.begin(), text.end(), true);
        CHECK(end_itr == text.end());
        CHECK_INITIAL_STATE(q1);
        CHECK_INITIAL_STATE(q2);
        CHECK_INITIAL_STATE(q3);
    }


    SECTION("Highest write available is selected.") {
        q2.available = 2;
        string text("Hi");
        auto end_itr = dist(text.begin(), text.end(), true);
        CHECK(q2.available == 0);
        CHECK(q2.begin == text.begin());
        CHECK(q2.end == text.end());
        CHECK(end_itr ==  text.end());
        CHECK_INITIAL_STATE(q1);
        CHECK_INITIAL_STATE(q3);
    }

    SECTION("Distributor only writes what it can.") {
        q2.available = 8;
        q3.available = 7;

        string text("Hi there you!");
        auto end_itr = dist(text.begin(), text.end(), true);
        CHECK(q2.available == 0);
        CHECK(q3.available == 7);
        CHECK(q2.begin == text.begin());
        CHECK(q2.end == text.begin() + text.find(" you"));
        REQUIRE(end_itr == text.begin() + text.find(" you"));

        auto end_itr2 = dist(end_itr, text.end(), true);
        CHECK(q2.available == 0);
        CHECK(q3.available == 3);
        CHECK(q2.begin == text.begin());
        CHECK(q2.end == text.begin() + text.find(" you"));
        CHECK(q3.begin == text.begin() + text.find("you"));
        CHECK(q3.end == text.end());
        CHECK(end_itr2 == text.end());
    }

    SECTION("If not EOF, can't proceed for small strings...") {
        q2.available = 2;
        string text("Hi");
        REQUIRE_THROWS_AS({
            dist(text.begin(), text.end(), false);
        }, std::logic_error);
        CHECK(q2.available == 2);
        CHECK_INITIAL_STATE(q1);
        CHECK_INITIAL_STATE(q3);
    }

    SECTION("Must capture large word in the middle of junk.") {
        string text("!!?supercalifragilisti_e_pialidocious.");
        q1.available = text.size() - 3;
        const auto itr = dist(text.begin(), text.end(), false);
        CHECK(itr == text.end());
        CHECK(q1.available == 0);
        CHECK(q1.begin == text.begin() + 3);
        CHECK(q1.end == text.end());
        CHECK_INITIAL_STATE(q2);
        CHECK_INITIAL_STATE(q3);
    }

    #undef CHECK_INITIAL_STATE
};

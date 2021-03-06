#ifndef FILE_GUARD_WC_TOP_H
#define FILE_GUARD_WC_TOP_H

#include <cctype>
#include <map>
#include <string>
#include <vector>
#include <utility>

namespace wc {


// Keeps N number of top words. Use the add method to increment the words.
template<int top_count>
class top_word_collection {
public:
    using pair = std::pair<std::string, std::size_t>;

    top_word_collection()
    :   min_count(0),
        words()
    {}

    std::size_t get_min_count() const {
        return min_count;
    }

    void add(const std::string & word, const std::size_t count) {
        if (min_count > count) {
            return;  // Ignore
        }
        pair new_value(word, count);
        // Find first position not greater than count.
        auto position = std::lower_bound(
            words.begin(),
            words.end(),
            new_value,
            [](pair a, pair b) { return a.second > b.second; }
        );
        words.insert(position, new_value);
        trim();
    }

    const std::vector<pair> & get_words() const {
        return words;
    }

    // Total number of words including ties.
    size_t total_words() const {
        return words.size();
    }

private:
    std::size_t min_count;
    std::vector<pair> words;

    void trim() {
        // Ensure that the number of words were storing only exceeds the
        // requred count if the last words are ties.
        if (words.size() < top_count) {
            return;
        }

        min_count = words[top_count - 1].second;

        if (words.size() > top_count) {
            // We could just erase the end of the vector but we want to include
            // words that are tieing for last place. The code below finds the
            // first non unique item.
            min_count = words[top_count - 1].second;

            auto itr = words.begin() + top_count;
            while(itr != words.end() && itr->second == min_count) {
                ++ itr;
            }
            words.erase(itr, words.end());
        }
    }
};


}  // end namespace wc

#endif

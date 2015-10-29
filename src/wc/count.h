#ifndef FILE_GUARD_WC_COUNT_H
#define FILE_GUARD_WC_COUNT_H

#include <cctype>
#include <map>
#include <string>
#include <vector>
#include <utility>

namespace wc {


// Returns true if C is part of a word.
inline bool is_word_character(char c) {
    return ((c >= 'A' && c <= 'Z')
            || (c >= 'a' && c <= 'z')
            || (c >= '0' && c <= '9'));
}


template<typename Iterator, typename Char, typename Func>
void read_blob(Iterator begin, Iterator end, Func receive_word) {
    bool in_word = false;
    string word;
    Iterator word_start;
    Char c;
    for(Iterator i = begin; i != end; ++i) {
        c = *i;
        if (!in_word && is_word_character(c)) {
            word_start = i;
        } else if (in_word && !is_word_character(c)) {
            //TODO: Check i -1 is ok.
            receive_word(word_start, i - 1);
        }
    }
}


// Keeps N number of top words. Use the add method to increment the words.
template<int top_count>
class top_word_collection {
public:
    using pair = std::pair<std::string, int>;

    top_word_collection()
    :   min_count(0),
        words()
    {}

    int get_min_count() const {
        return min_count;
    }

    void add(const std::string & word, const int count) {
        // ASSERT count >= min_count
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
        // ASSERT min_count <= top_words.back().second
        // Update minimum if it's just been replaced.
        if (total_words() >= top_count)
        {
            min_count = words.back().second;
        }
    }

    // Total number of words including ties.
    size_t total_words() const {
        return words.size();
    }

private:
    int min_count;
    std::vector<pair> words;

    void trim() {
        // Ensure that the number of words were storing only exceeds the
        // requred count if the last words are ties.
        if (words.size() <= top_count) {
            return;
        }
        // We could just erase the end of the vector but we want to include
        // words that are tieing for last place. The code below finds the first
        // non unique item.
        const int min_count = words[top_count - 1].second;

        auto itr = words.begin() + top_count;
        while(itr != words.end() && itr->second == min_count) {
            ++ itr;
        }
        words.erase(itr, words.end());
    }
};


// Counts words, keeping a map of all word counts and the top ten.
class word_counter {
public:
    word_counter()
    :   counts(),
        word()
    {
    }

    template<typename Iterator>
    void operator()(Iterator start, Iterator end) {
        word = std::string(start, end);
        std::transform(word.begin(), word.end(), word.begin(), std::tolower);
        counts[word] ++;
    }

    const std::map<std::string, int> & words() const {
        return counts;
    }

private:
    std::map<std::string, int> counts;
    std::string word;
};

}  // end namespace wc

#endif

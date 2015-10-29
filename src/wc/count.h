#ifndef FILE_GUARD_WC_COUNT_H
#define FILE_GUARD_WC_COUNT_H


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


// Keeps N number of top words.
template<int top_count>
class top_word_collection {
public:
    using pair = std::pair<std::string, int>;

    top_words()
    :   counts(),
        min_collection_count(0),
        words()
    {}

    int get_min_count() const {
        return min_count;
    }

    void insert(const std::string & word, const int count) {
        // ASSERT count >= min_collection_count
        pair new_value(word, count);
        // Find first position not greater than count.
        auto position = std::lower_bound(
            counts.begin(),
            counts.end(),
            new_value,
            [](pair a, pair b) { return a.second > b.second; }
        );
        words.insert(position, new_value);
        trim();
        // ASSERT min_collection_count <= top_words.back().second
        min_collection_count = words.back().second;
    }

private:
    std::vector<int> counts;
    int min_count;
    std::vector<info> words;

    void trim() {
        // ASSERT PRE - original = words.length();

        // Now see if there are more than top_count unique words. If so,
        // delete some.
        if (words.length() <= top_words) {
            return;
        }
        int unique_count = 0;
        auto itr = words.begin();
        auto last = itr;
        ++ itr;
        while (itr != words.end()) {
            if (itr->second != last->second) {
                ++ unique_count;
            }
            if (unique_count > top_count) {
                words.erase(itr, words.end());
                break;
            }
            auto last = itr;
            ++ itr;
        }

        // ASSERT POST - check top_words.length() <= original;
    }
};


// Counts words, keeping a map of all word counts and the top ten.
template<int top_count>
class word_counter {
public:
    word_counter()
    :   counts(),
        word(),
    {
    }

    void operator()(Iterator start, Iterator end) {
        word = std::string(start, end);
        const int count = (++ counts[word]);
        if (top_word_collection.get_min_count() <= count) {
            top_word_collection.insert(word, count);
        }
    }

private:
    std::map<std::string, int> counts;
    std::string word;
    top_words<top_count> top_counts;
};

}  // end namespace wc

#endif

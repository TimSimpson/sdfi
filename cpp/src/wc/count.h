#ifndef FILE_GUARD_WC_COUNT_H
#define FILE_GUARD_WC_COUNT_H


#include <algorithm>
#include <cctype>
#include <map>
#include <stdexcept>
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


// Reads the given blob into the receiver. Returns the start of the last word
// it was reading, or the end iterator if it finished.
template<typename Iterator, typename Func>
Iterator read_blob(Iterator begin, Iterator end, Func & receive_word)
{
    bool in_word = false;
    Iterator word_start;
    for(Iterator i = begin; i != end; ++i) {
        const bool word_char = is_word_character(*i);
        if (!in_word && word_char) {
            word_start = i;
            in_word = true;
        } else if (in_word && !word_char) {
            //TODO: Check i -1 is ok.
            receive_word(word_start, i);
            in_word = false;
        }
    }
    if (in_word) {
        return word_start;
    } else {
        return end;
    }
}


// last_pos = reader(start_itr, end_itr)

// [&file](start_itr, end_itr) {
//      file.read(start_itr, end_itr - start_itr);
//      const auto length = file.gcount();
//      return start_itr + length;
// }


template<int buffer_size, typename FileReader, typename Func>
void read_using_buffer(FileReader & file_reader, Func & process_text) {
    char buffer[buffer_size];
    auto start_itr = buffer;

    while(file_reader) {
        const auto max_length = buffer_size;
        const auto length = file_reader(start_itr, max_length);
        auto end_itr = start_itr + length;

        auto last_unprocessed_pos = process_text(start_itr, end_itr);
        if (last_unprocessed_pos == end_itr) {
            start_itr = buffer;
        } else {
            if (last_unprocessed_pos == buffer) {
                throw std::length_error("Buffer is too small to accomodate "
                    "continous data of this size.");
            }
            // Because the function didn't finish, we need to put that
            // data at the start so it can try again.
            std::copy(last_unprocessed_pos, end_itr, buffer);
            // TODO: Consider passing the previous itr back into process_text-
            //       then this could be skipped.
            start_itr = buffer;
        }
    }
}


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

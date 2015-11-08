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

using word_map = std::map<std::string, std::size_t>;

// Returns true if C is part of a word.
inline bool is_word_character(char c) {
    return ((c >= 'A' && c <= 'Z')
            || (c >= 'a' && c <= 'z')
            || (c >= '0' && c <= '9'));
}

// Reads the given blob into the receiver. Returns the start of the last word
// it was reading, or the end iterator if it finished.
template<typename Iterator, typename Func>
Iterator read_blob(Iterator begin, Iterator end, bool eof, Func & receive_word)
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
        if (eof) {
            // We've been promised this is the end, so read it.
            receive_word(word_start, end);
        } else {
            // Tell the caller how far we got so this will be replayed.
            return word_start;
        }
    }
    // If we just read a word or the last elements weren't intersting
    // tell the caller we've finished with the text.
    return end;
}


template<typename InputStream>
struct stream_ref_wrapper {
    InputStream & stream;

    stream_ref_wrapper(InputStream & stream)
    :   stream(stream)
    {}

    bool has_more() const {
        return stream && stream.good() && !stream.eof();
    }

    template<typename Iterator, typename SizeType>
    SizeType read(Iterator output_buffer, SizeType count) {
        stream.read(output_buffer, count);
        return stream.gcount();
    }
};


// Repeatedly reads from an input stream into a buffer it creates, calling a
// processor function to look at the contents of the buffer during each
// iteration.
// This processor function is passed the start and end iterators to a fresh
// chunk of data and a boolean argument to signify the end of the stream, and
// is expected to pass back an iterator to one element past the last position
// in the buffer it was able to process.
// See the tests for more details.
template<int buffer_size, typename Input, typename Func>
auto read_using_buffer(Input & input, Func & process_text)
    -> decltype(input.has_more(), void())
{
    char buffer[buffer_size];
    auto start_itr = buffer;

    while(input.has_more()) {
        const auto max_length = buffer_size;

        const auto length = input.read(
            start_itr, max_length - (start_itr - buffer));

        auto end_itr = start_itr + length;

        // Each time, process_text reads the full buffer- this is to give it
        // a chance to pick up any words it may have missed the last round,
        // which get copied back to the start of the buffer.
        auto last_unprocessed_pos = process_text(
            buffer, end_itr, !input.has_more());
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
            //       that way it wouldn't have to reparse the old data.
            start_itr = buffer + (end_itr - last_unprocessed_pos);
        }
    }
}


// This overload works for typical input streams. It uses the decltype to
// determine if the overload is appropriate by seeing if the argument has
// a "good" method.
template<int buffer_size, typename InputStream, typename Func>
auto read_using_buffer(InputStream & input_stream, Func & process_text)
    -> decltype(input_stream.good(), void())
{
    stream_ref_wrapper<decltype(input_stream)> wrapper(input_stream);
    read_using_buffer<buffer_size>(wrapper, process_text);
}

// Collects words in a word_map.
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
        std::transform(word.begin(), word.end(), word.begin(), ::tolower);
        counts[word] ++;
    }

    const word_map & words() const {
        return counts;
    }

private:
    word_map counts;
    std::string word;
};

}  // end namespace wc

#endif

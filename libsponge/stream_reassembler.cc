#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : buffer(), _output(capacity), _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_nonoverlapped_substring(const string &data, const size_t index) {
    auto next_elem = buffer.upper_bound(index);  // First buffered substring with larger index
    size_t _index = index;                       // Start index of nonoverlapped substring
    size_t _last_index = index + data.length();  // Last index of nonoverlapped substring

    // only data between next_assembled_index and first_unacceptable_index is allowed
    if (_index < next_assembled_index)
        _index = next_assembled_index;

    size_t first_unacceptable_index = next_assembled_index + (_capacity - _output.buffer_size());
    if (_last_index > first_unacceptable_index) {
        _last_index = first_unacceptable_index;
    }

    while (_index < _last_index) {
        /* Update _sub_last_index */
        size_t _sub_last_index = _last_index;
        if (next_elem != buffer.end() && next_elem->first < _sub_last_index) {
            _sub_last_index = next_elem->first;
        }

        /* Update _index */
        if (next_elem != buffer.begin()) {
            auto prev_elem = prev(next_elem);
            if (prev_elem->first + prev_elem->second.length() > _index) {
                _index = prev_elem->first + prev_elem->second.length();
            }
        }

        /* Push nonoverlapped substring to buffer */
        if (_index < _sub_last_index) {
            buffer.insert(make_pair(_index, data.substr(_index - index, _sub_last_index - _index)));
            _unassembled_bytes += _sub_last_index - _index;
        } else {
            _sub_last_index = _index + 1;
        }

        /* There's no more element in buffer to compare */
        if (next_elem == buffer.end())
            break;

        /* Update loop condition */
        next_elem++;
        _index = _sub_last_index;
    }
}

void StreamReassembler::reassemble() {
    auto top = buffer.begin();

    while (!buffer.empty() && next_assembled_index == top->first) {
        _output.write(top->second);
        next_assembled_index += top->second.length();
        _unassembled_bytes -= top->second.length();
        buffer.erase(top->first);
        top = buffer.begin();
    }
}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    /* Record eof_index when this segment is eof */
    if (eof)
        eof_index = index + data.length();

    /* Push all nonoverlapped substring of data into the buffer */
    push_nonoverlapped_substring(data, index);

    /* Move in-order segments into _output*/
    reassemble();

    /* Signal _output end_signal if all segments was assembled */
    if (next_assembled_index == eof_index) {
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled_bytes; }

bool StreamReassembler::empty() const { return buffer.empty(); }

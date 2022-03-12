#include "stream_reassembler.hh"
#include <iostream>

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
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    /* Record eof_index when this segment is eof */
    if (eof)
        eof_index = index + data.length();

    /* Remove overlapped data with unassembled bytes */
    uint64_t _index = index;
    uint64_t _last_index = index + data.length();

    while (_index < _last_index) {                      /* There're unassembled bytes before data */
        auto next_elem = buffer.lower_bound(_index);
        if (next_elem != buffer.end()) {
            if (next_elem->first <= _last_index) {
                _last_index = next_elem->first;
            }
        }
        if (next_elem != buffer.begin()) {  /* There're unassembled bytes before data */
            auto prev_elem = prev(next_elem);
            if (prev_elem->first + prev_elem->second.length() > _index) {
                _index = prev_elem->first + prev_elem->second.length();
            }
        } else if (next_assembled_index > _index) {     /* This data is the first unassembled bytes */
            _index = next_assembled_index;
        }

        if (_index >= _last_index) {        /* data is all overlapped */
            _index = _index + 1;
            _last_index = index + data.length();
            cout << endl;
            continue;
        }

        string _data = data.substr(_index - index, _last_index - _index);

        /* Check remaining capacity */
        uint64_t remain_capacity = _capacity - _unassembled_bytes - _output.buffer_size();
        while (remain_capacity < _data.length()) {
            if (buffer.empty()) {   /* Nothing to delete! Ignore data */
                if (remain_capacity == 0) return;
                _data = _data.substr(0, remain_capacity);
                break;
            }
            auto last_elem = prev(buffer.end());
            uint64_t remove_bytes = _data.length() - remain_capacity;

            if (remove_bytes >= last_elem->second.length()) {  /* last element is shorter than data */
                _unassembled_bytes -= last_elem->second.length();
                buffer.erase(last_elem->first);
            } else {
                _unassembled_bytes -= remove_bytes;
                last_elem->second = last_elem->second.substr(0, last_elem->second.length() - remove_bytes);
            }
            remain_capacity = _capacity - _unassembled_bytes - _output.buffer_size();
        }

        /* Insert adjusted data into buffer */
        buffer.insert(make_pair(_index, _data));
        _unassembled_bytes += _data.length();

        _index = _last_index + 1;
        _last_index = index + data.length();
    }

    /* Reassemble data in unassembled buffer */
    auto top = buffer.begin();

    while (!buffer.empty() && next_assembled_index == top->first) {
        _output.write(top->second);
        next_assembled_index += top->second.length();
        _unassembled_bytes -= top->second.length();
        buffer.erase(top->first);
        top = buffer.begin();
    }

    /* Signal _output end_signal if every input was assembled */
    if (next_assembled_index == eof_index) {
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled_bytes; }

bool StreamReassembler::empty() const { return buffer.empty(); }

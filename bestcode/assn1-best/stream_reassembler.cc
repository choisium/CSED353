#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity)
    : _output(capacity)
    , _capacity(capacity)
    , _end_idx(0)
    , _unassembled_bytes(0)
    , _eof_cond(0)
    , _eof_idx(0)
    , vec(capacity, 0)
    , buf(capacity, 0) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    size_t left_idx, right_idx, i, tmp_idx, tmp_written_bytes;
    string tmp_string = "";
    left_idx = index < _end_idx ? _end_idx : index;
    right_idx = (index + data.length()) < (_end_idx + _capacity) ? (index + data.length()) : (_end_idx + _capacity);
    // When eof sign comes, check eof condition is true and memo the eof index to end the input later
    if (eof) {
        _eof_cond = 1;
        _eof_idx = index + data.length();
    }
    // vec is where to check whether a bit is used or not, buf is where to designate a character to a bit
    for (i = left_idx; i < right_idx; i++) {
        tmp_idx = i % _capacity;
        // if(buf[tmp_idx] != data[i - index])
        //  if(buf[tmp_idx] > 0) break;
        if (vec[tmp_idx] == 0) {
            vec[tmp_idx] = 1;
            _unassembled_bytes += 1;
        }
        buf[tmp_idx] = data[i - index];
    }

    for (i = _end_idx; i < _end_idx + _capacity; i++) {
        tmp_idx = i % _capacity;
        if (vec[tmp_idx] == 0)
            break;
        tmp_string.push_back(buf[tmp_idx]);
    }
    tmp_written_bytes = _output.write(tmp_string);
    // Flush bits after the substring is written
    for (i = _end_idx; i < _end_idx + tmp_written_bytes; i++)
        vec[i % _capacity] = 0;
    _end_idx += tmp_written_bytes;
    _unassembled_bytes -= tmp_written_bytes;
    // check EOF condition.
    if (_eof_idx == _end_idx && _eof_cond == 1)
        _output.end_input();

    return;
}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled_bytes; }

bool StreamReassembler::empty() const { return _unassembled_bytes == 0; }

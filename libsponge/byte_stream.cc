#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) { _capacity = capacity; }

size_t ByteStream::write(const string &data) {
    size_t write_count = remaining_capacity();
    if (write_count > data.length())
        write_count = data.length();
    buffer += data.substr(0, write_count);
    _bytes_written += write_count;
    return write_count;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const { return buffer.substr(0, len); }

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    buffer = buffer.substr(len, _capacity);
    _bytes_read += len;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    std::string output = peek_output(len);
    pop_output(len);
    return output;
}

void ByteStream::end_input() { end_signal = true; }

bool ByteStream::input_ended() const { return end_signal; }

size_t ByteStream::buffer_size() const { return buffer.length(); }

bool ByteStream::buffer_empty() const { return buffer.length() == 0; }

bool ByteStream::eof() const { return end_signal && buffer.length() == 0; }

size_t ByteStream::bytes_written() const { return _bytes_written; }

size_t ByteStream::bytes_read() const { return _bytes_read; }

size_t ByteStream::remaining_capacity() const { return _capacity - buffer.length(); }

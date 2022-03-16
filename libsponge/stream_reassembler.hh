#ifndef SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
#define SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH

#include "byte_stream.hh"

#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

//! \brief A class that assembles a series of excerpts from a byte stream (possibly out of order,
//! possibly overlapping) into an in-order byte stream.
class StreamReassembler {
  private:
    // Your code here -- add private members as necessary.
    std::map<size_t, std::string> buffer;  //!< The buffer of unassembled segments
    size_t next_assembled_index = 0;       //!< The next index to be assembled
    size_t _unassembled_bytes = 0;         //!< The bytes of unassembled segments in buffer
    size_t eof_index = UINT64_MAX;         //!< The index of eof

    ByteStream _output;  //!< The reassembled in-order byte stream
    size_t _capacity;    //!< The maximum number of bytes

    void push_nonoverlapped_substring(const std::string &data, const size_t index);
    void resolve_overflow();
    void reassemble();

  public:
    //! \brief Construct a `StreamReassembler` that will store up to `capacity` bytes.
    //! \note This capacity limits both the bytes that have been reassembled,
    //! and those that have not yet been reassembled.
    StreamReassembler(const size_t capacity);

    //! \brief Receive a substring and write any newly contiguous bytes into the stream.
    //!
    //! The StreamReassembler will stay within the memory limits of the `capacity`.
    //! Bytes that would exceed the capacity are silently discarded.
    //!
    //! \param data the substring
    //! \param index indicates the index (place in sequence) of the first byte in `data`
    //! \param eof the last byte of `data` will be the last byte in the entire stream
    void push_substring(const std::string &data, const uint64_t index, const bool eof);

    //! \name Access the reassembled byte stream
    //!@{
    const ByteStream &stream_out() const { return _output; }
    ByteStream &stream_out() { return _output; }
    //!@}

    //! The number of bytes in the substrings stored but not yet reassembled
    //!
    //! \note If the byte at a particular index has been pushed more than once, it
    //! should only be counted once for the purpose of this function.
    size_t unassembled_bytes() const;

    //! \brief Is the internal state empty (other than the output stream)?
    //! \returns `true` if no substrings are waiting to be assembled
    bool empty() const;

    /* Below functions are auxiliary functions used in upper wrapper, tcp_receiver */
    uint64_t first_unassembled_seqno() const { return next_assembled_index; }
    size_t assembled_bytes() const { return _output.buffer_size(); }
    bool eof() const { return next_assembled_index == eof_index; }
};

#endif  // SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH

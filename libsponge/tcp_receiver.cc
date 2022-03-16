#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    const TCPHeader& header = seg.header();
    size_t length = seg.length_in_sequence_space();

    /* Set the Initial Sequence Number */
    if (header.syn) {
        length -= 1;
        _syn_flag = true;
        _isn = header.seqno;
    }

    if (header.fin) length -= 1;

    uint64_t stream_index;
    if (header.syn && length == 0) {
        stream_index = 0;
    } else {
        stream_index = unwrap(header.seqno, _isn, _reassembler.first_unassembled_seqno());
        if (!header.syn) stream_index -= 1;
    }

    /* Push payload to reassembler */
    if (_syn_flag) {
        _reassembler.push_substring(seg.payload().copy(), stream_index, header.fin);
        _ackno = wrap(_reassembler.first_unassembled_seqno() + 1, _isn);
        if (_reassembler.eof()) _ackno = _ackno + 1;
    }
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (_syn_flag) return _ackno;
    return nullopt;
}

size_t TCPReceiver::window_size() const { return _capacity - _reassembler.assembled_bytes(); }

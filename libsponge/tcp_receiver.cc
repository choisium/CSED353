#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    const TCPHeader& header = seg.header();

    /* Set the Initial Sequence Number */
    if (header.syn) {
        _isn = header.seqno;

        /* Only SYN signal came in */
        if (seg.length_in_sequence_space() == 0) {
            _reassembler.push_substring("", 0, header.fin);
            _ackno = WrappingInt32(0);
            return;
        }
    }

    /* Push payload to reassembler */
    uint64_t stream_index = unwrap(header.seqno, _isn, _reassembler.first_unassembled_seqno()) - 1;
    _reassembler.push_substring(seg.payload().copy(), stream_index, header.fin);
    _ackno = wrap(_reassembler.first_unassembled_seqno() + 1, _isn);
}

optional<WrappingInt32> TCPReceiver::ackno() const { return _ackno; }

size_t TCPReceiver::window_size() const { return _capacity - _reassembler.assembled_bytes(); }

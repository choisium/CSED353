#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

inline uint64_t TCPReceiver::first_unassembled_seqno() const {
    uint64_t _first_unassembled_seqno = stream_out().bytes_written();

    if (_syn_flag)
        _first_unassembled_seqno += 1;
    if (stream_out().input_ended())
        _first_unassembled_seqno += 1;

    return _first_unassembled_seqno;
}

void TCPReceiver::segment_received(const TCPSegment &seg) {
    const TCPHeader &header = seg.header();

    /* Set the initial sequence number */
    if (header.syn) {
        _syn_flag = true;
        _isn = header.seqno;
    }

    /* If _syn_flag is not set, ignore this segment */
    if (!_syn_flag)
        return;

    /* Set the final sequence number */
    if (header.fin) {
        _fin_flag = true;
    }

    /* Compute the stream_index of current segment */
    uint64_t stream_index = unwrap(header.seqno, _isn, first_unassembled_seqno());
    if (!header.syn) {
        stream_index -= 1;
    }

    /* Push payload to reassembler */
    _reassembler.push_substring(seg.payload().copy(), stream_index, header.fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (!_syn_flag)
        return nullopt;

    return wrap(first_unassembled_seqno(), _isn);
}

size_t TCPReceiver::window_size() const { return _capacity - stream_out().buffer_size(); }

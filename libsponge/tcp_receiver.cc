#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    const TCPHeader &header = seg.header();

    /* Set the Initial Sequence Number */
    if (header.syn) {
        _syn_flag = true;
        _isn = header.seqno;
    }

    /* If _syn_flag is not set, ignore this segment */
    if (!_syn_flag)
        return;

    /* Compute the stream_index of current segment */
    uint64_t stream_index;
    if (header.syn) {
        stream_index = 0;
    } else {
        stream_index = unwrap(header.seqno - 1, _isn, _reassembler.first_unassembled_index());
    }

    /* Push payload to reassembler */
    _reassembler.push_substring(seg.payload().copy(), stream_index, header.fin);
    _ackno = wrap(_reassembler.first_unassembled_index() + 1, _isn);
    if (_reassembler.eof())
        _ackno = _ackno + 1;
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (_syn_flag)
        return _ackno;
    return nullopt;
}

size_t TCPReceiver::window_size() const { return _capacity - _reassembler.assembled_bytes(); }

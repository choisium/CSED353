#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

inline uint64_t TCPReceiver::assembled_bytes() const { return stream_out().buffer_size(); }

inline uint64_t TCPReceiver::first_unassembled_seqno() const {
    uint64_t _first_unassembled_seqno = 0;
    if (_syn_flag) _first_unassembled_seqno += 1;
    _first_unassembled_seqno += stream_out().bytes_written();
    if (stream_out().input_ended()) _first_unassembled_seqno += 1;
    return _first_unassembled_seqno;
}

uint64_t TCPReceiver::first_unacceptable_seqno() const {
    uint64_t _first_unacceptable_seqno = first_unassembled_seqno() + window_size();
    if (_first_unacceptable_seqno == _fin_seqno - 1) {
        _first_unacceptable_seqno = _fin_seqno;
    }
    return _first_unacceptable_seqno;
}

bool TCPReceiver::update_seqno_space(WrappingInt32 seqno, uint64_t length) {
    uint64_t start_seqno = unwrap(seqno, _isn, first_unassembled_seqno());
    uint64_t end_seqno = start_seqno + length;
    uint64_t _first_unacceptable_seqno = first_unacceptable_seqno();
    auto next_elem = _seqno_space.upper_bound(start_seqno);

    if (start_seqno >= _first_unacceptable_seqno || end_seqno <= first_unassembled_seqno()) {
        return false;
    }

    /* Merge with previous item if seqno space is overlapped */
    if (next_elem != _seqno_space.begin()) {
        auto prev_elem = prev(next_elem);
        if (prev_elem->second >= start_seqno) {
            if (prev_elem->first < start_seqno) {
                start_seqno = prev_elem->first;
            }
            _seqno_space.erase(prev_elem->first);
        }
    }

    /* Merge with next item if seqno space is overlapped */
    if (next_elem != _seqno_space.end() && next_elem->first <= end_seqno) {
        if (next_elem->second > end_seqno) {
            end_seqno = next_elem->second;
        }
        _seqno_space.erase(next_elem->first);
    }

    /* Cut sequence space if it is over the first unacceptable sequence number */
    if (end_seqno > _first_unacceptable_seqno) {
        end_seqno = _first_unacceptable_seqno;
    }

    /* Insert segment's sequence space */
    _seqno_space.insert(make_pair(start_seqno, end_seqno));

    return true;
}

void TCPReceiver::segment_received(const TCPSegment &seg) {
    const TCPHeader &header = seg.header();
    const uint64_t length = seg.length_in_sequence_space();
    const uint64_t checkpoint = first_unassembled_seqno();

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
        _fin_seqno = unwrap(header.seqno + length, _isn, checkpoint);
    }

    /* Update sequence number space to keep track window and ackno */
    bool updated = update_seqno_space(header.seqno, length);
    if (!updated) return;

    /* Compute the stream_index of current segment */
    uint64_t stream_index;
    if (header.syn) {
        stream_index = 0;
    } else {
        stream_index = unwrap(header.seqno - 1, _isn, checkpoint);
    }

    /* Push payload to reassembler */
    _reassembler.push_substring(seg.payload().copy(), stream_index, header.fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (_syn_flag)
        return wrap(first_unassembled_seqno(), _isn);
    return nullopt;
}

size_t TCPReceiver::window_size() const { return _capacity - assembled_bytes(); }

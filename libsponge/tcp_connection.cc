#include "tcp_connection.hh"

#include <iostream>
#include <queue>
#include <optional>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPConnection::_abort_connection() {
    _receiver.stream_out().set_error();
    _sender.stream_in().set_error();
}

void TCPConnection::_send() {
    queue<TCPSegment> &sender_segments_out = _sender.segments_out();
    
    while (!sender_segments_out.empty()) {
        TCPSegment &segment = sender_segments_out.front();

        optional<WrappingInt32> ackno = _receiver.ackno();
        if (ackno.has_value()) {
            segment.header().ack = true;
            segment.header().ackno = * ackno;
        }

        segment.header().win = min(_receiver.window_size(), static_cast<size_t>(numeric_limits<uint16_t>::max()));

        _segments_out.push(segment);
        sender_segments_out.pop();
    }
}


void TCPConnection::_send_rst() {
    while(!_sender.segments_out().empty()) {
        _sender.segments_out().pop();
    }
    while(!_segments_out.empty()) {
        _segments_out.pop();
    }

    _sender.send_empty_segment();

    queue<TCPSegment> &sender_segments_out = _sender.segments_out();
    
    TCPSegment &segment = sender_segments_out.front();

    optional<WrappingInt32> ackno = _receiver.ackno();
    if (ackno.has_value()) {
        segment.header().ack = true;
        segment.header().ackno = * ackno;
    }
    segment.header().win = _receiver.window_size();
    segment.header().rst = true;

    _segments_out.push(segment);
    sender_segments_out.pop();

    _abort_connection();
}

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _time_since_last_segment_received; }

void TCPConnection::segment_received(const TCPSegment &seg) {
    const TCPHeader &header = seg.header();
    _time_since_last_segment_received = 0;

    if (header.rst) {
        _abort_connection();
        return;
    }

    _receiver.segment_received(seg);

    if (header.ack) {
        _sender.ack_received(header.ackno, header.win);
    }

    if (_receiver.ackno().has_value()) {
        _sender.fill_window();
        
        if (seg.length_in_sequence_space() != 0) {
            if (_sender.segments_out().empty()) {
                _sender.send_empty_segment();
            }
        }
    }

    if (_receiver.ackno().has_value()
        && (seg.length_in_sequence_space() == 0)
        && seg.header().seqno == _receiver.ackno().value() - 1) {
        _sender.send_empty_segment();
    }

    /* If FIN packet received before outbound reached EOF, linger false */
    if (_receiver.stream_out().eof() && !_sender.stream_in().eof()){ // && _sender.segments_out().empty() && _segments_out.empty())) {
        _linger_after_streams_finish = false;
    }

    _send();
}

bool TCPConnection::active() const {
    const ByteStream &stream_in = _sender.stream_in();
    const ByteStream &stream_out = _receiver.stream_out();


    return (!stream_in.error() && (!stream_in.eof() || bytes_in_flight() > 0)) || (!stream_out.error() && !stream_out.eof())
        || (_linger_after_streams_finish && stream_in.eof() && stream_out.eof());
}

size_t TCPConnection::write(const string &data) {
    size_t written_bytes = _sender.stream_in().write(data);
    _sender.fill_window();
    _send();
    return written_bytes;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    _sender.tick(ms_since_last_tick);
    _time_since_last_segment_received += ms_since_last_tick;

    if (_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS) {
        _send_rst();
    }
    if (_receiver.stream_out().eof()
    && _sender.stream_in().eof()
    && bytes_in_flight() == 0
    && _time_since_last_segment_received >= 10 * _cfg.rt_timeout) {
        _linger_after_streams_finish = false;
    }
    _send();
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    // send FIN packet
    _sender.fill_window();
    _send();
}

void TCPConnection::connect() {
    // send SYN packet
    _sender.fill_window();
    _send();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            _send_rst();
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}

#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , _timer{retx_timeout} {}

uint64_t TCPSender::bytes_in_flight() const { return _bytes_in_flight; }

void TCPSender::fill_window() {
    /*
     * Compute remaining window.
     * If receiver's window size is zero, act like the window size is one.
     */
    uint64_t remaining_window = (_window == 0 ? 1 : _window) - _bytes_in_flight;

    /* Repeatedly send segments until window is full */
    while (remaining_window > 0 && (_next_seqno == 0                         /* first segment */
                                    || !_stream.buffer_empty()               /* middle segments */
                                    || (_stream.input_ended() && !_fin_flag) /* last segment */
                                    )) {
        TCPSegment segment;

        /* Set seqno */
        segment.header().seqno = wrap(_next_seqno, _isn);

        /* Set SYN signal */
        if (_next_seqno == 0) {
            segment.header().syn = true;
            remaining_window -= 1;
        }

        /* Set payload */
        segment.payload() = Buffer(_stream.read(min(remaining_window, TCPConfig::MAX_PAYLOAD_SIZE)));
        remaining_window -= segment.payload().size();

        /* Set FIN signal */
        if (remaining_window > 0 && _stream.input_ended()) {
            segment.header().fin = true;
            remaining_window -= 1;
            _fin_flag = true;
        }

        /* Send and record segment for retransmission*/
        _segments_out.push(segment);
        _outgoing_segments.push(segment);
        _next_seqno += segment.length_in_sequence_space();
        _bytes_in_flight += segment.length_in_sequence_space();

        /* Run timer */
        _timer.run();
    };
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    /* Update window */
    _window = window_size;

    /* Compute absolute ackno */
    uint64_t ackno_absolute = unwrap(ackno, _isn, _next_seqno);

    /* Ignore impossible or duplicated ackno */
    if (ackno_absolute > _next_seqno || ackno_absolute <= _last_ackno)
        return;

    while (!_outgoing_segments.empty()) {
        /* Compute absolute seqno of buffered segment */
        TCPSegment &segment = _outgoing_segments.front();
        uint64_t segment_seqno = unwrap(segment.header().seqno, _isn, _next_seqno);
        uint64_t segment_end_seqno = segment_seqno + segment.length_in_sequence_space();
        uint64_t right_acked_seqno = min(ackno_absolute, segment_end_seqno);
        uint64_t left_acked_seqno = max(segment_seqno, _last_ackno);

        /* update _bytes_in_flight */
        _bytes_in_flight -= right_acked_seqno - left_acked_seqno;

        /* Stop when the segment is not fully acknowledged yet */
        if (right_acked_seqno != segment_end_seqno) {
            break;
        }

        /* Remove acknowledged segment and update _bytes_in_flight */
        _outgoing_segments.pop();
    }

    /* Update _window and _last_ackno */
    _last_ackno = ackno_absolute;

    /* Reset RTO */
    _timer.reset_all();

    /* If there's any in-flight segments, restart timer */
    if (!_outgoing_segments.empty()) {
        _timer.run();
    }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    _timer.tick(ms_since_last_tick);

    if (_timer.expired()) {
        /* Retransmit the earliest segment */
        TCPSegment &segment = _outgoing_segments.front();
        _segments_out.push(segment);

        /* If window is zero, double RTO for exponential backoff */
        if (_window != 0) {
            _timer.double_rto();
        }

        /* Reset and run timer */
        _timer.reset();
        _timer.run();
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _timer.consecutive_retransmissions(); }

void TCPSender::send_empty_segment() {
    TCPSegment segment;
    segment.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.push(segment);
}

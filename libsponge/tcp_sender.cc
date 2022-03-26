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
    /* Repeatedly send segments until window is full */
    while (_window > 0 && (_next_seqno == 0                         /* first segment */
                           || !_stream.buffer_empty()               /* middle segments */
                           || (_stream.input_ended() && !_fin_flag) /* last segment */
                           )) {
        TCPSegment segment;

        /* Set seqno */
        segment.header().seqno = wrap(_next_seqno, _isn);

        /* Set SYN signal */
        if (_next_seqno == 0) {
            segment.header().syn = true;
            _window -= 1;
        }

        /* Set payload */
        segment.payload() = Buffer(_stream.read(min(_window, TCPConfig::MAX_PAYLOAD_SIZE)));
        _window -= segment.payload().size();

        /* Set FIN signal */
        if (_window > 0 && _stream.input_ended()) {
            segment.header().fin = true;
            _window -= 1;
            _fin_flag = true;
        }

        /* Send segment */
        _segments_out.push(segment);
        _bytes_in_flight += segment.length_in_sequence_space();
        _next_seqno += segment.length_in_sequence_space();

        /* Record segment for resend */
        _outgoing_segments.push(segment);

        _timer.run();
    };
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    /* Update window */
    /* If window size is zero, act like the window size is one */
    if (window_size == 0) {
        _window = 1;
        _window_zero_flag = true;
    } else {
        _window = window_size;
        _window_zero_flag = false;
    }

    /* Compute absolute ackno */
    uint64_t ackno_absolute = unwrap(ackno, _isn, _next_seqno);

    /* Ignore impossible ackno */
    if (ackno_absolute > _next_seqno)
        return;

    while (!_outgoing_segments.empty()) {
        /* Compute absolute seqno of buffered segment */
        TCPSegment &front = _outgoing_segments.front();
        uint64_t front_seqno_length = front.length_in_sequence_space();
        uint64_t seqno_absolute = unwrap(front.header().seqno, _isn, _next_seqno);

        /* Stop when the segment is not acknowledged yet */
        if (seqno_absolute + front_seqno_length > ackno_absolute) {
            if (_ackno < ackno_absolute) {
                _bytes_in_flight -= (ackno_absolute - max(_ackno, seqno_absolute));
            }
            break;
        }

        /* Remove acknowledged segment and update _bytes_in_flight */
        if (_ackno > seqno_absolute) {
            _bytes_in_flight -= front_seqno_length - (_ackno - seqno_absolute);
        } else {
            _bytes_in_flight -= front_seqno_length;
        }
        _outgoing_segments.pop();
    }

    _window -= _bytes_in_flight;

    /* Stop the retransmission timer when all outstanding data has been acknowledged */
    if (ackno_absolute > _ackno) {
        _ackno = ackno_absolute;

        _timer.reset_all(); /* Reset RTO */
        if (!_outgoing_segments.empty()) {
            /* If there's any in-flight segments, restart timer */
            _timer.run();
        }
    }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    _timer.tick(ms_since_last_tick);

    if (_timer.expired()) {
        TCPSegment &front = _outgoing_segments.front();
        _segments_out.push(front);

        if (!_window_zero_flag) {
            _timer.double_rto();
        }
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

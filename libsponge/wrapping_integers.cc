#include "wrapping_integers.hh"

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    uint32_t result = n + isn.raw_value();
    return WrappingInt32{result};
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    uint32_t offset_n = n - isn;                         // Absolute seqno of n but with wrapping(32 bits)
    uint32_t offset_check = (checkpoint) % (1ll << 32);  // Absolute seqno of checkpoint but with wrapping(32 bits)
    uint64_t baseline = checkpoint - offset_check;       // The biggest multiple of 2^32 smaller than checkpoint
    uint64_t result = baseline + offset_n;

    /* Adjust result to be the closest value to checkpoint */
    if (offset_check > offset_n && offset_check - offset_n > (1ll << 31)) {
        result += 1ll << 32;
    } else if (offset_n > offset_check && checkpoint > (1ll << 31) && offset_n - offset_check > (1ll << 31)) {
        result -= 1ll << 32;
    }

    return {result};
}

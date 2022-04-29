#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();
    EthernetAddress *next_hop_ether = NULL;

    // find ethernet address matched to next_hop_ip
    for (auto iter = _address_mapping.begin(); iter != _address_mapping.end(); iter++) {
        if (get<1>(*iter) == next_hop_ip) {
            next_hop_ether = &get<2>(*iter);
        }
    }

    EthernetFrame frame;
    EthernetHeader &header = frame.header();
    header.src = _ethernet_address;

    if (next_hop_ether) {
        // destination ethernet address is already known. send it write away.
        header.type = EthernetHeader::TYPE_IPv4;
        header.dst = *next_hop_ether;
        frame.payload() = dgram.serialize();
    } else {
        // check if an ARP request about the same IP address is already sent in 5 seconds
        for (auto iter = _address_waiting.begin(); iter != _address_waiting.end(); iter++) {
            if (iter->second == next_hop_ip)
                return;
        }

        // broadcast an ARP request
        ARPMessage msg;
        msg.opcode = ARPMessage::OPCODE_REQUEST;
        msg.sender_ip_address = _ip_address.ipv4_numeric();
        msg.sender_ethernet_address = _ethernet_address;
        msg.target_ip_address = next_hop_ip;

        header.type = EthernetHeader::TYPE_ARP;
        header.dst = BROADCAST_ADDRESS;
        // header.dst = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        frame.payload() = msg.serialize();

        _datagram_queue.insert(make_pair(next_hop_ip, dgram));
        _address_waiting.push_back(make_pair(_current_tick + 5 * 1000, next_hop_ip));
    }

    _frames_out.push(frame);
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    EthernetHeader header = frame.header();

    if (header.type == EthernetHeader::TYPE_IPv4 && header.dst == _ethernet_address) {
        // parse the payload and return the resulting datagram
        InternetDatagram dgram;
        if (dgram.parse(frame.payload()) == ParseResult::NoError)
            return dgram;

    } else if (header.type == EthernetHeader::TYPE_ARP &&
               (header.dst == BROADCAST_ADDRESS || header.dst == _ethernet_address)) {
        // parse ARPMessage
        ARPMessage in_msg;
        if (in_msg.parse(frame.payload()) != ParseResult::NoError)
            return {};

        // remember the mapping between the sender's IP address and Ethernet address
        _address_mapping.push_back(
            make_tuple(_current_tick + 30 * 1000, in_msg.sender_ip_address, in_msg.sender_ethernet_address));

        // check unsent datagrams waiting for ethernet address
        auto range = _datagram_queue.equal_range(in_msg.sender_ip_address);
        for (auto iter = range.first; iter != range.second; iter++) {
            cout << "sending dgram after receiving ARP" << endl;
            EthernetFrame out_frame;
            EthernetHeader &out_header = out_frame.header();
            out_header.type = EthernetHeader::TYPE_IPv4;
            out_header.src = _ethernet_address;
            out_header.dst = in_msg.sender_ethernet_address;
            out_frame.payload() = iter->second.serialize();
            _frames_out.push(out_frame);
        }
        _datagram_queue.erase(in_msg.sender_ip_address);

        // if it’s an ARP request asking for our IP address, send an appropriate ARP reply.
        if (in_msg.opcode == ARPMessage::OPCODE_REQUEST && in_msg.target_ip_address == _ip_address.ipv4_numeric()) {
            ARPMessage out_msg;
            out_msg.opcode = ARPMessage::OPCODE_REPLY;
            out_msg.sender_ip_address = _ip_address.ipv4_numeric();
            out_msg.sender_ethernet_address = _ethernet_address;
            out_msg.target_ip_address = in_msg.sender_ip_address;
            out_msg.target_ethernet_address = in_msg.sender_ethernet_address;

            EthernetFrame out_frame;
            EthernetHeader &out_header = out_frame.header();
            out_header.type = EthernetHeader::TYPE_ARP;
            out_header.src = _ethernet_address;
            out_header.dst = in_msg.sender_ethernet_address;
            out_frame.payload() = out_msg.serialize();

            _frames_out.push(out_frame);
        }
    }

    return {};
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) {
    _current_tick += ms_since_last_tick;

    // mapping between ip address and ethernet address expires after 30 seconds
    while (!_address_mapping.empty()) {
        auto mapping = _address_mapping.front();
        if (get<0>(mapping) < _current_tick) {
            _address_mapping.pop_front();
        } else {
            break;
        }
    }

    // request for ethernet address expires after 5 seconds
    while (!_address_waiting.empty()) {
        auto waiting = _address_waiting.front();
        if (waiting.first < _current_tick) {
            _address_waiting.pop_front();
        } else {
            break;
        }
    }
}

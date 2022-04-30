Assignment 5 Writeup
=============

My name: Soomin Choi

My POVIS ID: choisium

My student ID (numeric): 20160169

This assignment took me about 3 hours to do (including the time on studying, designing, and writing the code).

If you used any part of best-submission codes, specify all the best-submission numbers that you used (e.g., 1, 2): -

- **Caution**: If you have no idea about above best-submission item, please refer the Assignment PDF for detailed description.

Program Structure and Design of the NetworkInterface:
# Private Member
추가한 private member는 다음과 같다.
1. `deque<tuple<size_t, uint32_t, EthernetAddress>> _address_mapping`: ip address에  
    대응되는 Ethernet address의 mapping을 저장한다. expiry time, ip address, Ethernet  
    address의 tuple을 저장한다.
    1. deque를 선택한 이유는 다음과 같다.
    2. 각 매핑은 30초 동안만 유효하고 이후엔 expire시켜야 하는데, 매핑이 쌓이고 사라지는 방식이  
        FIFO를 따르므로 queue의 동작을 지원하는 데이터 구조를 선택했다.
    3. 주어진 ip address에 대해 Ethernet address mapping이 존재하는지 확인하는 iteration을  
        수행해야 하므로, queue를 지원하면서도 iteration도 지원하는 deque를 선택했다.
2. `deque<pair<size_t, uint32_t>> _address_waiting`: 대응되는 Ethernet address를  
    몰라서 ARP request를 보낸 ip address의 리스트이다. expiry time과 ip address의 pair을  
    저장한다.
    1. deque를 선택한 이유는 다음과 같다.
    2. response가 오지 않은 ARP request에 대해 5초가 지난 후 expire시켜야 하는데, 이 동작은   
        FIFO에 해당해 queue의 동작을 지원하는 데이터 구조를 선택했다.
    3. 주어진 ip address에 대해 이미 ARP request를 보냈는지 확인하는 iteration을 수행해야  
        하므로, queue를 지원하면서도 iteration도 지원하는 deque를 선택했다.
3. `multimap<uint32_t, InternetDatagram> _datagram_queue`: 대응되는 Ethernet   
    address를 몰라서 ARP request를 보내고 pending하고 있는 datagram의 리스트이다. map의  
        value는 datagram이고, key는 해당 datagram의 next hop ip address이다.
    1. multimap을 선택한 이유는 다음과 같다.
    2. 해당 ip address에 대한 ARP response가 도착했을 때, 이를 기다리는 datagram을 찾는  
        동작에는 map이 적절하다고 판단했다.
    3. 하나의 ip address에 대한 여러 datagram이 존재할 수 있으므로 map 중에서도 multimap으로  
        선택했다.
4. `size_t _current_tick`: 0으로 초기화되며, `tick()` 함수를 통해 전달받은 지금까지 흐른  
    시간을 ms 단위로 저장한다.
5. `const EthernetAddress BROADCAST_ADDRESS{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}`:  
    ARPMessage를 broadcast할 때 사용하는 주소인 FF:FF:FF:FF:FF:FF를 상수로 선언한 것이다.


# Public Methods
주어진 public method에 대한 구현은 다음과 같다.
1. `send_datagram()`: upper layer로부터 datagram을 받았을 때 Ethernet address를 알고  
    있는지 여부에 따라 적절한 동작을 수행한다.
    1. `_address_mapping`을 순회하며, 인자로 받은 next hop ip address에 대한 Ethernet  
        address mapping(`next_hop_ether`)이 존재하는지 확인한다.
    2. `next_hop_ether`이 존재한다면 datagram을 담은 IPv4 frame을 전송한다.
        1. frame을 새로 만들고, type, src, dst를 적절하게 설정한다.
        2. frame의 payload를 인자로 받은 datagram으로 설정한다.
    3. `next_hop_ether`이 존재하지 않는다면 ARP request를 보낸다.
        1. `_address_waiting`을 순회하며, 동일한 ip address에 대한 element가 있는지  
            확인한다. 있다면 5초 안에 이미 ARP request를 보낸 적이 있는 것이므로 return한다.
        2. next hop ip address에 대한 Ethernet address를 요청하는 `ARPMessage`를 만든다.
        3. frame을 새로 만들고, type, src, dst를 적절하게 설정한다. dst는  
            `BROADCAST_ADDRESS`로 설정한다.
        4. frame의 payload를 만들어진 `ARPMessage`로 설정한다.
        5. 인자로 받은 datagram을 `_datagram_queue`에 저장한다.
        6. `_address_waiting`에 next hop ip address를 저장한다.
    4. 2번과 3번에서 만들어진 frame을 `_frames_out`에 푸시한다.
2. `recv_frame()`: frame을 받았을 때, 타입에 맞게 적절한 동작을 수행한다.
    1. 인자로 받은 frame이 IPv4 타입이고 frame의 dst가 `_ethernet_address`와 동일하면  
        다음의 동작을 수행한다.
        1. frame의 payload를 `InternetDatagram`로 파싱한다. 파싱이 성공하면 파싱한  
            datagram을 리턴한다.
    2. 인자로 받은 frame이 ARP 타입이고 frame의 dst가 `BROADCAST_ADDRESS`나  
        `_ethernet_address`와 동일하다면 다음의 동작을 수행한다.
        1. frame의 payload를 `ARPMessage`로 파싱한다. 파싱이 실패하면 리턴한다.
        2. `_address_mapping`에 tuple (`_current_tick` + 30초, sender ip address,  
            sender ethernet address)를 저장한다.
        3. `_datagram_queue`에서 sender ip address에 대한 mapping을 기다리고 있는  
            datagram을 순회하며, 적절한 IPv4 frame을 만들어 `_frames_out`에 푸시한다.
        4. 만약 `ARPMessage`의 opcode가 request이고 target ip address가  
            `_ip_address`와 일치한다면, 적절한 ARP reply frame을 만들어 `_frames_out`에 푸시한다.

3. `tick()`: 시간을 흘렀음을 안내받아 다음의 동작을 수행한다.
    1. `_current_tick`에 인자로 받은 `ms_since_last_tick`를 더해준다.
    2. `_address_mapping`을 앞에서부터 확인하며 expiry time(30초)이 지난 mapping을 삭제한다.
    3. `_address_waiting`을 앞에서부터 확인하며 ARP request를 보낸 ip address들 중 expiry  
        time(5초)이 지난 element들 삭제한다.

Implementation Challenges:  
- 적절한 자료 구조를 선정하는 것이 가장 고민되었다. 특히 ARP request를 5초 안에 이미 보냈다면 다시  
    보내지 않도록 하기 위해 데이터 구조(`_address_waiting`)를 새로 추가할지, 아니면 기존에 추가했던  
        데이터 구조(`_address_mapping`)를 활용할지 고민했다. 하지만 최종적으로는 구현하기  
            쉬우면서도 이해하기 쉬운 방식으로 결정해 새로운 데이터 구조를 추가하기로 결정하였다.  
- 자료 구조를 선정하는 것 외에 다른 어려움은 없었다. 매뉴얼을 따라 구현하니 어렵지 않게 구현할 수 있었다.

Remaining Bugs:  
없을 것이다. 모든 테스트를 통과하고 있다.

- Optional: I had unexpected difficulty with: -

- Optional: I think you could make this assignment better by: -

- Optional: I was surprised by: -

- Optional: I'm not sure about: -

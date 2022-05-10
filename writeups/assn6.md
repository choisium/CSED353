Assignment 6 Writeup
=============

My name: Soomin Choi

My POVIS ID: choisium

My student ID (numeric): 20160169

This assignment took me about 2 hours to do (including the time on studying, designing, and writing the code).

If you used any part of best-submission codes, specify all the best-submission numbers that you used (e.g., 1, 2): -

- **Caution**: If you have no idea about above best-submission item, please refer the Assignment PDF for detailed description.

Program Structure and Design of the Router:
# Data Structure
`add_route()` 함수를 통해 추가되는 route를 관리하기 위해 private member로 `_routing_table`  
을 추가했다.

`_routing_table`은 (`route_prefix`, `prefix_length`, `next_hop`, `interface_num`)  
의 4-tuple을 저장하는 deque이다. 각 route는 prefix_length의 descengding order로 저장된다.  
이는 `route_one_datagram()` 함수에서 longest-prefix match logic을 구현할 때 앞에서부터  
비교할 수 있도록 의도한 것이다.

# Logic of Implementation
구현한 method에 대한 설명은 다음과 같다.
1. `add_route()`: 인자로 들어온 route를 routing table에 추가한다.
    1. `_routing_table`을 돌면서 `prefix_length`의 내림차순에 맞게 route를 삽입할 위치를 찾는다.
    2. `_routing_table`의 해당 위치에 route를 삽입한다.
2. `route_one_datagram()`: 인자로 들어온 datagram을 적절한 interface로 route한다.
    1. `_routing_table`을 돌면서 longest-prefix-match route를 찾는다. `_routing_table`  
        가 `prefix_length`의 내림차순으로 정렬되어 있기 때문에 앞에서부터 돌면서 prefix와 일치하는   
        즉시 탐색을 멈춰도 된다.
    2. 매치되는 route가 존재하고, 인자로 받은 datagram의 ttl이 1보다 크면 아래의 동작을 수행한다.  
        그렇지 않으면 drop한다.
    3. datagram의 ttl을 1 감소시킨다.
    4. 매치된 route의 `next_hop`이 존재하지 않으면, datagram의 destination address를  
    `next_hop`으로 한다.
    5. 매치된 route의 interface에 대해 `send_datagram`을 호출하여 수정된 datagram을 전송한다.

Implementation Challenges:  
딱히 어려운 부분은 없었다.

Remaining Bugs:
없을 것이다. 모든 테스트를 통과하고 있다.

- Optional: I had unexpected difficulty with: -

- Optional: I think you could make this lab better by: -

- Optional: I was surprised by: -

- Optional: I'm not sure about: -

Assignment 4 Writeup
=============

My name: Soomin Choi

My POVIS ID: choisium

My student ID (numeric): 20160169

This assignment took me about 12.5 hours to do (including the time on studying, designing, and writing the code).

If you used any part of best-submission codes, specify all the best-submission numbers that you used (e.g., 1, 2): -

- **Caution**: If you have no idea about above best-submission item, please refer the Assignment PDF for detailed description.

Your benchmark results (without reordering, with reordering): [1.41, 1.53]

Program Structure and Design of the TCPConnection:
# Private member and methods
추가한 private member는 `_time_since_last_segment_received`로, 마지막으로 segment를  
전송받은 이후로 흐른 시간을 저장하며 0으로 초기화된다. `tick()` 함수를 통해 흐른 시간만큼을 더해주고  
`segment_received()` 함수에서 0으로 초기화된다.

추가하는 private methods는 아래와 같다.
1. `_abort_connection()`: inbound stream과 outbound stream의 `set_error()` 함수를  
    호출하여 error state로 설정한다. RST flag가 설정된 segment를 받거나 전송할 때  
    connection을 끊기 위해 사용된다.
2. `_send()`: TCPSender의 outbound stream에 쌓여 있는 segment들을 모두 pop하여 적절한 ack와  
    window 값을 설정하여 TCPConnection의 `_segments_out`으로 보낸다.
3. `_send_rst()`: RST flag를 설정한 TCP segment를 생성해 `_segments_out`으로 전송하고,  
    `_abort_connection()` 함수를 호출하여 error state로 설정한다.

# Public methods
## Wiring-up methods
하위 레벨의 함수들을 외부와 연결해주는 함수들로, TCPSender와 TCPReceiver에서 적절한 함수를 호출하여  
그 리턴값을 리턴한다.
1. `remaining_outbound_capacity()`: `_sender`의 outbound stream에 대하여  
    `remaining_capacity()`를 호출한다.
2. `bytes_in_flight()`: `_sender`의 `bytes_in_flight()` 함수를 호출한다.
3. `unassembled_bytes()`: `_receiver`의 `unassembled_bytes()` 함수를 호출한다.

## Writer methods
패킷을 전송하는 것과 관련된 함수들이다.
1. `connect()`: 처음 SYN packet을 보내는 함수이다. TCPSender의 `fill_window()` 함수를  
    호출하여 SYN packet을 생성하고 `_send()` 함수를 호출하여 해당 packet을 전송한다.
2. `write()`: 유저(application)로부터 데이터를 받아 해당 데이터를 담은 패킷을 보내는 함수이다.  
    TCPSender의 outbound stream에 인자로 받은 data를 write하고 `fill_window()` 함수를  
    호출하여 해당 데이터를 담은 packet을 생성한다. 이후 `_send()` 함수를 호출하여 해당 packet을  
    전송하고, outbound stream에 write한 bytes 수를 리턴한다.
3. `end_input_stream()`: 데이터 전송을 완료한 뒤 마지막으로 FIN packet을 보내는 함수이다.  
    TCPSender의 outbound stream의 `end_input()` 함수를 호출하여 데이터 전송이 끝났음을 알린다.  
    이후 TCPSender의 `fill_window()` 함수를 호출하여 FIN packet을 생성하고 `_send()` 함수를  
    호출하여 해당 packet을 전송한다.
4. `~TCPConnection()`: destructor가 호출되었을 때 active 상태이면 `_send_rst()` 함수를  
    호출하여 RST packet을 전송한다.

그 외 함수들은 다음과 같이 작성하였다.
1. `time_since_last_segment_received()`: private member인  
    `_time_since_last_segment_received`을 리턴한다.
2. `tick()`: 시간이 흘렀음을 안내받아 아래의 동작을 수행한다.
    1. TCPSender의 `tick()` 함수를 호출하여 시간이 흘렀음을 알린다. TCPSender에서는 타이머의  
    정보를 업데이트하고 필요한 retransmission을 진행한다.
    2. `_time_since_last_segment_received` 값을 업데이트한다.
    3. TCPSender의 `consecutive_retransmissions()` 함수를 호출하고, 이 값이  
    `TCPConfig::MAX_RETX_ATTEMPTS`보다 커지면 `_send_rst()` 함수를 호출하여 connection을  
    끊는다.
    4. 3번의 조건을 만족하지 못한다면, `_send()` 함수를 호출한다. 이는 1번에서 발생한  
    retransmission packet을 전송하기 위해서이다.
3. `segment_received()`: 새로운 segment가 도착했을 때 호출되어 아래의 동작을 수행한다.
    1. `_time_since_last_segment_received`를 0으로 초기화한다.
    2. 만약 RST flag가 설정되어 있다면, `_abort_connection()` 함수를 호출하고 종료한다.
    3. TCPReceiver의 `segment_received()` 함수에 segment를 전달하여 받은 데이터를 처리한다.
    4. 만약 ACK flag가 설정되어 있다면, TCPSender의 `ack_received()` 함수를 호출하여  
        ackno와 window 크기를 업데이트하도록 한다.
    5. 만약 connection이 잘 형성된 이후라면 다음의 동작을 수행한다. connection의 형성 여부는  
        TCPReceiver의 `ackno()` 함수가 nullopt가 아닌 실제 값을 리턴하는지로 확인할 수 있다.
        1. TCPSender의 `fill_window()` 함수를 호출하여 남은 window만큼을 채워 전송할  
        packet을 생성한다.
        2. 만약 전송할 packet이 없고, segment가 sequence number를 하나 이상 차지하거나  
        keep-alive segment에 해당한다면 TCPSender의 `send_empty_segment()` 함수를  
        호출하여 최소한 1개의 packet을 전송하도록 한다.
    6. 만약 FIN packet을 보내기 전에 inbound stream이 끝났다면  
        `_linger_after_streams_finish`를 false로 설정하여 passive close를 가능하도록 한다.
    7. `_send()` 함수를 호출하여 생성된 packet을 모두 전송한다.
4. `active()`: connection이 유효한지 여부를 리턴하며, 다음의 조건을 만족할 때 true를 리턴한다.
    1. Inbound stream과 outbound stream이 모두 error state가 아니다.
    2. Inbound stream이 아직 동작한다. 즉 아직 input_ended 상태가 아니거나 fully assembled  
        되지 않은 상태여야 한다.
    3. Outbound stream이 아직 동작한다. 즉 아직 eof에 도달하지 않았거나 전송 중인 bytes  
        (`bytes_in_flight`)가 있거나 fully acked된 상태가 아니어야 한다.
    4. Inbound stream이나 Outbound stream이 끝났더라도 lingering 중이면 active 상태이다.  
        `_linger_after_streams_finish`가 true이고, 마지막 segment를 받은 이후로 충분한  
        시간(`10 * _cfg.rt_timeout`)이 흐르는 동안 active 상태를 유지한다.


Implementation Challenges:  
구현에서 가장 어려움을 겪었던 부분은 하위 layer에서 버그가 존재하는 부분과 Timeout의 해결을 위해  
불필요한 연산을 없애는 부분이었다. 이를 해결하기 위해 주요 하위 class들의 리팩토링을 진행하였다. 아래는  
주요 변경 사항을 기록한 것이다.

# Stream Reassembler
- `push_nonoverlapped_substring()`에서 인자로 받은 data 중 first unassembled index  
    (코드에서의 명칭은 `next_assembled_index`)와 first unacceptable index 사이에 위치하는  
    data만 buffer에 저장하도록 했다. 이로 인해 `resolve_overflow()` 함수를 사용하지 않고도  
    window를 벗어나는 string을 처음부터 잘라낼 수 있도록 하여 불필요한 함수의 사용을 줄였다.

# TCP Receiver
- `update_seqno_space()` 함수를 거치지 않고도 동작하도록 수정하였다. 이 함수는 지금까지 받은  
    segment들이 커버하는 sequence space를 기록하여 first unassembled seqno를 찾기 위해  
    사용되었다. 하지만 inbound stream의 `bytes_written()` 함수로부터 쉽게 계산할 수 있다는  
    것에 착안하여 불필요한 함수의 사용을 줄였다.

# TCP Sender
- FIN signal을 설정하는 부분의 조건에서 outbound stream의 `input_ended()` 여부 대신  
    `eof()` 여부를 확인하도록 수정하였다. 이는 62번 이후의 테스트케이스에서 window가 클 때, 아직  
    buffer가 다 비워지지 않았는데 write는 끝난 상황에서 FIN flag를 같이 보내 문제가 됐었다.

Remaining Bugs: 없을 것이다. 모든 테스트를 다 통과하고 있다.

- Optional: I had unexpected difficulty with:
    - 하위 layer의 버그를 찾는 것이 가장 오래 걸렸다. TCPSender에서 FIN signal을 설정하는  
        조건을 확인할 때 input_ended()가 아닌 eof()를 호출하여 확인했어야 했는데, 이전 어싸인에서  
        모든 테스트 케이스가 통과해서 문제가 없다고 생각했다. 테스트 케이스만으로는 모든 버그를 커버할  
        수 없다는 것을 다시 한 번 느꼈다.
    - active 조건을 코드로 구현하는 부분이 헷갈렸다. 처음엔 간단할 줄 알고 한 줄로 작성하려다가  
        복잡하다는 걸 깨닫고는 여러 조건을 최대한 분리해서 동작을 명확히 하려고 했다.

- Optional: I think you could make this assignment better by: -

- Optional: I was surprised by: -

- Optional: I'm not sure about: -

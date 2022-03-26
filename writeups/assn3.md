Assignment 3 Writeup
=============

My name: Soomin Choi

My POVIS ID: choisium

My student ID (numeric): 20160169

<!-- 3/23 pm 7:30-8:30 to read document - 1h
    3/25 pm 3:30-7:12 to implement - 4 h
    3/25 pm 10:15-11:3 (1h) / 11:20-11:50 (30m) / 12:17-12:32 (-) / 12:43-1:52 (1h) /// 7.5 hours 
    
    Need to refactoring & write write-up
    3/27 am 1:00-4:00 /// 3 hours
    -->
This assignment took me about [n] hours to do (including the time on studying, designing, and writing the code).

If you used any part of best-submission codes, specify all the best-submission numbers that you used (e.g., 1, 2): -

- **Caution**: If you have no idea about above best-submission item, please refer the Assignment PDF for detailed description.

Program Structure and Design of the TCPSender:  
## Timer
TCPSender를 구현하기 위해 Helper class인 Timer class를 만들었다. Timer class를 이용하여 TCPSender에서 segment가 lost되었는지 판단하고 exponential backoff에 따라 segment를 재전송할 수 있다.  

### Private member
1. `_initial_retransmission_timeout`: RTO의 초기값을 저장한다.
2. `_retransmission_timeout`: RTO의 현재값을 저장한다.
3. `_consecutive_retransmissions`: 연속으로 재전송이 일어난 횟수를 기록한다.
4. `_elapsed_time`: segment를 전송한 이후 기존에 받지 않았던 acknowledgement를 받기까지 흐른 시간을 기록한다.
5. `_running`: timer가 현재 실행되고 있는지 여부를 저장한다.

### Public methods
1. 생성자: `_initial_retransmission_timeout`와 `_retransmission_timeout`을 인자로 받은 `retx_timeout`로 초기화한다.
2. `run()`: `_running`을 true로 설정한다.
3. `reset()`: RTO를 제외한 나머지 private member들을 초기화하는 함수로, `_running`을 false로 바꾸고, `_elapsed_time`을 0으로 설정한다. 
4. `reset_all()`: RTO를 포함한 대부분의 private member들을 초기화하는 함수로, `reset()` 함수를 호출한 뒤 `_retransmission_timeout`을 초기값으로 되돌리고 `_consecutive_retransmissions`를 0으로 설정한다.
5. `double_rto()`: `_retransmission_timeout`를 2배로 증가시키고 `_consecutive_retransmissions`를 1 증가시킨다.
6. `tick()`: `_running`이 true라면, `_elapsed_time`에 인자로 받은 `ms_since_last_tick`을 더한다.
7. `expired()`: timer가 expired 되었는지 여부를 리턴하는 함수로, `_elapsed_time`가 `_retransmission_timeout`보다 같거나 큰지를 리턴한다.
8. `consecutive_retransmissions()`: `_consecutive_retransmissions` 값을 리턴한다.

## TCPSender
### Added private members
1. `_window`: Receiver의 window의 크기를 저장한다. SYN flag가 set되어 있는 첫 segment를 전송하기 위해 초기값은 1로 설정된다.
2. `_outgoing_segments`: 전송했지만 아직 ack를 받지 못한 segment들을 저장한다. timeout이 발생하면 저장되어 있는 oldest segment부터 재전송된다.
3. `_bytes_in_flight`: 현재 `_outgoing_segments`에 저장되어 있는 segment들이 커버하는 seqno의 총 개수를 저장한다.
4. `_fin_flag`: FIN flag가 set된 segment의 최초 전송 여부를 저장한다.
5. `_last_ackno`: 지금까지 받았던 가장 큰 ackno를 저장한다. 새로운 acknowledge가 들어왔을 때 duplicate 여부를 판단하기 위해 선언하였다.
6. `_timer`: Timer class의 변수로, exponential backoff를 수행하거나 timeout 여부를 확인할 수 있다.

### Major public methods
멤버 변수을 거의 그대로 리턴하는 함수는 제외하였다.
1. `fill_window()`: Receiver의 window를 채울 때까지 `_stream`에서 데이터를 읽어와 segment들을 만들어 `_segments_out`에 추가한다.
    1. `window`에서 `_bytes_in_flight`를 뺀 값인 `remaining_window`를 계산한다. 이 때 만약 `_window`가 0이면 1로 계산한다.
    2. `remaining_window`가 0보다 크고, 다음의 조건 중 하나라도 만족하면 아래 동작을 수행한다.
        1. `_stream`이 비어있지 않을 때: 보낼 데이터가 있으면 segment를 전송한다.
        2. `_next_seqno`가 0일 때: 처음에는 `_stream`이 비어있더라도 SYN flag가 set된 segment를 전송한다.
        3. `_stream.input_ended()`가 true이고 `_fin_flag`가 false일 때: 모든 데이터를 전송했을 때, FIN flag가 set된 segment를 최소 한 번 전송한다.
    3. 새로운 segment를 만들어 필요한 정보를 저장한다.
        1. header의 seqno를 `_next_seqno`에 해당하는 값으로 업데이트한다.
        2. `_next_seqno`가 0이면 SYN flag를 set하고, `remaining_window`의 값을 1 감소시킨다.
        3. `_stream`으로부터 `remaining_window`와 `MAX_PAYLOAD_SIZE` 중 더 작은 값만큼 읽어와 payload에 저장하고, `_window`의 값을 payload의 길이만큼 감소시킨다.
        4. `remaining_window`에 공간이 남아있고 `_stream.input_ended()`가 true이면 FIN flag를 set하고 `remaining_window`의 값을 1 감소시키며, `_fin_flag`를 true로 설정한다.
    3. segment를 전송하고 타이머를 실행시킨다.
        1. `_segments_out`와 `_outgoing_segments`에 만든 segment를 push한다.
        2. `_bytes_in_flight`와 `_next_seqno`에 segment가 sequence space에서 차지하는 길이만큼 더해준다.
        3. `_timer.run()`을 통해 타이머를 실행시킨다.


2. `ack_received()`: 새로운 acknowledgement를 받았을 때 실행되는 함수로, `_window`를 업데이트하고 인자로 들어온 `ackno`에 따라 
    1. `_window`를 인자로 들어온 `window_size`로 업데이트한다.
    2. 인자로 들어온 `ackno`의 absolute 값 `ackno_absolute`를 구한다.
    3. 만약 `ackno_absolute`가 `_next_seqno`보다 큰 불가능한 값이거나 `_last_ackno`보다 작거나 같아 중복된 ACK를 나타내는 값이라면 무시한다(return).
    4. while문을 통해 아래의 동작을 반복하여 `_outgoing_segments`의 segment들 중 전체가 acknowledged 된 segment는 삭제한다.
        1. `_outgoing_segments`에서 가장 오래된 segment를 받아온다.
        2. segment의 absolute seqno를 계산하고, `_last_ackno`와 `ackno_absolute`로부터 해당 segment에서 새롭게 acknowledge된 가장 왼쪽 sequence number(`left_acked_seqno`)와 오른쪽 sequence number(`right_acked_seqno`)를 계산한다.
        3. `_bytes_in_flight`에서 새롭게 acknowledge된 sequence number만큼 뺀다(`right_acked_seqno - left_acked_seqno`).
        4. 만약 `right_acked_seqno`가 해당 segment의 마지막 sequence number + 1(`segment_end_seqno`)보다 작다면, 해당 segment 이후로는 더 이상 새롭게 acknowledge된 것이 없으므로 while문을 벗어난다.
        5. `_outgoing_segments`에서 pop을 통해 가장 오래된 segment를 없앤다.
    5. `_last_ackno`를 `ackno_absolute`로 업데이트한다.
    6. 새로운 ACK가 들어왔으므로 timer의 RTO를 리셋한다.
    7. 만약 `_outgoing_segments`에 segment가 아직 남아있다면 timer를 재시작한다.

3. `tick()`: 지난 tick() 함수 호출 이후로 몇 밀리초가 지났는지를 인자로 받으며, timeout을 확인하여 오래된 segment를 재전송한다.
    1. `_timer.tick()` 함수를 호출하여 `_elapsed_time`을 업데이트한다.
    2. `_timer.expired()`가 true이면 다음을 수행한다.
        1. `_outgoing_segments`에서 가장 초기 segment를 재전송한다.
        2. `_window`가 0이면 `_timer.double_rto()`를 통해 RTO를 2배로 증가시킨다.
        3. timer의 `_elapsed_time`을 리셋하고 재시작한다.

4. `send_empty_segment()`: sequence space에서 길이가 0인 TCPSegment를 만들고 전송한다. 이는 상위 component(TCPConnection)에서 empty ACK segment를 보낼 때 사용된다.
    1. 새로운 segment를 만들고, header의 seqno를 `_next_seqno`에 해당하는 값으로 업데이트한다.
    2. `_segments_out`에 만든 segment를 push하여 해당 segment를 전송한다.


Implementation Challenges:
[]

Remaining Bugs:
[]

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this assignment better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]

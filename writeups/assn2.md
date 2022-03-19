Assignment 2 Writeup
=============

My name: Soomin Choi

My POVIS ID: choisium

My student ID (numeric): 20160169

This assignment took me about 7.5 hours to do (including the time on studying, designing, and writing the code).

If you used any part of best-submission codes, specify all the best-submission numbers that you used (e.g., 1, 2): -

- **Caution**: If you have no idea about above best-submission item, please refer the Assignment PDF for detailed description.

Program Structure and Design of the TCPReceiver and wrap/unwrap routines:
# wrap/unwrap
## wrap
wrap의 경우, absolute sequence number `n`을 `isn`만큼 shifting되고 32-bit로 wrapping된  
related sequence number(WrappingInt32)로 바꿔야 한다. 이는 다음과 같은 순서로 계산하여 구할  
수 있다.
1. `n`에 `isn`을 더한다.
2. 1의 값을 uint32_t 변수(`result`)에 저장한다. 이렇게 되면 `n + isn`에 대한 값이 32-bit로  
wrapping이 되므로 이렇게 계산할 수 있다.

## unwrap
unwrap의 경우, initial sequence number `isn`에 대한 relative sequence number `n`을  
absolute sequence number `checkpoint`에 가장 가까운 absolute sequence number로  
바꿔야 한다. 이는 다음과 같은 순서로 계산하여 구할 수 있다.
1. 먼저 `checkpoint`보다 작은 2^32의 배수 중 가장 큰 값을 찾는다(`baseline`).
2. 이 `baseline`에 `n`과 `isn`의 차이(`offset_n`)를 더해 `checkpoint`에 가장 가까운 `n`의  
변환값 후보를 하나 찾는다(`result`).
3. 이후 `result`가 `checkpoint`에 가장 가까운 absolute sequence number인지 확인한다.  
기본적인 아이디어는 다음과 같다. `result`와 `result ± 2^32`이 모두 `checkpoint`에 가까운  
absolute sequence number가 될 수 있으며, 가장 가까운 absolute sequence number는  
`checkpoint`와의 차이가 2^31보다 작게 나올 것이다.  
    1. 만약 `checkpoint`가 `result`보다 더 큰 값이면, `checkpoint - result` 가 2^31보다  
    큰지 확인한다. 그렇다면 `result + 2^32`가 `checkpoint`에 더 가까운 값이다.  
    2. 만약 `checkpoint`가 `result`보다 더 작은 값이고, `result`가 2^32보다 크거나 같은  
    값이라면 `result - checkpoint` 가 2^31보다 큰지 확인한다. 그렇다면 `result - 2^32`가  
    `checkpoint`에 더 가까운 값이다. 

# TCP Receiver
## Data Structure
TCP Receiver에 추가한 private member는 다음과 같다.
1. `_syn_flag`: SYN flag가 설정된 segment의 도달 여부를 기록한다.
2. `_isn`: Initial Sequence Number를 기록한다.
3. `_fin_seqno`: FIN marker의 absolute sequence number를 기록한다.
4. `_seqno_space`: 지금까지 들어온 segments들이 커버하는 sequence number space를 기록한다.  
`_seqno_space`에 저장되는 element는 연속된 sequence number space별  
(첫번째 seqno, 마지막 seqno + 1)을 저장한다. 예를 들어, 강의 자료에 있는 예시의 경우  
_seqno_space에는 (0, 20), (24, 27), (29, 32) 가 저장된다.

## Logic
### Private Methods
1. `assembled_bytes()`: `stream_out().buffer_size()`를 리턴한다. 이는 현재 reassemble  
되어 아직 사용자가 읽어가지 않아 Byte Stream에 남아 있는 데이터의 길이(byte)를 의미한다.

2. `first_unassembled_seqno()`: 아직 reassemble되지 않은 첫번째 absolute sequence  
number를 리턴한다.
    1. `_seqno_space`가 비어있으면 0을 리턴한다.
    2. 그렇지 않으면 첫번째 pair의 second 값을 리턴한다. 현재 구현 상 첫번째 element의 key는  
    반드시 0이 되므로 그 value는 first unassembled absolute sequence number가 된다.

3. `first_unacceptable_seqno()`: capacity로 인해 처음으로 저장될 수 없는 absolute  
sequence number를 리턴한다.
    1. 먼저 `first_unassembled_seqno()`에 `window_size()`를 더한다(`_first_unacceptabel_seqno`).
    2. 만약 1에서 구한 값이 `_fin_seqno - 1`과 동일하다면, `_first_unacceptabel_seqno`를  
    `_fin_seqno`로 변경한다. 이는 FIN signal은 실질적인 데이터에 포함되지 않기 때문에 capacity에  
    무관하게 더해질 수 있기 때문이다.
    3. `_first_unacceptabel_seqno`를 리턴한다.

4. `update_seqno_space()`: 새롭게 받아온 segment가 커버하는 sequence space를 업데이트한다.
    1. 받아온 segment의 `start_seqno`와 `end_seqno`를 구한다.
    2. `_seqno_space`에서 해당 segment의 sequence space와 겹치는 element가 있는지 확인하여  
    다음의 동작을 수행한다.
        1. 겹치는 이전 sequence space element(`prev_elem`)이 존재한다면, `start_seqno` 
        를 그 자신과 `prev_elem`의 시작 seqno 중 더 작은 값으로 변경하고 `prev_elem`을  
        `_seqno_space`에서 삭제한다.
        2. 겹치는 다음 sequence space element(`next_elem`)이 존재한다면, `end_seqno`를  
        그 자신과 `next_elem`의 끝 seqno 중 더 큰 값으로 변경하고 `next_elem`을  
        `_seqno_space`에서 삭제한다.
    3. 만약 `end_seqno`가 first unacceptable absolute sequence number보다 큰 값이라면,  
    capacity를 넘는 부분이 저장될 수 없으므로 `end_seqno`를 first unacceptable absolute  
    sequence number으로 변경한다.
    4. `_seqno_space`에 `(start_seqno, end_seqno)` pair는 insert한다.

### Public Methods
1. `ackno()`: `_syn_flag`가 true이면 `first_unacceptable_seqno()`를 wrapping한 값을  
리턴하고, 그렇지 않으면 `nullopt`를 리턴한다.
2. `window_size()`: `_capacity`에서 `assembled_bytes()`  
를 뺀 값을 리턴한다.
3. `segment_received()`: 새로운 segment를 받아올 때마다 호출되는 함수이다.  
    1. segment의 header를 확인하여 SYN flag가 설정되어 있으면, `_syn_flag`를 true로 바꾸고  
    `_isn`에 header의 seqno를 기록한다. 
    2. 만약 _syn_flag가 false이면 segment를 무시한다. 이는 ISN을 알지 못하면 segment의 stream  
    index를 계산할 수 없기 때문이다.   
    3. FIN flag가 설정되어 있으면, `_fin_seqno`에 FIN marker의 absolute sequence number를  
    기록한다.
    4. `update_seqno_space()`를 호출하여 입력으로 들어온 segment의 sequence space를 기록한다.
    5. `_isn`, `header.seqno`를 사용하여 현재 segment의 시작    `stream_index`를 계산한다.
        1. 만약 SYN flag가 set되어 있다면 data가 `stream_index`를 0으로 한다.
        2. 그렇지 않다면 `stream_index`는 `header.seqno`를 `_isn`에 대한 relative  
        sequence number로 바꾼 값 - 1을 사용한다. 이 떄 `unwrap`에 사용하는 checkpoint는  
        `first_unassembled_index()`로 한다.
    6. `_reassembler.push_substring()`을 호출하여 `seg.payload()`를 Stream  
    Reassembler에 push한다. 이 때 EOF signal은 FIN signal이 set되어 있는지 여부로 준다.

Implementation Challenges:
- relative sequence number, absolute sequence number, stream index간의 변환이 헷갈렸다.  
수업 자료에 포함되었던 sequence number에 대한 diagram을 보고 wrap, unwrap을 구현하면서 이해했다고  
생각했지만 막상 TCP Receiver에서 stream index를 작성할 때 많이 헷갈렸다.
- window를 계산할 때 Assembled bytes를 어떻게 TCP Receiver로 가져올까 고민했다. Byte Stream  
에만 있는 정보인데, TCP Receiver의 stream_out() 함수를 사용해 값을 가져오도록 구현하였다. 
이를 사용하는 방식으로 설계했다.

Remaining Bugs: 없을 것이다. 모든 테스트를 다 통과하고 있다.

- Optional: I had unexpected difficulty with: unwrap을 짤 때, checkpoint로부터 가장  
가까운 sequence number를 찾는 것이 아니라 checkpoint보다 큰 것 중 가장 가까운 sequence  
number를 찾는 것으로 이해하고 짰었다. 이후 이를 알아채 다시 고치긴 했지만, 내가 문서를 조금 더 꼼꼼하게  
봤다면 큰 문제가 없었을 것이라는 생각이 들었다.

- Optional: I think you could make this assignment better by: 강의 자료에 포함되어 있던  
first unassembled sequence number, first unacceptable sequence number, window에 대한  
diagram이 assignment 문서에 포함되어 있으면 참고하기 더 수월할 것 같다.

- Optional: I was surprised by: 기존 public methods들의 interface를 해치지 않으면 다른  
public method를 추가해도 괜찮은 것으로 잘못 이해하고 있었다. 제출 전에 조교님께서 메일로 안내해주신  
덕분에 public interface를 건드리지 않는 방식으로 다시 구현할 수 있었다. 처음에 헷갈렸던 이유는 저번  
어싸인을 구현할 때 first unassembled seqno에 대한 정보를 Stream Reassembler에서 저장하도록  
구현해서 이를 활용하고자 했기 때문인 것 같다. 애초에 public interface가 수정이 불가능한 것을  
염두해두고, TCP Receiver에서 따로 트랙킹할 방법을 고안해야 했던 것 같다.

- Optional: I'm not sure about: window를 계산하기 위해 stream_out() 함수를 사용하여 Byte  
Stream의 assembled_bytes값을 정보를 가져오고 있는데, 이 방식이 abstraction을 깨는 것이 아닌지  
궁금하다. 오히려 Stream Reassembler에서 해당 값을 가져오는 함수를 작성하고 이 함수를 사용하는 것이  
abstraction을 지키는 것이라고 느껴졌다. assembled_bytes를 가져오지 않고도 window를 계산할 수  
있는 방법이 있는지 궁금하다.

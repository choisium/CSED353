Assignment 2 Writeup
=============

My name: Soomin Choi

My POVIS ID: choisium

My student ID (numeric): 20160169

This assignment took me about 5.5 hours to do (including the time on studying, designing, and writing the code).

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
3. `_ackno`: TCPReceiver가 지금까지 받지 못한 가장 첫 relative sequence number를 기록한다.

또한 Stream Reassembler의 private member인 `next_assembled_index`의 명칭을  
`_first_unassembled_index` 로 변경하였다. 기존에 이 변수가 저장하는 값이 first unassembled  
stream index이기 때문에 이를 더 잘 나타내는 이름으로 바꾸었다.

## Logic
Stream Reassembler에 TCP Receiver 구현에 필요한 정보를 얻어오기 위한 함수들을 일부 추가하였다.  
아래는 추가한 함수 및 구현한 TCP Receiver 함수의 로직을 설명한 것이다.

1. `StreamReassembler::first_unassembled_index()`: 아직 reassemble되지 않은 첫번째  
stream index를 리턴한다.
2. `StreamReassembler::assembled_bytes()`: `_output.buffer_size()`를 리턴한다. 이는  
현재 reassemble되어 아직 사용자가 읽어가지 않아 Byte Stream에 남아 있는 데이터의 길이(byte)를  
의미한다.
3. `StreamReassembler::eof()`: 모든 segment가 reassemble됐는지 여부를 리턴한다.
4. `TCPReceiver::ackno()`: `_syn_flag`가 true이면 `_ackno`를 리턴하고, 그렇지 않으면  
`nullopt`를 리턴한다.
5. `TCPReceiver::window_size()`: `_capacity`에서 `_reassembler.assembled_bytes()`  
를 뺀 값을 리턴한다.
6. `TCPReceiver::segment_received()`: 새로운 segment를 받아올 때마다 호출되는 함수이다.  
로직은 다음과 같다.
    1. segment의 header를 확인하여 SYN flag가 설정되어 있으면, `_syn_flag`를 true로 바꾸고  
    `_isn`에 header의 seqno를 기록한다.  
    2. 만약 _syn_flag가 false이면 segment를 무시한다. ISN을 알지 못하면 segment의 stream  
    index를 계산할 수 없다.  
    3. `_isn`, `header.seqno`를 사용하여 현재 segment의 시작    `stream_index`를 계산한다.
        1. 만약 SYN flag가 set되어 있다면 data가 `stream_index`를 0으로 한다.
        2. 그렇지 않다면 `stream_index`는 `header.seqno`를 `_isn`에 대한 relative  
        sequence number로 바꾼 값 - 1을 사용한다. 이 떄 `unwrap`에 사용하는 checkpoint는  
        `_reassembler.first_unassembled_index()`로 한다.
    4. `_reassembler.push_substring()`을 호출하여 `seg.payload()`를 Stream  
    Reassembler에 push한다. 이 때 EOF signal은 FIN signal이 set되어 있는지 여부로 준다.
    5. 마지막으로 `_ackno`를 업데이트한다. `_reassembler.first_unassembled_index()`를  
    `_isn`에 대해 wrap 하여 구한다. 만약 Reassembler가 마지막 segment까지 모두 reassemble  
    했다면 `_ackno`에 1을 더해준다.

Implementation Challenges:
- window를 계산할 때 Assembled bytes를 어떻게 TCP Receiver로 가져올까 고민했다. Byte Stream  
에만 있는 정보인데, TCP Receiver의 stream_out() 함수를 사용해 값을 가져오기엔 abstraction을  
깨는 것 같았다. 그래서 Stream Reassembler에 해당 값을 가져오는 함수(assembled_bytes)를 작성하여  
이를 사용하는 방식으로 설계했다.
- relative sequence number, absolute sequence number, stream index간의 변환이 헷갈렸다.  
수업 자료에 포함되었던 sequence number에 대한 diagram을 보고 wrap, unwrap을 구현하면서 이해했다고  
생각했지만 막상 TCP Receiver에서 stream index를 작성할 때 많이 헷갈렸다.

Remaining Bugs: 없을 것이다. 모든 테스트를 다 통과하고 있다.

- Optional: I had unexpected difficulty with: unwrap을 짤 때, checkpoint로부터 가장  
가까운 sequence number를 찾는 것이 아니라 checkpoint보다 큰 것 중 가장 가까운 sequence  
number를 찾는 것으로 이해하고 짰었다. 이후 이를 알아채 다시 고치긴 했지만, 내가 문서를 조금 더 꼼꼼하게  
봤다면 큰 문제가 없었을 것이라는 생각이 들었다.

- Optional: I think you could make this assignment better by: 강의 자료에 포함되어 있던  
first unassembled sequence number, first unacceptable sequence number, window에 대한  
diagram이 assignment 문서에 포함되어 있으면 참고하기 더 수월할 것 같다.

- Optional: I was surprised by: -

- Optional: I'm not sure about: -

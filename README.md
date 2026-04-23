# UDP Reliability - Intro

This is my introductory networking project, used to learn the basics of network programming.
It includes a sender and a receiver app, sending messages via UDP. 
It implements a reliability layer with stop-and-wait ARQ, sequence numbers, ACKs, and retransmits.

## What It Does

The project contains code for a sender app, a receiver app, and the definition for the protocol header. 
The two apps connect via Windows sockets, and communicate by sending UDP datagrams.
The receiver waits for UDP datagrams containing messages from the sender, 
printing any received messages and responding to the sender with "ACK" packets. 
Both apps simulate packet drops and handle packet drop scenarios. 
Both apps output information about sent and received packets, as well as any packet drops that they encounter.

## Building

Requires Windows with MSVC (Visual Studio 2022 recommended) and CMake 3.10+. 
Run from a Developer Command Prompt or any terminal with the VS environment initialized.

```bash
cmake --preset x64-debug
cmake --build out/build/x64-debug
```

For a release build:

```bash
cmake --preset x64-release
cmake --build out/build/x64-release
```

Available presets: `x64-debug`, `x64-release`, `x86-debug`, `x86-release`.

## Running

Start the receiver first, then the sender — the receiver must be listening before the sender connects.

```bash
./out/build/x64-debug/receiver.exe
./out/build/x64-debug/sender.exe
```

## Design Notes

### API/Implementation Choices:

- Chose Windows sockets for simplicity and will use POSIX with abstraction in followups to cover bases.
- Chose UDP because I wanted hands on learning about the reliability layer.
- Chose 508 bytes as maximum datagram size to avoid fragmentation in worst-case MTU paths. 
  576 (min IPv4 MTU) - 60 (max IP header) - 8 (UDP header) = 508
- Kept variable initialization outside of send/receive loops to retain last buffer for duplicate detection.

### Scope:

I intentionally scoped the project to be very small, keeping it introductory in nature.
The following is a list of implementations/techniques used here and planned improvements in my followup:
- stop-and-wait ARQ -> sliding window
- simplex messaging -> full-duplex messaging
- localhost connections only -> connections from anywhere

I will need to learn and implement techniques such as NAT traversal in order to communicate across networks.

## What I'd Do Differently

- The project took a lot less time than expected. Erring on the safe side, I cut scope from half-duplex communication to simplex. Looking back, I probably could have achieved it.
- I leaned on references too early when stuck. Whenever I reasoned through a problem first, I usually got close. I should have trusted that process more often, even when I was concerned about time.
- I wish I had pulled out the buffer construction and datagram send into a helper function — it would have required careful thought about which variables to pass in and when the buffer gets updated relative to the duplicate detection logic.
- I used a goto to exit the retransmission nested loop, which is a bit of a shortcut. I would have refactored the retransmission logic into a helper function instead.

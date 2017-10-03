# `pe2` folder

Protocol execution.

## Usage

On two Linux machines, run the following commands:

```bash
git clone https://github.com/GeeLaw/vecole-redux.git
cd vecole-redux/code/pe2
chmod +x *.sh
./compile.sh
```

Then, on **one** machine, run `./datagen`, then copy `a`, `b`, `x`, `stdans` to the other machine (the corresponding folder).

On one machine, whose IP address, let’s suppose, is `10.100.0.1`, run `./alice.sh`. On the other machine, run `./bob.sh 10.100.0.1`. After a while, on the first machine, run `diff --report-identical-files ao stdans` to confirm that the answer is correct.

Optionally, you can use `./alice.sh 5` with `./bob.sh 10.100.0.1 5` to perform 5 rounds. Other numbers follow the pattern, of course.

Note that the first time you play Alice or Bob in a while, you might be asked for your password (or the password of an administrator) because the shell scripts use (`sudo`ne) `nice` to decrease the niceness so that protocol execution preempts other code on the machine in the hope that the statistics faithfully reflect the performance of the two agents.

## Overview

In one execution of the batch-OLE protocol, Alice and Bob uses 3 concurrent socket connections. In the implementation, Alice is the servers and Bob is the clients.

In the following description, steps with the same number are parallelisable.

### Alice’s steps

| Step number | Description |
| :---------: | ----------- |
| 1 | Select random `s[i]`. |
| 2 | Garbled computation:  |
| 2.a | Receive Bob’s keys (with connection 1). |
| 2.a | Receive her keys from Bob with vector OLE (with connection 2). |
| 2.b | Use the decoder to find `u[i]=a[i]y[i]+c[i]`. |
| 2 | Eliminate the cryotographic blinding: |
| 2.a | Send `D[i]=x[i]-y[i]` to Bob (with connection 3). |
| 2.b | Receive `v[i]=a[i]D[i]+b[i]-c[i]` from Bob (with connection 3). |
| 3 | Compute batch-OLE with `z[i]=a[i]x[i]+b[i]=u[i]+v[i]`. |

### Bob’s steps

| Step number | Description |
| :---------: | ----------- |
| 1 | Select random `c[i]`. |
| 1 | Create key pairs from DARE encoder. |
| 2 | Garbled computation : |
| 2.a.i | Compute his keys with `a[i]` and `c[i]`. |
| 2.a.ii | Send his keys to Alice (with connection 1). |
| 2.a | Send Alice’s keys with vector OLE (with connection 2). |
| 2 | Eliminate the cryptographic blinding: |
| 2.a | Receive `D[i]=x[i]-y[i]` from Alice (with connection 3). |
| 2.b | Send `v[i]=a[i]D[i]+b[i]-c[i]` to Alice (with connection 3). |

### Considerations for larger fields

It is possible to further optimize the process for several send-receive pattern. For example, in the “eliminate the cryptographic blinding” process, computing and sending of a certain `v[i]` can be done immediately after the receiving of `D[i]` instead of after waiting for all `v[i]`s to arrive.

Moreover, vector OLEs can be parallelised so that the batches are performed concurrently. This should significantly improve performance.

## Communication specifications

The sockets use TCP/IP and disables Nagle’s algorithm so that short messages (those fixed messages) get delivered immediately.

### Fixed messages

All fixed messages are of 8 bytes and are in **machine endianness**.

- Ping: `0x42de0135245310ed`.
- Pong: `0x4201356738573920`.
- Hello: `0x4242424242424242`. (The answer to life, universe and everything.)
- ByeBye: `0x8888888888888888`. (In Chinese, 88 is phonetically similar to bye-bye.)
- Success: `0x6666666666666666`. (In Chinese, 6 is phonetically similar to “溜”, which, when used as an adjective or an adverb, means *to be/perform good at*)
- Fail: `0x0000000000000000`.

### Establishing the connections

A ping-pong with endianness checking is performed to establish the connection.

1. The client sends **Ping** to the server.
2. The server receives the message and reinterprets it as `uint64_t`.
3. If on the server, the memory is *not* interpreted as the value of Ping, the server aborts the connection. This checks the two machines has the same endianness so that endianness conversion is unnecessary.
4. Otherwise, the server sends **Pong** to the client and the connection is established.

### Connection 1: transfer Bob’s keys

1. Bob sends **Hello**.
2. Bob sends the concatenated memory representation of the keys.
3. Bob sends **ByeBye**.

### Connection 2: transfer Alice’s keys with vector OLE

1. Bob sends **Hello**.
2. Bob sends the memory representation of `E(r,k1[t...])+e` to Alice.
3. Alice sends `s[i]*E(r,k1[t...])+E(r',k')` (two copies to simulate OT) to Bob.
4. If decoding is successful, Bob sends **Success**, then the memory representation of `s[i]k1[t...]+k'+k2[t...]` to Alice. Otherwise, Bob sends **Fail** and redoes the current round by going back to step 2 (with the current batch).
5. Alice finds `s[i]k[t...]+k2[t...]` by subtracting `k'`.
6. If there are still keys for further computation, go back to step 2 (with the new batch). Otherwise, Bob sends **ByeBye**.

### Connection 3: eliminate cryptographic blinding

1. Alice sends **Hello**.
2. Alice sends the memory representation of `x[i]-y[i]` to Bob.
3. Bob sends the memory representation of `v[i]=a[i]*(x[i]-y[i])+b[i]-c[i]` to Alice.
4. Bob sends **ByeBye**. (N.B. Bob sends **ByeBye** while Alice sent **Hello**! The rule here is that **Hello** is sent by the first message sender, and that **ByeBye** is sent by the last message sender.)

## Command line parameters

```
pe2 { alice | ipv4 }
    port1 port2 port3
    luby sparse prg
    { x | a b } count
```

- `alice`: literal string `alice`, runs the program as Alice.
- `ipv4`: an IPv4 address where Alice is located, runs the program as Bob.
- `port1`, `port2` and `port3`: 3 different ports through which the two agents communicate.
- `luby`: the file name of Luby code.
- `sparse`: the file name of sparse linear code.
- `prg`: the file name of Goldreich’s function.
- `x`: the file name of Alice’s input.
- `a` and `b`: the file names of Bob’s inputs.
- `count`: the number of batches to execute.

## Files

| File name | Meaning | Description |
| :-------: | ------- | ----------- |
| `pe2.cpp` | Protocol Execution version 2 | The main program. This is the file that should be compiled. Version 2 means that it uses DARE on-the-fly, instead of generating the encoding/decoding circuit in advance, which proves to be slow and space-occupying. |
| `datagen.cpp` | Data Generation | Simple command to generate data. |
| `pch.hpp` | Pre-compiled Header | The `include`s for the main program. || `common.hpp` | Common utilities | Implements some common utilities, included by the main program before `alice.hpp` and `bob.hpp`. |
| `alice.hpp` | Alice | Implements the role of Alice, included by the main program. |
| `bob.hpp` | Bob | Implements the role of Bob, included by the main program. |
| `sparse` | Sparse code file | Generated by example program `sparsegen`. |
| `luby` | LT code file | Generated by example program `ltgen`. |
| `prg` | Goldreich’s function | Generated by example program `goldgen`. |
| `gen.bat` | Data Generation | A handy tool that compiles `datagen.cpp` and generates data on Windows. |
| `test.bat` | Test | A handy tool that compiles `pe2.cpp` and runs two batches of OLEs on Windows. The tool will run both Alice and Bob on the same machine, talking via local loopback. On the first run, you might be notified by Windows Firewall and have to allow the program through. |
| `compile.sh` | Compile | Compiles `pe2.cpp` and `datagen.cpp` on Linux. |
| `alice.sh` | Alice | Plays Alice on Linux. |
| `bob.sh` | Bob | Plays Bob on Linux. |

**Important** You should not use `autocrlf` for Git, which might cause `.bat` to be checked out with LF line endings, resulting undefined behaviour for Command Prompt.

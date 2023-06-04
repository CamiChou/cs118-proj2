# CS118 Project 2

This is the repo for spring23 cs118 project 2.
The Docker environment has the same setting with project 0.

## Academic Integrity Note

You are encouraged to host your code in private repositories on [GitHub](https://github.com/), [GitLab](https://gitlab.com), or other places. At the same time, you are PROHIBITED to make your code for the class project public during the class or any time after the class. If you do so, you will be violating academic honestly policy that you have signed, as well as the student code of conduct and be subject to serious sanctions.

## Provided Files

- `project` is the folder to develop codes for future projects.
- `grader` contains an autograder for you to test your program.
- `scenarios` contains test cases and inputs to your program.
- `docker-compose.yaml` and `Dockerfile` are files configuring the containers.

## Docker bash commands

```bash
# Setup the container(s) (make setup)
docker compose up -d

# Bash into the container (make shell)
docker compose exec node1 bash

# Remove container(s) and the Docker image (make clean)
docker compose down -v --rmi all --remove-orphans
```

## Environment

- OS: ubuntu 22.04
- IP: 192.168.10.225. NOT accessible from the host machine.
- Files in this repo are in the `/project` folder. That means, `server.cpp` is `/project/project/server.cpp` in the container.
  - When submission, `server.cpp` should be `project/server.cpp` in the `.zip` file.

## Project 2 specific

### How to use the test script

To test your program with the provided checker, go to the root folder of the repo and
run `python3 grader/executor.py <path-to-server> <path-to-scenario-file>`.  
For example, to run the first given test case, run the following command:

```bash
python3 grader/executor.py project/server scenarios/setting1.json
# Passed check point 1
# Passed check point 2
# OK
```

If your program passes the test, the last line of output will be `OK`.
Otherwise, the first unexpect/missing packet will be printed in hex.
Your program's output to `stdout` and `stderr` will be saved to `stdout.txt` and `stderr.txt`, respectively.
You can use these log files to help you debug your router implementation.
You can also read `executor.py` and modify it (like add extra outputs) to help you.
We will not use the grader in your submitted repo for grading.

### How to write a test scenario

A test scenario is written in a JSON file. There are 5 example test cases in the `scenarios` folder.
The fields of the JSON file are:

- `$schema`: Specify the JSON schema file so your text editor can help you validate the format.
  Should always point to `setting_schema.json`.
- `input`: Specify the input file to the program. Should use relative path to the JSON file.
- `actions`: A list of actions taken in the test scenario. There are 3 types of actions:
  - `send`: Send a TCP/UDP packet at a specified port (`port`).
  - `expect`: Expect to receive a TCP/UDP packet at a specified port (`port`).
  - `check`: Delay for some time for your server to process (`delay`, in seconds).
    Then, check if all expectations are satisfied.
    All packets received since the last checkpoint must be exactly the same as specified in `expect` instructions.
    There should be no unexpected or missing packets
  - The last action of `actions` must be `check`.
- The fields of a packet include:
  - `port`: The ID of the router port to send/receive the packet, not the port number.
    The port numbers are specified in `src_port` and `dst_port`.
  - `src_ip` and `src_port`: The source IP address and port number.
  - `dst_ip` and `dst_port`: The destination IP address and port number.
  - `proto`: The transport layer protocol. Can only be `tcp` or `udp`.
  - `payload`: The application layer payload of the packet. Must be a string.
  - `ttl`: Hop limit of the packet.
  - `seq`: TCP sequence number.
  - `ack`: TCP acknowledge number.
  - `flag`: The flag field in TCP header. Should be specified in numbers. For example, ACK should be `16`.
  - `rwnd`: TCP flow control window.
  - `ip_options_b64`: The IP options. Must be encoded in base64 if specified.
  - `ip_checksum`: The checksum for an IP packet. Automatically computed to be the correct number if not specified.
  - `trans_checksum`: The checksum in the TCP/UDP header. Automatically computed to be the correct number if not specified.
  - Most of these fields are optional, but omitting mandatory fields may crash the grader.

Please read the example JSON files and the schema JSON for details.

### How to examine a test scenario

To print all packets in a test scenario in hex format,
run `python3 grader/packet_generate.py` and input the JSON setting.
You may also use `<` to redirect the input to the JSON file, like

```bash
python3 grader/packet_generate.py < scenarios/setting1.json
# ================== SEND @@ 01 ==================
# 45 00 00 1c 00 00 40 00  40 11 b6 54 c0 a8 01 64
# c0 a8 01 c8 13 88 17 70  00 08 50 69
# ================== ========== ==================
#
# ================== RECV @@ 02 ==================
# 45 00 00 1c 00 00 40 00  3f 11 b7 54 c0 a8 01 64
# c0 a8 01 c8 13 88 17 70  00 08 50 69
# ================== ========== ==================
#
# Check point 1
#
# ================== SEND @@ 01 ==================
# 46 00 00 20 00 00 40 00  40 11 b4 4f c0 a8 01 64
# c0 a8 01 c8 01 01 00 00  13 88 17 70 00 08 50 69
# ================== ========== ==================
#
# ================== RECV @@ 02 ==================
# 46 00 00 20 00 00 40 00  3f 11 b5 4f c0 a8 01 64
# c0 a8 01 c8 01 01 00 00  13 88 17 70 00 08 50 69
# ================== ========== ==================
#
# Check point 2
#

```

### Other notes

- We will use a different version of grader for the final test to integrate with Gradescope.
  But it will be similar to the given one.
  Modifying the grader in this repo will not affect anything.
- We will include many hidden test cases in the final test. Do not fully depend on the 5 given ones.
  They do not cover all edge cases that we want to test.
- The autograder will only build your program in the `project` folder, and grade the built `server` executable.
  Your program should not depend on other files to run.

## TODO

Team Members and UIDs:

Camille Chou (905707606)
Jason Inaba (905539383)
Ollie Pai (305718387)

High level Design:

On a high level, our router performs Network Address and Port Translation (NAPT) to translate LAN IP addresses and ports to a WAN IP address and ports, and vice versa in the language of C++. It handles incoming packets, performs checks and translations, and forwards the packets to the appropriate destination. The code consists of outside classes and files that we added to the main.cpp which were the config_parser.cpp/config_parser.h files which parsed the .txt files that we were given in order to properly store the information regarding napt translations, IP headers in proper formats such as maps, structs, and classes.

The main function initializes the router by parsing a configuration file using the outside classes. It sets up the necessary data structures, such as the mappings between LAN and WAN IP addresses and ports. It also creates a server socket and listens for incoming connections.

When a client connects to the router, a new thread is created to handle the client. The handle_client function is responsible for receiving and processing packets from the client. It starts by checking the IP checksum and TTL (Time to Live) of the packet. If the checksum or TTL is invalid, the packet is dropped. The function also takes in account if the packet ois traveling LAN to LAN, LAN to WAN, or WAN to LAN and processes accordingly via the NAPT table translations (two maps). It performs dynamic NAPTt trasnlation to ensure it can rapid update.

After the translation is performed, the function recomputes the IP and transport layer checksums and sends the packet to the destination using a PseudoHeader. The process continues until all packets from the client are processed, and then the client socket is closed.

The problems you ran into and how you solved the problems:

We ran into several problems when creating this router. For one, it took us awhile to realize how after NAPT translations it was necessary to recompute a checksum using a pseudoheader. That took a lot of reserch to figure out and scouring documentation that led us to realize how exactly pseudoheaders worked and how to compute the checksum using them. The data parsing also was a bit of a learning curve to figure out which structures and information to extract. This was done via a lot of trial and error. A final problem that we had was that the Gradescope tcp offset was assignned incorrectl, mainly the size. This was really messing up the static NAPT table and stress tests and it wasn't until we asked the TA about insight on the Gradescope case were we finally able to achieve a full score.

Acknowledgement of any online tutorials or code examples (except class website) you have been using.

We used some information from the internet to guide our understanding of the router, how it worked and all its components. A lot of diagrams from wiki pages and the textbook regarding IP, UDP, and TCP header were used, especially when it came down to distinguishing how many bits and bytes of data were allocated for each section. Furthermore, some TA slides we found helpful for getting started and understanding the concepts and high level design.

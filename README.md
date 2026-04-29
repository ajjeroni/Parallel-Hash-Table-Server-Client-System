# Parallel Hash Table Server-Client System

Adan Jeronimo - ajjeroni@csu.fullerton.edu\
Ryan Franson - ryanfranson@csu.fullerton.edu\
Aaron Davila - \
Gary Samuel -  

## Overview
This project implements a **multi-threaded server-client system** using a **parallel hash table**. The goal is to explore concepts such as threading, mutexes, synchronization, and message passing in a real-world database access scenario. 

The server handles multiple client requests concurrently while ensuring safe access to shared data structures.

---

## How to Execute the Program

### 1. Compile the Programs
Make sure you are in the project directory, then run:

```bash
make
```
This will compile both the server and client programs.

### 2. Start the Server
Run the server with:

```bash
./server namesDB.txt <NUMBER_OF_THREADS>
```
- namesDB.txt → input file containing records (id, first name, last name)
- <NUMBER_OF_THREADS> → number of worker threads handling requests

### 3. Start the Client 
In a seperate terminal, run:

```bash
./client
```
The client will:
- Generate random IDs
- Send requests to the server
- Receive and print matching records (if found)

### 4. Expected Output
Example client output:

```bash
messageType=2 id=46 firstName=Daniel lastName=Ariza
messageType=2 id=62 firstName=Michael lastName=Busslinger
messageType=2 id=31 firstName=Tommy lastName=Chao
```

- If a record does not exist, the server returns id = -1

### 5. Stop the Program
Press:

```bash
ctrl + c
```

- This should trigger cleanup (message queue removal, thread termination)

## Implemented Extra Credit ✅

## Features
- Multi-threaded server using pthreads
- Parallel hash table with fine-grained locking (mutex per cell)
- Client-server communication using System V message queues
- Concurrent record insertion and retrieval
- Protection against race conditions and deadlocks

---

## Technologies Used
- **Language:** C++
- **Libraries:**
  - pthread (for threading)
  - STL (vector, list)
  - System V message queues

---

## How It Works

### Server
- Loads records from a file into a hash table
- Creates:
  - Fetcher threads (handle client requests)
  - Inserter threads (simulate database updates)
- Uses mutexes to protect individual hash table cells
- Communicates with clients via message queues

### Client
- Sends random record ID requests to the server
- Waits for responses
- Prints retrieved records

---

## Hash Table Design
- Fixed size: 100 cells
- Each cell contains:
  - A linked list of records
  - A mutex lock
- Hash function:
  - index = record_id % 100


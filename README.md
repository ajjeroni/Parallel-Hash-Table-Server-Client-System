# Parallel Hash Table Server-Client System

Adan Jeronimo - ajjeroni@csu.fullerton.edu\
Ryan Franson - \
Aaron Davila - \
Gary Samuel -  

## Overview
This project implements a **multi-threaded server-client system** using a **parallel hash table**. The goal is to explore concepts such as threading, mutexes, synchronization, and message passing in a real-world database access scenario. 

The server handles multiple client requests concurrently while ensuring safe access to shared data structures.

---

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

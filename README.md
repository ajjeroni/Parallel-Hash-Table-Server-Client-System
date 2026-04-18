# Parallel Hash Table Server-Client System

## Team
- Adan Jeronimo - ajjeroni@csu.fullerton.edu
- Ryan Franson
- Aaron Davila
- Gary Samuel

## Language
C++

## How To Build
Run this project on Linux.

```sh
make
```

## How To Run
Start the server first and then run one or more clients in separate terminals.

```sh
./server namesDB.txt 10
./client
```

## Extra Credit
Implemented.

The server now uses a condition-variable-based thread pool, and each client creates a private
reply queue so multiple clients can run at the same time without reading each other's results.

## Notes
- Press `Ctrl-C` in the server terminal to stop the server and remove the shared request queue.
- Press `Ctrl-C` in a client terminal to remove that client's private reply queue.

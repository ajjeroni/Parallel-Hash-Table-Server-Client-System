Parallel Hash Table Server Client System

Names:
Adan Jeronimo - ajjeroni@csu.fullerton.edu
Ryan Franson
Aaron Davila
Gary Samuel

Language:
C++

How to compile:
Use Linux and run:

```sh
make
```

How to run:

```sh
./server namesDB.txt 10
./client
```

Extra credit:
Yes. The fetcher threads use a condition variable so they sleep until there is work.
I also made it so more than one client can run at the same time by giving each client its own
reply queue.

Notes:
- Run the server first.
- Press Ctrl-C on the server to remove the main queue.
- Press Ctrl-C on a client to remove that client's private queue.

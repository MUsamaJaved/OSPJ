## How to perform performance measurement on `fibo`

Run the similar command inside QEMU:

    for n in `seq 1 5`; do (time -v fibo -q 10000000 2>&1 | grep seconds); done

Run the `./clean.rb` and paste the output. Then press `Ctrl+D`. You'll see the
following output:

    ...
    Socket messages received: 0
    Signals delivered: 0
    Page size (bytes): 4096
    Exit status: 0
    ===========================================================================
    0.55    0.45    0.45    0.45    0.45    
    0.05    0.0     0.01    0.02    0.01    

First row is user time, second - system time.

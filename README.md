## PSRS-Parallel-Sort
Implementation of the PSRS Parallel Sort Algorithm (C)

# Compilation:
```
gcc -Wall -Wextra psrs.c -lm
```

# Running:

Single trial (mode 1)
```
./a.out NUM_THREADS 1
```

Experiment mode (mode 2)
```
./a.out NUM_THREADS 2 MAX_ARRAY_SIZE
```
Note: minimum array size is 1M and increases exponentially by a power of 2.
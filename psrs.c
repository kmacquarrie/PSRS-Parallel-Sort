#include <pthread.h>
#include <string.h>
#include <math.h>

#include "quicksort.h"

// PHASE 1: Distribute workload and get all threads to locally run QS;
// Get partition

struct worker_args {
    int* array;
    int size;
    int partition_size;
};

int* slice(int A[], int s, int n) {
    // Return heap-backed slice of A starting from s with n elems
    int* new_a = malloc(n*sizeof(int));
    //if (slice == NULL) {return NULL;}
    memcpy(new_a, A + s, n*sizeof(int));
    return new_a;
}

void* worker_1(void* a) {
    // PHASE 1: Local quicksort; return partitions

    // Starts worker for a list A
    struct worker_args* args = (struct worker_args *) a;
    int* A = args->array;
    int n = args->size;
    int partition_size = args->partition_size;
    quick_sort(A, 0, n - 1);
    //print_list(A, n);

    // Partitions at index w*(n_tot/p^2)
    int num_partitions = n / partition_size;
    int* partitions = malloc(sizeof(int) * n/partition_size); 
    for (int i = 0; i < num_partitions; i++) {
        partitions[i] = A[i * partition_size];
    }


    // Free and return
    free(A);
    free(args);
    return (void*) partitions;
}

int main(int argc, char** argv) {
    // Program Args: [1] - NUM_THREADS

    int num_threads = 3;

    if (argc > 1) {
        num_threads = atoi(argv[1]);
    }

    pthread_t threads[num_threads];

    int a[] = {16, 2, 17, 24, 33, 28, 30, 1, 0, 27, 9, 25, 34,
               23, 19, 18, 11, 7, 21, 13, 8, 35, 12, 29, 6, 3,
               4, 14, 22, 15, 32, 10, 26, 31, 20, 5}; // as in paper


    // Divide work by number of processes
    int n = sizeof(a)/sizeof(a[0]);
    int idx_mul = n/num_threads;
    int partition_size = n/pow(num_threads, 2);
    int rem = n%num_threads;

    // Create threads and give them quicksort work on data
    for (int t = 0; t < num_threads; t++) {        
        int size = idx_mul + (t == num_threads - 1 ? rem : 0);
        int* a_slice = slice(a, t*idx_mul, idx_mul);

        struct worker_args* args = malloc(sizeof(struct worker_args));
        args->array = a_slice;
        args->size = size;
        args->partition_size = partition_size;

        pthread_create(&threads[t], NULL, worker_1, (void *) args);
    }

    int local_sample_size = (num_threads*(idx_mul/partition_size)+rem);
    int* reg_sample = malloc(local_sample_size*sizeof(int));
    int prev_size = 0;
    for (int t = 0; t < num_threads; t++) {
        void* partitions;
        pthread_join(threads[t], &partitions);
        int size = local_sample_size/num_threads + (t == num_threads - 1? rem : 0);

        memcpy(reg_sample + prev_size, (int*) partitions, size * sizeof(int));
        prev_size += size;

        free(partitions);
    }
    quick_sort(reg_sample, 0, local_sample_size);
    print_list(reg_sample, local_sample_size);

    int* new_pivots = malloc(sizeof(int)*(num_threads-1));
    for (int i = 0; i < num_threads-1; i++) {
        // Find p-1 pivots
        new_pivots[i] = reg_sample[i*num_threads + num_threads + 1];
    }
    free(reg_sample);
    print_list(new_pivots, 2);


    free(new_pivots);
}
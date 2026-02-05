#include "psrs.h"

pthread_barrier_t barrier;
int num_threads = 3;

struct worker_ctx {
    int* array;
    int slice_size;
    int size;

    int p;
    int** local_samples;
    int* pivots;

    int** partition_sizes;
    int*** partition_starts;

    int** final_buffer;
    int* final_sizes;

    int thread_id;
    int mode;
    int trial_num;
};

int cmp_int(const void* a, const void* b) {
    int x = *(const int*)a;
    int y = *(const int*)b;
    return (x > y) - (x < y); 
}

void print_list(int A[], int size) {
    // Prints list of ints
    printf("[");
    for (int i = 0; i < size; i++) {
        if (i == (size - 1)) {
            printf("%d", A[i]);
        } else {
            printf("%d ", A[i]);
        }
    }
    printf("]\n");
}

double calculateAverage(int arr[], int size) {
    int sum = 0;
    for (int i = 0; i < size; i++) {
        sum += arr[i];
    }

    return (double)sum / size;
}

// Thread logic for PSRS
void* worker(void* a) {
    // Starts worker for a list A
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int before = tv.tv_sec * 1000000L + tv.tv_usec;

    struct worker_ctx* ctx = (struct worker_ctx *) a;
    int thread_id = ctx->thread_id;

    int* array = ctx->array;
    int slice_size = ctx->slice_size;

    int size = ctx->size;

    int** local_samples = ctx->local_samples;
    int** final_buffer = ctx->final_buffer;
    int* final_sizes = ctx->final_sizes;

    int trial_num = ctx->trial_num;

    // Phase 1 - Local quicksort; return partitions
    qsort(array, slice_size, sizeof(int), cmp_int);
    //printf("Worker %d sorted:\n", thread_id);
    //print_list(array, slice_size);

    int p = ctx->p;
    int partition_size = size/pow(p, 2);
    
    for (int i = 0; i < p; i++) {
        local_samples[thread_id][i] = array[partition_size*i];
    }
    
    pthread_barrier_wait(&barrier);
    gettimeofday(&tv, NULL);
    int after = (tv.tv_sec * 1000000L + tv.tv_usec) - before;
    if (thread_id == 0 && trial_num == 9) printf("Phase I time: %d\n", after);
    before = tv.tv_sec * 1000000L + tv.tv_usec;
    //printf("Worker %d got pivots:\n", thread_id);
    //print_list(local_samples[thread_id], p);
    
    // Phase 2 - Thread id=0 gathers a regular sample, fills partitions
    int* pivots = ctx->pivots;
    if (thread_id == 0) {
        int* regular_sample = malloc(sizeof(int)*p*p);

        for (int t = 0; t < p; t++) {
            memcpy(regular_sample + t*p, local_samples[t], sizeof(int)*p);
        }

        qsort(regular_sample, p*p, sizeof(int), cmp_int);
        

        // Find (p-1) pivots
        for (int i = 0; i < p-1; i++) {
            pivots[i] = regular_sample[i*p + p];
        }
        //print_list(pivots, p-1);

        free(regular_sample);
    }

    pthread_barrier_wait(&barrier);
    gettimeofday(&tv, NULL);
    after = (tv.tv_sec * 1000000L + tv.tv_usec) - before;
    if (thread_id == 0 && trial_num == 9) printf("Phase II time: %d\n", after);
    before = tv.tv_sec * 1000000L + tv.tv_usec;
    // Phase 3 - Each process gets partitions
    // Each partition starts: 0 or pivot

    int*** partition_starts = ctx->partition_starts;
    int** partition_sizes = ctx->partition_sizes;

    // Insert pivots into partition
    int last_idx = 0;
    int s = 0;

    // Thread i gets ith partition_start

    // Phase 3: compute partition starts & sizes
    partition_starts[thread_id][0] = array;

    for (int i = 1; i < p; i++) {
        s = 0;
        while ((last_idx < slice_size) && (array[last_idx] <= pivots[i-1])) {
            last_idx++;
            s++;
        }
        partition_starts[thread_id][i] = &array[last_idx];
        partition_sizes[thread_id][i-1] = s;
        //if (thread_id == 2) printf("partition_start value %d: %d w/ size: %d\n", i, array[last_idx], s);
    }

    // Last partition
    partition_sizes[thread_id][p-1] = slice_size - last_idx;
    //if (thread_id == 2) printf("last partition size: %d\n", slice_size - last_idx);

    //printf("last idx: %d\n", last_idx);
    //printf("last partition size: %d\n", slice_size-last_idx);
    pthread_barrier_wait(&barrier);
    gettimeofday(&tv, NULL);
    after = (tv.tv_sec * 1000000L + tv.tv_usec) - before;
    if (thread_id == 0 && trial_num == 9) printf("Phase III time: %d\n", after);
    before = tv.tv_sec * 1000000L + tv.tv_usec;

    // Phase 4 - Get the lists and do a merge (not sort)

    // Take the thread_idth partition from each processor's contribution

    // Gather partitions
    int** local_buffer = malloc(sizeof(int*)*p);
    int* local_buff_sizes = malloc(sizeof(int)*p);
    int cumul_size = 0;
    for (int t = 0; t < p; t++) {
        int part_size = partition_sizes[t][thread_id];
        local_buffer[t] = malloc(sizeof(int)*part_size);
        memcpy(local_buffer[t], partition_starts[t][thread_id], sizeof(int)*part_size);
        local_buff_sizes[t] = part_size;
        cumul_size += part_size;
    }

    // Each iteration finds the minimum among local_buffers 
    int* merged = malloc(sizeof(int)*cumul_size);
    // Track current index into each local buffer
    int* idx = calloc(p, sizeof(int));   // initialized to 0

    for (int i = 0; i < cumul_size; i++) {
        int min_val = INT_MAX;   
        int min_buf = -1;

        // Scan all k buffers
        for (int t = 0; t < p; t++) {

            // If this buffer still has elements left
            if (idx[t] < local_buff_sizes[t]) {

                int val = local_buffer[t][idx[t]];

                // Update global minimum
                if (val < min_val) {
                    min_val = val;
                    min_buf = t;
                }
            }
        }

        // Place smallest element into merged output
        merged[i] = min_val;
        idx[min_buf]++;
    }
    free(idx);


    pthread_barrier_wait(&barrier); 
    gettimeofday(&tv, NULL);
    after = (tv.tv_sec * 1000000L + tv.tv_usec) - before;
    if (thread_id == 0 && trial_num == 9) printf("Phase IV time: %d\n", after);
    final_buffer[thread_id] = merged;
    final_sizes[thread_id] = cumul_size;
    //if (thread_id == 2) print_list(merged, cumul_size);

    for (int t = 0; t < p; t++) {
        free(local_buffer[t]);
    }

    free(local_buffer);
    free(local_buff_sizes);
    
    return NULL; 
}

// Carry out the PSRS sorting algorithm - includes setup and teardown
void psrs_sort(int* a, int n, int mode, int trial_num) {
      // Divide work by number of processes
    int idx_mul = n/num_threads;
    int rem = n%num_threads;

    // Initialize shared data structures
    int** local_samples = malloc(sizeof(int*) * num_threads);   
    for (int i = 0; i < num_threads; i++) {
        local_samples[i] = malloc(sizeof(int) * num_threads);   
    }

    int*** partition_starts = malloc(sizeof(int*) * num_threads);   
    for (int i = 0; i < num_threads; i++) {
        partition_starts[i] = malloc(sizeof(int*) * num_threads);   
    }

    int** partition_sizes = malloc(sizeof(int*) * num_threads);   
    for (int i = 0; i < num_threads; i++) {
        partition_sizes[i] = malloc(sizeof(int) * num_threads);   
    }

    int* pivots = malloc(sizeof(int)*(num_threads-1));

    int** final_buffer = malloc(sizeof(int*)*num_threads);
    int* final_sizes = malloc(sizeof(int)*num_threads);

    struct worker_ctx** contexts = malloc(num_threads*sizeof(struct worker_ctx*));

    // Initialize barrier, list of threads
    pthread_barrier_init(&barrier, NULL, num_threads);
    pthread_t threads[num_threads];

    // Create threads and give them quicksort work on data
    for (int t = 0; t < num_threads; t++) {        
        int size = idx_mul + (t == num_threads - 1 ? rem : 0);
        int* a_slice = a + t*idx_mul; // Modified ptr of shared array

        // Each worker gets its own context
        struct worker_ctx* ctx = malloc(sizeof(struct worker_ctx));
        ctx->array = a_slice;
        ctx->slice_size = size;
        ctx->size = n;
        ctx->p = num_threads;

        ctx->local_samples = local_samples;
        ctx->pivots = pivots;

        ctx->partition_sizes = partition_sizes;
        ctx->partition_starts = partition_starts;

        ctx->thread_id = t;

        ctx->final_buffer = final_buffer;
        ctx->final_sizes = final_sizes;
        ctx-> mode = mode;
        ctx->trial_num = trial_num;

        contexts[t] = ctx;

        pthread_create(&threads[t], NULL, worker, (void *) ctx);
    }

    for (int t = 0; t < num_threads; t++) {
        pthread_join(threads[t], NULL);
    }

    // Concatenate finalized list:
    int offset = 0;
    for (int t = 0; t < num_threads; t++) {
        int curr_size = final_sizes[t];
        memcpy(a + offset, final_buffer[t], curr_size*sizeof(int));
        offset += curr_size;
    }

    //free(a);
    free(pivots);
    for (int t = 0; t < num_threads; t++) {
        free(local_samples[t]);
        free(partition_sizes[t]);
        free(partition_starts[t]);
        free(contexts[t]);
        free(final_buffer[t]);
    }
    free(local_samples);
    free(partition_sizes);
    free(partition_starts);
    free(contexts);

    free(final_buffer);
    free(final_sizes);
}

int main(int argc, char** argv) {
    // Program Args: [1] - NUM_THREADS
    //               [2] - MODE (1: Single test, 2: Experiment)

    int mode = 1;
    int max_size = 32000001;

    if (argc > 1) {
        num_threads = atoi(argv[1]);
        if (num_threads < 1) {
            printf("Error: Number of threads must be positive");
            return 0;
        }
        if (argc > 2) {
            mode = atoi(argv[2]);
        }

        if (argc > 3) {
            max_size = atoi(argv[3]);
        }
    }

    if (mode == 2) {
        struct timeval tv;
        // Generate random sample
        for (int size = 1000000; size <= max_size; size = size*2) {
            
            // Run trials; take average
            
            int trial_times_q[10];
            int trial_times_p[10];
            for (int trial = 0; trial < 10; trial++) {
                int* a = malloc(sizeof(int)*size);
                int* b = malloc(sizeof(int)*size);

                // Randomly sample lists
                for (int i = 0; i < size; i++) {
                    a[i] = random();
                    b[i] = a[i];
                }

                // Run one experiment of quicksort
                gettimeofday(&tv, NULL);
                int before = tv.tv_sec * 1000000L + tv.tv_usec;
                qsort(a, size, sizeof(int), cmp_int);
                gettimeofday(&tv, NULL);
                trial_times_q[trial] = (tv.tv_sec * 1000000L + tv.tv_usec) - before;

                // Run one experiment of PSRS
                gettimeofday(&tv, NULL);
                before = tv.tv_sec * 1000000L + tv.tv_usec;
                psrs_sort(b, size, 2, trial);
                gettimeofday(&tv, NULL);
                trial_times_p[trial] = (tv.tv_sec * 1000000L + tv.tv_usec) - before;

                // Verify correctness
                for (int i = 0; i < size; i++) {
                    if (a[i] != b[i]) {
                        printf("Diff detected.");
                    }
                }

                free(a);
                free(b);
            }

            double q_avg = calculateAverage(trial_times_q, 10);
            double p_avg = calculateAverage(trial_times_p, 10);
            printf("========================================\nSize: %d\n", size);
            printf("Quicksort 10-trial avg: %.4g\n", q_avg);
            printf("PSRS 10-trial avg: %.4g\n", p_avg);
            printf("Average Speedup: %.4g\n", q_avg/p_avg);
        }
    } else {
        int a[] = {16, 2, 17, 24, 33, 28, 30, 1, 0, 27, 9, 25, 34,
                   23, 19, 18, 11, 7, 21, 13, 8, 35, 12, 29, 6, 3,
                   4, 14, 22, 15, 32, 10, 26, 31, 20, 5}; // as in paper
        int n = sizeof(a)/sizeof(a[0]);
        psrs_sort(a, n, 1, 0);
        print_list(a, n);
    }

  
    return 0;
}
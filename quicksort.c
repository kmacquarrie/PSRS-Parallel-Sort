// Basic C implementation of QuickSort

#include "quicksort.h"

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

void swap(int* a, int* b) {
    // Basic swap assuming two int pointers
    int tmp = *a;
    *a = *b;
    *b = tmp;
    return;
}

int partition(int A[], int low, int high) {
   // Pivot value
    int pivot = A[high]; 
    int i = low - 1;
   
    for (int j = low; j <= high - 1; j++) {
        if (A[j] < pivot) {
            i++;
            swap(&A[i], &A[j]);
        }
    }
        
    // Swap A[i] and A[j]
    swap(&A[i + 1], &A[high]);
    return i + 1;
}

void quick_sort(int A[], int low, int high) {
    if (low < high) {
        int p = partition(A, low, high);
        quick_sort(A, low, p - 1);
        quick_sort(A, p+1, high);
    }
}

/*
int main() {
    int a[] = {9, 8, 7, 6, 5, 4, 3, 2, 1};
    int n = sizeof(a)/sizeof(a[0]);
    quick_sort(a, 0, n - 1);
    print_list(a, n);
}
*/
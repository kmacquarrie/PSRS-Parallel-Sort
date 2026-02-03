// Basic C implementation of QuickSort

#include <stdio.h>
#include <stdlib.h>

void print_list(int A[], int size);

void swap(int* a, int* b);

int partition(int A[], int low, int high);

void quick_sort(int A[], int low, int high);

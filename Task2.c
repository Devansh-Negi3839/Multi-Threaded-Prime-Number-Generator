#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <sys/time.h>

#define N 10000
#define SQRT_N ((int)sqrt(N))
#define NUM_THREADS 4
#define BUFF_SIZE 1000

pthread_mutex_t mutex;
pthread_cond_t condition, condition2;
int curr = 0;
int *store;
int *sieve;
int count = 0;

int in = 0, out = 0;

// Initialize the sieve array with all values set to 1.
void initSieve(int *sieve, int size)
{
    for (int i = 0; i < size; i++)
        sieve[i] = 1;
}

// Put a value into the buffer.
void put(int value)
{
    pthread_mutex_lock(&mutex);

    // Wait while the buffer is full.
    while (count == BUFF_SIZE)
    {
        pthread_cond_wait(&condition2, &mutex);
    }

    // Insert the value into the buffer.
    store[in] = value;
    in = (in + 1) % BUFF_SIZE;
    count++;

    // Signal that a value has been added to the buffer.
    pthread_cond_signal(&condition);

    pthread_mutex_unlock(&mutex);
}

// Get a value from the buffer.
int get()
{
    pthread_mutex_lock(&mutex);

    // Wait while the buffer is empty.
    while (count == 0)
    {
        pthread_cond_wait(&condition, &mutex);
    }

    // Retrieve a value from the buffer.
    int val = store[out];
    out = (out + 1) % BUFF_SIZE;
    count--;

    // Signal that a space is available in the buffer.
    pthread_cond_signal(&condition2);

    pthread_mutex_unlock(&mutex);
    return val;
}

// Worker function for the sieve algorithm.
void *sieve_worker(void *arg)
{
    while (1)
    {
        // Get a value from the buffer.
        int val = get();
        if (val == -1)
        {
            return NULL;
        }
        // Mark multiples of the prime number as non-prime.
        if (sieve[val] == 1)
        {
            for (int i = val * val; i <= N; i += val)
            {
                sieve[i] = 0;
            }
        }
    }
    return NULL;
}

int main()
{
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&condition, NULL);
    pthread_cond_init(&condition2, NULL);
    int total_Primes = 0;
    int sieveSize = N + 1;
    sieve = (int *)malloc(sieveSize * sizeof(int));

    if (sieve == NULL)
    {
        printf("Memory allocation failed.\n");
        return 1;
    }

    initSieve(sieve, sieveSize);

    store = (int *)malloc(BUFF_SIZE * sizeof(int));

    if (store == NULL)
    {
        printf("Memory allocation failed.\n");
        free(sieve);
        return 1;
    }

    struct timeval start, end;

    gettimeofday(&start, NULL);

    // Create worker threads
    pthread_t threads[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++)
    {
        if (pthread_create(&threads[i], NULL, sieve_worker, NULL) != 0)
        {
            printf("Thread creation failed.\n");
            return 1;
        }
    }

    // Initialize sieve with the first set of prime numbers.
    for (int p = 2; p * p <= SQRT_N; p++)
    {
        if (sieve[p] == 1)
        {
            for (int i = p * p; i <= SQRT_N; i += p)
            {
                sieve[i] = 0;
            }
            // Put the prime number into the buffer.
            put(p);
        }
    }

    // Put the remaining prime numbers into the buffer.
    for (int p = sqrt(SQRT_N) + 1; p <= SQRT_N; p++)
    {
        if (sieve[p] == 1)
        {
            put(p);
        }
    }

    // Signal the end of prime number generation by putting -1 into the buffer.
    for (int p = 0; p <= NUM_THREADS; p++)
    {
        put(-1);
    }

    // Signal all threads to wake up.
    pthread_cond_broadcast(&condition);

    // Wait for threads to finish
    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    gettimeofday(&end, NULL);

    long seconds = end.tv_sec - start.tv_sec;
    long micros = ((seconds * 1000000) + end.tv_usec) - (start.tv_usec);

    // Print remaining prime numbers
    for (int num = 2; num <= N; num++)
    {
        if (sieve[num])
        {
            printf("%d ", num);
            total_Primes++;
        }
    }

    printf("\n");
    printf("total_Primes=%d\n", total_Primes);
    free(sieve);
    free(store);

    printf("\nTime taken: %ld seconds %ld microseconds\n", seconds, micros);
    return 0;
}

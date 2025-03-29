#include <stdio.h>
#include <thread>

#include "CycleTimer.h"

typedef struct {
    float x0, x1;
    float y0, y1;
    unsigned int width;
    unsigned int height;
    int maxIterations;
    int* output;
    int threadId;
    int numThreads;
} WorkerArgs;


extern void mandelbrotSerial(
    float x0, float y0, float x1, float y1,
    int width, int height,
    int startRow, int numRows,
    int maxIterations,
    int output[]);


//
// workerThreadStart --
//
// Thread entrypoint.
// NAIVE SOLUTION, IMBALANCE WORKLOAD BETWEEN THREADS
// void workerThreadStart(WorkerArgs * const args) {

//     // TODO FOR CS149 STUDENTS: Implement the body of the worker
//     // thread here. Each thread should make a call to mandelbrotSerial()
//     // to compute a part of the output image.  For example, in a
//     // program that uses two threads, thread 0 could compute the top
//     // half of the image and thread 1 could compute the bottom half.
//     int thread_total_rows;
//     int thread_start_row;
//     //printf("Hello world from thread %d\n", args->threadId);
//     // Use the thread ID to determine start_row for each thread
//     // Assume start_row is 0 and total rows is height.
//     // Need to consider when height is not fully divisible by numThreads. The last thread need to compute the extra tail.
//     if (args->height % args->numThreads !=0 && args->threadId == args->numThreads-1) {
//         thread_total_rows = args->height / args->numThreads + args->height % args->numThreads;
//     }
//     else {
//         thread_total_rows = args->height / args->numThreads;
//     }
//     thread_start_row = args->threadId * (args->height / args->numThreads);
    
//     printf("thread ID = %d\n", args->threadId);
//     printf("thread total rows = %d\n", thread_total_rows);
//     printf("thread start rows = %d\n", thread_start_row);

//     double startTime = CycleTimer::currentSeconds();
//     mandelbrotSerial(args->x0, args->y0, args->x1, args->y1, args->width, args->height, thread_start_row, thread_total_rows, args->maxIterations, args->output);
//     double endTime = CycleTimer::currentSeconds();
//     printf("Thread %d: %.3f ms\n", args->threadId, (endTime - startTime) * 1000);
// }

// For each thread, there's a stride between each row assigned to it, stride = #threads, this evenly distribute the workloads.
void workerThreadStart(WorkerArgs * const args) {
    
    unsigned int thread_total_rows = 1;
    unsigned int thread_start_row = args->threadId;

    while (thread_start_row <= args->height-1){
        mandelbrotSerial(args->x0, args->y0, args->x1, args->y1, args->width, args->height, thread_start_row, thread_total_rows, args->maxIterations, args->output);
        thread_start_row = thread_start_row + args->numThreads;
    }
}

//
// MandelbrotThread --
//
// Multi-threaded implementation of mandelbrot set image generation.
// Threads of execution are created by spawning std::threads.
void mandelbrotThread(
    int numThreads,
    float x0, float y0, float x1, float y1,
    int width, int height,
    int maxIterations, int output[])
{
    static constexpr int MAX_THREADS = 32;

    if (numThreads > MAX_THREADS)
    {
        fprintf(stderr, "Error: Max allowed threads is %d\n", MAX_THREADS);
        exit(1);
    }

    // Creates thread objects that do not yet represent a thread.
    std::thread workers[MAX_THREADS];
    WorkerArgs args[MAX_THREADS];

    for (int i=0; i<numThreads; i++) {
      
        // TODO FOR CS149 STUDENTS: You may or may not wish to modify
        // the per-thread arguments here.  The code below copies the
        // same arguments for each thread
        args[i].x0 = x0;
        args[i].y0 = y0;
        args[i].x1 = x1;
        args[i].y1 = y1;
        args[i].width = width;
        args[i].height = height;
        args[i].maxIterations = maxIterations;
        args[i].numThreads = numThreads;
        args[i].output = output;
      
        args[i].threadId = i;
    }

    // Spawn the worker threads.  Note that only numThreads-1 std::threads
    // are created and the main application thread is used as a worker
    // as well.
    for (int i=1; i<numThreads; i++) {
        workers[i] = std::thread(workerThreadStart, &args[i]);
    }
    
    workerThreadStart(&args[0]);

    // join worker threads
    for (int i=1; i<numThreads; i++) {
        workers[i].join();
    }
}


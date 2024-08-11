#include <algorithm>
#include <iostream>
#include <getopt.h>
#include <math.h>
#include <random>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "CycleTimer.h"
#include "const.h"

#define SEED 7
#define SAMPLE_RATE 1e-2

using namespace std;

typedef struct {
  int numThreads;
  ThreadOption threadType;
  bool threadOnly;
} KmeansArgs;

// Main compute functions
extern void kMeansThread(double *data, double *clusterCentroids,
                      int *clusterAssignments, int M, int N, int K,
                      double epsilon, ThreadOption threadType, int numThreads);
extern void kMeansSerial(double *data, double *clusterCentroids,
                      int *clusterAssignments, int M, int N, int K,
                      double epsilon, ThreadOption threadType, int numThreads);
// extern void kMeansThread(double *data, double *clusterCentroids,
//                       int *clusterAssignments, int M, int N, int K, double epsilon);

extern double dist(double *x, double *y, int nDim);

// Utilities
extern void logToFile(string filename, double sampleRate, double *data,
                      int *clusterAssignments, double *clusterCentroids, int M,
                      int N, int K);
extern void writeData(string filename, double *data, double *clusterCentroids,
                      int *clusterAssignments, int *M_p, int *N_p, int *K_p,
                      double *epsilon_p);
extern void readData(string filename, double **data, double **clusterCentroids,
                     int **clusterAssignments, int *M_p, int *N_p, int *K_p,
                     double *epsilon_p);


void usage(const char* progname) {
    printf("Usage: %s [options]\n", progname);
    printf("Program Options:\n");
    printf("  -n  --numThreads <N>  Use N threads\n");
    printf("  -t  --threadType      Threading type:\n");  
    printf("                          0: SERIAL\n");
    printf("                          1: CLUSTER\n");
    printf("                          2: DATA\n");
    printf("                          3: ALL\n");
    printf("  -to                   Run only thread version\n");
    printf("  -?  --help            This message\n");
}


void verifyArgs(KmeansArgs args) {
    if (args.numThreads < 1) {
        fprintf(stderr, "Error: Number of threads must be at least 1\n");
        exit(1);
    }
    if (args.numThreads > MAX_THREADS)
    {
        fprintf(stderr, "Error: Max allowed threads is %d\n", MAX_THREADS);
        exit(1);
    }
    if (args.threadType < 0 || args.threadType > 3) {
        fprintf(stderr, "Error: Thread type must be 0, 1, or 3, run ./kmeans -h for help\n");
        exit(1);
    }
    if (args.threadType == ThreadOption::SERIAL && args.numThreads > 1) {
        fprintf(stdout, "Warning: Serial threading does not support threading, numThreads value will be ignored\n");
    }

    if (args.threadType == ThreadOption::CLUSTER && args.numThreads > 1) {
        fprintf(stdout, "Warning: Cluster threading only support implicitly, numThreads value will be ignored\n");
    }

    printf("Running K-means with: numThreads=%d, threadType=%d, threadOnly=%d\n", args.numThreads, args.threadType, args.threadOnly ? 1 : 0);
} 


// Functions for generating data
double randDouble() {
  return static_cast<double>(rand()) / static_cast<double>(RAND_MAX);
}

void initData(double *data, int M, int N) {
  int K = 10;
  double *centers = new double[K * N];

  // Gaussian noise
  double mean = 0.0;
  double stddev = 0.5;
  std::default_random_engine generator;
  std::normal_distribution<double> normal_dist(mean, stddev);

  // Randomly create points to center data around
  for (int k = 0; k < K; k++) {
    for (int n = 0; n < N; n++) {
      centers[k * N + n] = randDouble();
    }
  }

  // Even clustering
  for (int m = 0; m < M; m++) {
    int startingPoint = rand() % K; // Which center to start from
    for (int n = 0; n < N; n++) {
      double noise = normal_dist(generator);
      data[m * N + n] = centers[startingPoint * N + n] + noise;
    }
  }

  delete[] centers;
}

void initCentroids(double *clusterCentroids, int K, int N) {
  // Initialize centroids (close together - makes it a bit more interesting)
  for (int n = 0; n < N; n++) {
    clusterCentroids[n] = randDouble();
  }
  for (int k = 1; k < K; k++) {
    for (int n = 0; n < N; n++) {
      clusterCentroids[k * N + n] =
          clusterCentroids[n] + (randDouble() - 0.5) * 0.1;
    }
  }
}

void genData(int M, int N, int K, double epsilon, double *data,
             double *clusterCentroids, int *clusterAssignments) {
  M = 1e6;
  N = 100;
  K = 3;
  epsilon = 0.1;

  data = new double[M * N];
  clusterCentroids = new double[K * N];
  clusterAssignments = new int[M];

  // Initialize data
  initData(data, M, N);
  initCentroids(clusterCentroids, K, N);

  // Initialize cluster assignments
  for (int m = 0; m < M; m++) {
    double minDist = 1e30;
    int bestAssignment = -1;
    for (int k = 0; k < K; k++) {
      double d = dist(&data[m * N], &clusterCentroids[k * N], N);
      if (d < minDist) {
        minDist = d;
        bestAssignment = k;
      }
    }
    clusterAssignments[m] = bestAssignment;
  }

  // Uncomment to generate data file
  writeData("./data.dat", data, clusterCentroids, clusterAssignments, &M, &N,
            &K, &epsilon);
}

int main(int argc, char** argv) {
  
  srand(SEED);

  int M, N, K;
  double epsilon;

  double *data;
  double *clusterCentroids;
  int *clusterAssignments;

  // Init thread options
  KmeansArgs args;
  args.numThreads = 1;
  args.threadType = ThreadOption::SERIAL;
  args.threadOnly = false;

  // parse commandline options ////////////////////////////////////////////
  int opt;
  static struct option long_options[] = {
      {"numThreads", 1, 0, 'n'},
      {"threadType", 1, 0, 't'},
      {"threadOnly", 0, 0, 'o'},
      {"help", 0, 0, '?'},
      {0 ,0, 0, 0}
  };

  while ((opt = getopt_long(argc, argv, "n:t:o:?", long_options, NULL)) != EOF) {

      switch (opt) {
      case 't':
      {
          args.threadType = static_cast<ThreadOption>(atoi(optarg));
          break;
      }
      case 'n':
      {
          args.numThreads = atoi(optarg);
          break;
      }
      case 'o':
      {
          args.threadOnly = true;
          break;
      }
      //@@
      case '?':
      default:
          usage(argv[0]);
          return 1;
      }
  }
  verifyArgs(args);
  // end parsing of commandline options //////////////////////////////////////
  

  // NOTE: we will grade your submission using the data in data.dat
  // which is read by this function
  readData("./data.dat", &data, &clusterCentroids, &clusterAssignments, &M, &N,
           &K, &epsilon);

  // NOTE: if you want to generate your own data (for fun), you can use the
  // below code to generate data and write it to a file (remember to uncomment the readData() if you do this)
  // Then comment out genData(), build and run the program again
  
  // genData(M, N, K, epsilon, data, clusterCentroids, clusterAssignments);

  printf("Running K-means with: M=%d, N=%d, K=%d, epsilon=%f\n", M, N,
         K, epsilon);

  // Log the starting state of the algorithm
  logToFile("./start.log", SAMPLE_RATE, data, clusterAssignments,
            clusterCentroids, M, N, K);

  double endTimeSerial = 0;
  double startTimeSerial = 0;

  if (!args.threadOnly){
    startTimeSerial = CycleTimer::currentSeconds();
    kMeansSerial(data, clusterCentroids, clusterAssignments, M, N, K, epsilon, args.threadType, args.numThreads);
    endTimeSerial = CycleTimer::currentSeconds();
    printf("[Total Time] kmeansSerial: %.3f ms\n", (endTimeSerial - startTimeSerial) * 1000);

    // Log the end state of the algorithm
    logToFile("./end_serial.log", SAMPLE_RATE, data, clusterAssignments,
              clusterCentroids, M, N, K);
  }

  if (args.threadType != ThreadOption::SERIAL) {
    if (!args.threadOnly) readData("./data.dat", &data, &clusterCentroids, &clusterAssignments, &M, &N,
           &K, &epsilon);

    double startTime = CycleTimer::currentSeconds();
    kMeansThread(data, clusterCentroids, clusterAssignments, M, N, K, epsilon, args.threadType, args.numThreads);
    double endTime = CycleTimer::currentSeconds();
    printf("[Total Time] kmeansThread: %.3f ms\n", (endTime - startTime) * 1000);

    if (!args.threadOnly) printf("[Speedup] kmeansThread: %.3f\n", (endTimeSerial - startTimeSerial) / (endTime - startTime));

    // Log the end state of the algorithm
    logToFile("./end_thread.log", SAMPLE_RATE, data, clusterAssignments,
              clusterCentroids, M, N, K);
  }
  

  free(data);
  free(clusterCentroids);
  free(clusterAssignments);
  return 0;
}
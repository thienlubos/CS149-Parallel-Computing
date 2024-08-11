#include <algorithm>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <mutex>

#include "utils.h"
#include "CycleTimer.h"
#include "const.h"

using namespace std;


/**
 * Computes L2 distance between two points of dimension nDim.
 * 
 * @param x Pointer to the beginning of the array representing the first
 *     data point.
 * @param y Poitner to the beginning of the array representing the second
 *     data point.
 * @param nDim The dimensionality (number of elements) in each data point
 *     (must be the same for x and y).
 */
double dist(double *x, double *y, int nDim) {
  double accum = 0.0;
  for (int i = 0; i < nDim; i++) {
    accum += pow((x[i] - y[i]), 2);
  }
  return sqrt(accum);
}

void assignmentsSerial(WorkerArgs *const args) {
  double *minDist = new double[args->M];
  for (int m =0; m < args->M; m++) {
      minDist[m] = 1e30;
      args->clusterAssignments[m] = -1;
  }
  for (int k = args->start; k < args->end; k++) {
      for (int m = 0; m < args->M; m++) {
        double d = dist(&args->data[m * args->N], &args->clusterCentroids[k *args-> N], args->N);
        // if (m % 200000 == 0) printf("[Compute] thread = %d, m = %d, minDist = %f\n", k, m, d); 
        if (d < minDist[m]) {
          minDist[m] = d;
          args->clusterAssignments[m] = k;
        }
      }
  }

  for (int m = 0; m < args->M; m++) {
    // if (m % 200000 == 0) printf("[Combine] clusterAssignment[%d] = %d\n", m, args->clusterAssignments[m]);
  }

  delete[] minDist;
  
}

/**
 * Checks if the algorithm has converged.
 * 
 * @param prevCost Pointer to the K dimensional array containing cluster costs 
 *    from the previous iteration.
 * @param currCost Pointer to the K dimensional array containing cluster costs 
 *    from the current iteration.
 * @param epsilon Predefined hyperparameter which is used to determine when
 *    the algorithm has converged.
 * @param K The number of clusters.
 * 
 * NOTE: DO NOT MODIFY THIS FUNCTION!!!
 */
static bool stoppingConditionMet(double *prevCost, double *currCost,
                                 double epsilon, int K) {
  for (int k = 0; k < K; k++) {
    if (abs(prevCost[k] - currCost[k]) > epsilon)
      return false;
  }
  return true;
}


void verifyNumThread(int numThreads) {
  
  if (numThreads < 1) {
    fprintf(stderr, "Error: Number of threads must be at least 1\n");
    exit(1);
  }

  if (numThreads > MAX_THREADS)
  {
      fprintf(stderr, "Error: Max allowed threads is %d\n", MAX_THREADS);
      exit(1);
  }
}


/**
 * Assigns each data point to its "closest" cluster centroid.
 */
void computeAssignments(WorkerArgs *const args) {
  
  double start1, start2;

  // Initialize arrays
  start1 = CycleTimer::currentSeconds();
  
  args->combineTime.time += (CycleTimer::currentSeconds() - start1) * 1000;
  args->combineTime.count++;

  // Assign datapoints to closest centroids
  start2 = CycleTimer::currentSeconds();

  // Serial
  assignmentsSerial(args);

  args->assignTime.time += (CycleTimer::currentSeconds() - start2) * 1000;
  args->assignTime.count++;
}


void assignmentThreadAllStart(WorkerArgs *const args, double *minDist, std::mutex &mutex) {
  int k = args->blockId;
  int start = args->threadId * args->step;
  int end = (args->threadId + 1) * args->step;
  for (int m = start; m < end; m++) {
    minDist[k * args->M + m] = dist(&args->data[m * args->N], 
                              &args->clusterCentroids[k * args->N],
                              args->N);
  }
}


void computeAssignmentsThreadAll(WorkerArgs &args) {

  double start1, start2;

  // Thread cluster
  std::mutex mutex;
  double *minDist = new double[args.M * args.K];
  int numBThreads = args.numThreads / args.K;
  args.step = std::max(args.M / numBThreads, 1);
  // printf("Step = %d\n", args.step);

  for (int m = 0; m < args.M; m++) {
    minDist[m] = 1e30;
    args.clusterAssignments[m] = -1;
  }
  verifyNumThread(args.numThreads);

    // Initialize arrays
  start1 = CycleTimer::currentSeconds();

  // Creates thread objects that do not yet represent a thread.
  std::thread workers[MAX_THREADS];
  WorkerArgs workerArgs[MAX_THREADS];

  for (int i = 0; i < args.numThreads; i++) {
    workerArgs[i] = args;
    workerArgs[i].threadId = (int) i / args.K;
    workerArgs[i].blockId = i % args.K;
    // printf("Thread %d, blockId = %d\n", workerArgs[i].threadId, workerArgs[i].blockId);
  }

  for (int i = 0; i < args.numThreads; i++) {
    workers[i] = std::thread(assignmentThreadAllStart, &workerArgs[i], minDist, std::ref(mutex));
  }

  for (int i = 0; i < args.numThreads; i++) {
    workers[i].join();
  }
  args.assignTime.time += (CycleTimer::currentSeconds() - start1) * 1000;
  args.assignTime.count++;

  start2 = CycleTimer::currentSeconds();
  for (int m = 0; m < args.M; m++) {
    double mydist = minDist[m];  
    int mycluster = 0;                
    for (int k = 1; k < args.K; k++) {
      if (minDist[k * args.M + m] < mydist) {
        mydist = minDist[k * args.M + m];
        mycluster = k;
      }
    }
    args.clusterAssignments[m] = mycluster;
  }
  args.combineTime.time += (CycleTimer::currentSeconds() - start2) * 1000;
  args.combineTime.count++;

  delete[] minDist;
  
}


void assignmentThreadClustersStart(WorkerArgs *const args, double *minDist, std::mutex &mutex) {
  int k = args->threadId;
  for (int m = 0; m < args->M; m++) {
    // Access by this will be slower than access by minDist[m * k]
    // minDist[m * args->K + k] = dist(&args->data[m * args->N], 
    //                                             &args->clusterCentroids[k * args->N],
    //                                             args->N);  
    minDist[k * args->M + m] = dist(&args->data[m * args->N], 
                              &args->clusterCentroids[k * args->N],
                              args->N);
    // if (m % 200000 == 0) printf("[Compute] Thread %d, m = %d, minDist = %f\n", args->threadId, m, minDist[m * args->K + k]);
    // mutex.lock();
    // if (d < minDist[m]) {
    //   minDist[m] = d;
    //   args->clusterAssignments[m] = args->threadId;
    // }
    // mutex.unlock();
  }
}


void computeAssignmentsThreadClusters(WorkerArgs &args) {

  double start1, start2;

  // Thread cluster
  std::mutex mutex;
  args.numThreads = args.K;
  double *minDist = new double[args.M * args.K];

  for (int m = 0; m < args.M; m++) {
    minDist[m] = 1e30;
    args.clusterAssignments[m] = -1;
  }
  verifyNumThread(args.numThreads);

    // Initialize arrays
  start1 = CycleTimer::currentSeconds();

  // Creates thread objects that do not yet represent a thread.
  std::thread workers[MAX_THREADS];
  WorkerArgs workerArgs[MAX_THREADS];

  for (int i = 0; i < args.numThreads; i++) {
    workerArgs[i] = args;
    workerArgs[i].threadId = i;
  }

  for (int i = 0; i < args.numThreads; i++) {
    workers[i] = std::thread(assignmentThreadClustersStart, &workerArgs[i], minDist, std::ref(mutex));
  }

  for (int i = 0; i < args.numThreads; i++) {
    workers[i].join();
  }
  args.assignTime.time += (CycleTimer::currentSeconds() - start1) * 1000;
  args.assignTime.count++;

  start2 = CycleTimer::currentSeconds();
  // for (int m = 0; m < args.M; m++) {
  //   double mydist = minDist[m * args.K];  
  //   int mycluster = 0;                
  //   for (int k = 1; k < args.K; k++) {
  //     if (minDist[m * args.K + k] < mydist) {
  //       mydist = minDist[m * args.K + k];
  //       mycluster = k;
  //     }
  //   }
  //   args.clusterAssignments[m] = mycluster;
  // }

  for (int m = 0; m < args.M; m++) {
    double mydist = minDist[m];  
    int mycluster = 0;                
    for (int k = 1; k < args.K; k++) {
      if (minDist[k * args.M + m] < mydist) {
        mydist = minDist[k * args.M + m];
        mycluster = k;
      }
    }
    args.clusterAssignments[m] = mycluster;
  }
  args.combineTime.time += (CycleTimer::currentSeconds() - start2) * 1000;
  args.combineTime.count++;

  delete[] minDist;
  
}


void assignmentsThreadDataStart(WorkerArgs *const args, double *minDist) {
  int m_start = args->threadId * args->step;
  int m_end = (args->threadId + 1) * args->step;
  if (args->threadId == args->numThreads - 1) {
    m_end = args->M;
  }
  for (int k = 0; k < args->K; k++) {
    for (int m =  m_start; m < m_end; m++) {
      double d = dist(&args->data[m * args->N], 
                                                &args->clusterCentroids[k * args->N],
                                                args->N);
      if (d < minDist[m]) {
        minDist[m] = d;
        args->clusterAssignments[m] = k;
      }
    }
  }
}


void computeAssignmentsThreadData(WorkerArgs &args) {
  // Thread data
  double *minDist = new double[args.M];

  for (int m = 0; m < args.M; m++) {
    minDist[m] = 1e30;
    args.clusterAssignments[m] = -1;
  }

  verifyNumThread(args.numThreads);

  // Creates thread objects that do not yet represent a thread.
  std::thread workers[MAX_THREADS];
  WorkerArgs workerArgs[MAX_THREADS];

  for (int i = 0; i < args.numThreads; i++) {
    workerArgs[i] = args;
    workerArgs[i].threadId = i;
  }

  for (int i = 0; i < args.numThreads; i++) {
    workers[i] = std::thread(assignmentsThreadDataStart, &workerArgs[i], minDist);
  }

  for (int i = 0; i < args.numThreads; i++) {
    workers[i].join();
  }

  delete[] minDist;

}

/**
 * Given the cluster assignments, computes the new centroid locations for
 * each cluster.
 */
void computeCentroids(WorkerArgs *const args) {
  int *counts = new int[args->K];

  // Zero things out
  for (int k = 0; k < args->K; k++) {
    counts[k] = 0;
    for (int n = 0; n < args->N; n++) {
      args->clusterCentroids[k * args->N + n] = 0.0;
    }
  }

  // Sum up contributions from assigned examples
  for (int m = 0; m < args->M; m++) {
    int k = args->clusterAssignments[m];
    for (int n = 0; n < args->N; n++) {
      args->clusterCentroids[k * args->N + n] +=
          args->data[m * args->N + n];
    }
    counts[k]++;
  }

  // Compute means
  for (int k = 0; k < args->K; k++) {
    counts[k] = max(counts[k], 1); // prevent divide by 0
    for (int n = 0; n < args->N; n++) {
      args->clusterCentroids[k * args->N + n] /= counts[k];
    }
  }

  // free(counts);

}

/**
 * Computes the per-cluster cost. Used to check if the algorithm has converged.
 */
void computeCost(WorkerArgs *const args) {

  double *accum = new double[args->K];

  // Zero things out
  for (int k = 0; k < args->K; k++) {
    accum[k] = 0.0;
  }

  // Sum cost for all data points assigned to centroid
  for (int m = 0; m < args->M; m++) {
    int k = args->clusterAssignments[m];
    accum[k] += dist(&args->data[m * args->N],
                     &args->clusterCentroids[k * args->N], args->N);
  }

  // Update costs
  for (int k = args->start; k < args->end; k++) {
    args->currCost[k] = accum[k];
  }

  delete[] accum;
}

/**
 * Computes the K-Means algorithm, using std::thread to parallelize the work.
 *
 * @param data Pointer to an array of length M*N representing the M different N 
 *     dimensional data points clustered. The data is layed out in a "data point
 *     major" format, so that data[i*N] is the start of the i'th data point in 
 *     the array. The N values of the i'th datapoint are the N values in the 
 *     range data[i*N] to data[(i+1) * N].
 * @param clusterCentroids Pointer to an array of length K*N representing the K 
 *     different N dimensional cluster centroids. The data is laid out in
 *     the same way as explained above for data.
 * @param clusterAssignments Pointer to an array of length M representing the
 *     cluster assignments of each data point, where clusterAssignments[i] = j
 *     indicates that data point i is closest to cluster centroid j.
 * @param M The number of data points to cluster.
 * @param N The dimensionality of the data points.
 * @param K The number of cluster centroids.
 * @param epsilon The algorithm is said to have converged when
 *     |currCost[i] - prevCost[i]| < epsilon for all i where i = 0, 1, ..., K-1
 */
void kMeansThread(double *data, double *clusterCentroids, int *clusterAssignments,
               int M, int N, int K, double epsilon, ThreadOption threadType, int numThreads) {

  // Used to track convergence
  double *prevCost = new double[K];
  double *currCost = new double[K];

  // The WorkerArgs array is used to pass inputs to and return output from
  // functions.
  WorkerArgs args;
  args.data = data;
  args.clusterCentroids = clusterCentroids;
  args.clusterAssignments = clusterAssignments;
  args.currCost = currCost;
  args.M = M;
  args.N = N;
  args.K = K;
  args.step = std::max(M / numThreads, 1);

  //@@ Thien Lu add
  args.threadOption = threadType;
  args.numThreads = numThreads;
  //@@

  // Initialize arrays to track cost
  for (int k = 0; k < K; k++) {
    prevCost[k] = 1e30;
    currCost[k] = 0.0;
  }

  /* Time logging */
  double startTime1, startTime2, startTime3, endTime1, endTime2, endTime3;  
  double diff1 = 0;
  double diff2 = 0;
  double diff3 = 0;

  args.assignTime.time = 0;
  args.assignTime.count = 0;
  args.combineTime.time = 0;
  args.combineTime.count = 0;
  
    
  /* Main K-Means Algorithm Loop */
  int iter = 0;
  
  while (!stoppingConditionMet(prevCost, currCost, epsilon, K)) {
      printf("--------------------------------\n");
      printf("Iteration %d\n", iter);
      // Update cost arrays (for checking convergence criteria)
      for (int k = 0; k < K; k++) {
        prevCost[k] = currCost[k];
      }

      // Setup args struct
      args.start = 0;
      args.end = K;

      startTime1 = CycleTimer::currentSeconds();
      switch (threadType)
      {
      case ThreadOption::SERIAL:
        computeAssignments(&args);
        break;
      case ThreadOption::CLUSTER:
        computeAssignmentsThreadClusters(args);
        break;
      case ThreadOption::DATA:
        computeAssignmentsThreadData(args);
        break;
      case ThreadOption::ALL:
        computeAssignmentsThreadAll(args);
        break;
      default:
        break;
      }
      endTime1 = CycleTimer::currentSeconds();
      diff1 += (endTime1 - startTime1) * 1000;

      startTime2 = CycleTimer::currentSeconds();
      computeCentroids(&args);
      endTime2 = CycleTimer::currentSeconds();
      diff2 += (endTime2 - startTime2) * 1000;

      startTime3 = CycleTimer::currentSeconds();
      computeCost(&args);
      endTime3 = CycleTimer::currentSeconds();
      diff3 += (endTime3 - startTime3) * 1000;

      iter++;
    }

    printf("[Total Time]: computeAssignments() \t\t %.3f ms\n", diff1 / iter);
    printf("[Total Time]: computeAssignments()::assignTime \t\t %.3f ms\n", args.assignTime.time / args.assignTime.count);
    printf("[Total Time]: computeAssignments()::combineTime \t\t %.3f ms\n", args.combineTime.time / args.combineTime.count);
    printf("[Total Time]: computeCentroids()   \t\t %.3f ms\n", diff2 / iter);
    printf("[Total Time]: computeCost()        \t\t %.3f ms\n", diff3 / iter);
  
    writeSpeedResults("./speedup.log", "computeAssignments()", diff1 / iter);
    writeSpeedResults("./speedup.log", "computeCentroids()", diff1 / iter);
    writeSpeedResults("./speedup.log", "computeCost()", diff1 / iter);
    writeSpeedResults("./speedup.log", "numThreads", args.numThreads);
    writeSpeedResults("./speedup.log", "threadType", threadType);
    writeSpeedResults("./speedup.log", "total_time", (diff1 + diff2 + diff3) / iter);

  delete[] currCost;
  delete[] prevCost;
}

void kMeansSerial(double *data, double *clusterCentroids, int *clusterAssignments,
               int M, int N, int K, double epsilon, ThreadOption threadType, int numThreads) {

  // Used to track convergence
  double *prevCost = new double[K];
  double *currCost = new double[K];

  // The WorkerArgs array is used to pass inputs to and return output from
  // functions.
  WorkerArgs args;
  args.data = data;
  args.clusterCentroids = clusterCentroids;
  args.clusterAssignments = clusterAssignments;
  args.currCost = currCost;
  args.M = M;
  args.N = N;
  args.K = K;
  args.step = std::max(M / numThreads, 1);
  printf("Step = %d\n", args.step);

  //@@ Thien Lu add
  args.threadOption = threadType;
  args.numThreads = numThreads;
  //@@

  // Initialize arrays to track cost
  for (int k = 0; k < K; k++) {
    prevCost[k] = 1e30;
    currCost[k] = 0.0;
  }

  /* Time logging */
  double startTime1, startTime2, startTime3, endTime1, endTime2, endTime3;  
  double diff1 = 0;
  double diff2 = 0;
  double diff3 = 0;
  args.combineTime.time = 0;
  args.combineTime.count = 0;
  args.assignTime.time = 0;
  args.assignTime.count = 0;
  
  /* Main K-Means Algorithm Loop */
  int iter = 0;
  
  while (!stoppingConditionMet(prevCost, currCost, epsilon, K)) {
      printf("--------------------------------\n");
      printf("Iteration %d\n", iter);
      // Update cost arrays (for checking convergence criteria)
      for (int k = 0; k < K; k++) {
        prevCost[k] = currCost[k];
      }

      // Setup args struct
      args.start = 0;
      args.end = K;

      startTime1 = CycleTimer::currentSeconds();
      computeAssignments(&args);
      endTime1 = CycleTimer::currentSeconds();
      diff1 += (endTime1 - startTime1) * 1000;

      startTime2 = CycleTimer::currentSeconds();
      computeCentroids(&args);
      endTime2 = CycleTimer::currentSeconds();
      diff2 += (endTime2 - startTime2) * 1000;

      startTime3 = CycleTimer::currentSeconds();
      computeCost(&args);
      endTime3 = CycleTimer::currentSeconds();
      diff3 += (endTime3 - startTime3) * 1000;

      iter++;
    }

    printf("[Total Time]: computeAssignments() \t\t %.3f ms\n", diff1 / iter);
    printf("[Total Time]: computeCentroids()   \t\t %.3f ms\n", diff2 / iter);
    printf("[Total Time]: computeCost()        \t\t %.3f ms\n", diff3 / iter);
  
    writeSpeedResults("./speedup.log", "computeAssignments()", diff1 / iter);
    writeSpeedResults("./speedup.log", "computeCentroids()", diff1 / iter);
    writeSpeedResults("./speedup.log", "computeCost()", diff1 / iter);
    writeSpeedResults("./speedup.log", "numThreads", args.numThreads);
    writeSpeedResults("./speedup.log", "threadType", threadType);
    writeSpeedResults("./speedup.log", "total_time", (diff1 + diff2 + diff3) / iter);

  delete[] currCost;
  delete[] prevCost;
}


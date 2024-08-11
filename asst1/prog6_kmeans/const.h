
// const.h

#ifndef CONST_H
#define CONST_H

#define MAX_THREADS 64

enum ThreadOption {
  SERIAL = 0,
  CLUSTER = 1,
  DATA = 2,
  ALL = 3,
};

typedef struct {
  double time;
  double count;
} TimeCycle;

typedef struct {
  // Control work assignments
  int start, end;

  // Shared by all functions
  double *data;
  double *clusterCentroids;
  int *clusterAssignments;
  double *currCost;
  int M, N, K;

  TimeCycle combineTime;
  TimeCycle assignTime;
  int numThreads;
  int blockId;
  int threadId;
  ThreadOption threadOption;
  int step;
} WorkerArgs;


#endif // CONST_H

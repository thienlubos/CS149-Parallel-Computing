make
# ./kmeans -t 1 > run.clusters.log
./kmeans -t 2 -n 64 > run.data.log
python plot.py
# ./kmeans -t 0 > run.seq.log
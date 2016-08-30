#include <cmath>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <time.h>
#include <omp.h>

using namespace std;

typedef vector<double> Point;
typedef vector<Point> Points;

// Gives random number in range [0..max_value]
unsigned int UniformRandom(unsigned int max_value) {
    unsigned int rnd = ((static_cast<unsigned int>(rand()) % 32768) << 17) |
                       ((static_cast<unsigned int>(rand()) % 32768) << 2) |
                       rand() % 4;
    return ((max_value + 1 == 0) ? rnd : rnd % (max_value + 1));
}

double Distance(const Point& point1, const Point& point2) {
    double distance_sqr = 0;
    size_t dimensions = point1.size();
    for (size_t i = 0; i < dimensions; ++i) {
        distance_sqr += (point1[i] - point2[i]) * (point1[i] - point2[i]);
    }
    return distance_sqr;
}

size_t FindNearestCentroid(const Points& centroids, const Point& point) {
    double min_distance = Distance(point, centroids[0]);
    size_t centroid_index = 0;
    for (size_t i = 1; i < centroids.size(); ++i) {
        double distance = Distance(point, centroids[i]);
        if (distance < min_distance) {
            min_distance = distance;
            centroid_index = i;
        }
    }
    return centroid_index;
}

// Calculates new centroid position as mean of positions of 3 random centroids
Point GetRandomPosition(const Points& centroids) {
    size_t K = centroids.size();
    int c1 = rand() % K;
    int c2 = rand() % K;
    int c3 = rand() % K;
    size_t dimensions = centroids[0].size();
    Point new_position(dimensions);
    for (size_t d = 0; d < dimensions; ++d) {
        new_position[d] = (centroids[c1][d] + centroids[c2][d] + centroids[c3][d]) / 3;
    }
    return new_position;
}

vector<size_t> KMeans(const Points& data, size_t K) {
    size_t data_size = data.size();
    size_t dimensions = data[0].size();
    vector<size_t> clusters(data_size);

    // Initialize centroids randomly at data points
    Points centroids(K);
    for (size_t i = 0; i < K; ++i) {
        centroids[i] = data[UniformRandom(data_size - 1)];
    }
	
	//omp_set_num_threads(3);
	int numTh = omp_get_max_threads();
	cout << numTh << endl;
	int portion = (data_size-1) / numTh + 1;

	

	vector<Points> centroids_loc(numTh);
    
    bool converged = false;
    while (!converged) {
        converged = true;


        vector<size_t> clusters_sizes(K);
	    vector<vector<size_t> > clusters_sizes_loc(numTh);
		for (int i=0; i < numTh; ++i) {
		     clusters_sizes_loc[i].resize(K);
			centroids_loc[i].assign(K, Point(dimensions));
	    }

        //#pragma omp parallel 
		{

        
        #pragma omp parallel for \
					firstprivate(data_size) \
					shared(centroids, clusters) \
					schedule(static) \
					reduction(&:converged)
	    for (int i = 0; i<data_size-1; ++i) {
		//for (int i = portion*tid; i < portion * (tid+1); ++i) {
			//if (i > data_size-1)
				//break;
			int tid = omp_get_thread_num();
			//cout << tid << ' ' << i << endl;
            size_t nearest_cluster = FindNearestCentroid(centroids, data[i]);
            if (clusters[i] != nearest_cluster) {
                clusters[i] = nearest_cluster;
                converged = false;
            }
        }



        if (!converged) {
            #pragma omp parallel for
			for (int i = 0; i<data_size-1; ++i) {
		        int tid = omp_get_thread_num();
				//cout << tid << ' ' << i << ' ' << omp_get_num_threads() << endl;
			//for (int i = portion*tid; i < portion * (tid+1); ++i) {
				//if (i > data_size-1)
					//break;
				for (size_t d = 0; d < dimensions; ++d) {
					(centroids)[clusters[i]][d] += data[i][d];
					//(centroids_loc[tid])[clusters[i]][d] += data[i][d];
				}
				++(clusters_sizes)[clusters[i]];
				//++(clusters_sizes_loc[tid])[clusters[i]];
			}
		}
		} //end of parallel section
		//cout << "checkpoint2" << endl;
	    
		if (!converged) {
		   //sum all
			/*for (int i=0; i<numTh; ++i) {
				for (int j=0; j<K; ++j) {
					for (size_t d = 0; d < dimensions; ++d)
						centroids[j][d] += centroids_loc[i][j][d];
					clusters_sizes[j] += clusters_sizes_loc[i][j];
				}
			}*/

			for (size_t i = 0; i < K; ++i) {
				if (clusters_sizes[i] != 0) {
					for (size_t d = 0; d < dimensions; ++d) {
						centroids[i][d] /= clusters_sizes[i];
					}
				}
			}
			//if there are not enough (K) clusters we create new one at random
			for (size_t i = 0; i < K; ++i) {
				if (clusters_sizes[i] == 0) {
					centroids[i] = GetRandomPosition(centroids);
				}
			}
		}
		//cout << "checkpoint3" << endl;
    }

    return clusters;
}

void ReadPoints(Points* data, ifstream& input) {
    size_t data_size;
    size_t dimensions;
    input >> data_size >> dimensions;
    data->assign(data_size, Point(dimensions));
    string s;
    for (size_t i = 0; i < data_size; ++i) {
        for (size_t d = 0; d < dimensions; ++d) {
            input >> s;
            (*data)[i][d] = atof(s.c_str());
        }
    }
}

void WriteOutput(const vector<size_t>& clusters, ofstream& output) {
    for (size_t i = 0; i < clusters.size(); ++i) {
        output << clusters[i] << '\n';
    }
}

int main(int argc , char** argv) {
	long t1 = clock();
    if (argc != 4) {
        std::printf("Usage: %s number_of_clusters input_file output_file\n", argv[0]);
        return 1;
    }
    size_t K = atoi(argv[1]);

    char* input_file = argv[2];
    ifstream input;
    input.open(input_file, ifstream::in);
    if(!input) {
        cerr << "Error: input file could not be opened\n";
        return 1;
    }

    Points data;
    ReadPoints(&data, input);
    input.close();

    char* output_file = argv[3];
    ofstream output;
    output.open(output_file, ifstream::out);
    if(!output) {
        cerr << "Error: output file could not be opened\n";
        return 1;
    }

    srand(123); // for reproducible results

    vector<size_t> clusters = KMeans(data, K);

    WriteOutput(clusters, output);
    output.close();
	long t2 = clock();

	//cout << t2 - t1 << endl;

    return 0;
}
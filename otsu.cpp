#include <cassert>
#include <iostream>
#include <omp.h>

#include "otsu.h"

void build_frequency_table(long long* frequency, char* image, int size, int threads_count, int chunk_size) {
	for (int i = 0; i < COLORS; i++) {
		frequency[i] = 0;
	}
#pragma omp parallel if (threads_count != -1)
	{
#pragma omp for
		for (int i = 0; i < size; i++) {
			int normalized = image[i] < 0 ? image[i] + 256 : image[i];
#pragma omp atomic
			frequency[normalized]++;
		}
	}
}

void build_table(double** main_table, long long* frequency, int threads_count, int chunk_size) {
#pragma omp parallel if (threads_count != -1)
	{
#pragma omp for
		for (int i = 1; i < COLORS; i++) {
			long long freq = 0;
			long long s = 0;
			for (int j = i; j < COLORS; j++) {
				long long local_freq = frequency[j];
				freq += frequency[j];
				s += j * frequency[j];
				if (freq != 0)
					main_table[i][j] = (double)s * s / freq;
			}
		}
	}
}

Threshold get_best_thresholds(double** main_table, int threads_count, int chunk_size) {
	assert(LEVELS == 4);
	bool is_multithread = (threads_count != -1);
	threads_count = threads_count != -1 ? threads_count : 1;
	Threshold* local = new Threshold[threads_count];
#pragma omp parallel if (is_multithread)
	{
#pragma omp for
		for (int i = 1; i < COLORS; i++) {
			for (int j = i + 1; j < COLORS; j++) {
				for (int k = j + 1; k < COLORS - 1; k++) {
					double cur_sigma = main_table[1][i] + main_table[i + 1][j] + main_table[j + 1][k] + main_table[k + 1][COLORS - 1];
					Threshold& local_sigma = local[omp_get_thread_num()];
					if (local_sigma.sigma < cur_sigma) {
						local_sigma = Threshold(cur_sigma, i, j, k);
					}
				}
			}
		}
	}
	Threshold best_threshold;
	for (int i = 0; i < threads_count; i++) {
		if (best_threshold.sigma < local[i].sigma) {
			best_threshold = local[i];
		}
	}
	return best_threshold;
}

Threshold get_threshold(char* image, int size, int threads_count, int chunk_size) {
	if (threads_count > 0)
		omp_set_num_threads(threads_count);
#pragma omp parallel
	{
		threads_count = omp_get_num_threads();
	}
	long long* frequency = new long long[COLORS];
	double** main_table = new double* [COLORS];
	for (int i = 0; i < COLORS; i++) {
		main_table[i] = new double[COLORS];
	}
	build_frequency_table(frequency, image, size, threads_count, chunk_size);
	build_table(main_table, frequency, threads_count, chunk_size);
	Threshold res = get_best_thresholds(main_table, threads_count, chunk_size);

	delete[] frequency;
	for (int i = 0; i < COLORS; i++) {
		delete[] main_table[i];
	}
	delete[] main_table;

	return res;
}

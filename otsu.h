#pragma once

const int COLORS = 256;
const int LEVELS = 4;

struct Threshold {
	double sigma;
	int t0, t1, t2;

	Threshold() : sigma(0), t0(0), t1(0), t2(0) {}
	Threshold(double sigma, int t0, int t1, int t2) : sigma(sigma), t0(t0), t1(t1), t2(t2) {}
};

void build_frequency_table(long long* frequency, char* image, int size, int threads_count, int chunk_size = 1);
void build_table(double** main_table, long long* frequency, int threads_count, int chunk_size = 1);
Threshold get_best_thresholds(double** main_table, int threads_count, int chunk_size = 1);
Threshold get_threshold(char* image, int size, int threads_count, int chunk_size = 1);
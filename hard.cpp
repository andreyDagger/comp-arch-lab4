#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif // !_CRT_SECURE_NO_WARNINGS

#pragma GCC optimize("Ofast")
#pragma GCC optimize("O2");

#include <iostream>
#include <omp.h>
#include <fstream>
#include <string>
#include <cstring>
#include <stdexcept>

#include "otsu.h"

#define MAX_IMAGE_SIZE (1 << 30) // Exactly one gb
#define MAX_TREADS_COUNT 1000

using namespace std;

ifstream open_for_input(char* filename) {
	ifstream file;
	file.exceptions(file.exceptions() | ios::failbit);
	file.open(filename, ios::binary);
	return file;
}

ofstream open_for_output(char* filename) {
	ofstream file;
	file.exceptions(file.exceptions() | ios::failbit);
	file.open(filename);
	return file;
}

void print_file_error_message_and_exit() {
	if (ios::eofbit) {
		cerr << "End-Of-File reached while performing an extracting operation on an input stream.\n";
	}
	else if (ios::failbit) {
		cerr << "The last input operation failed because of an error related to the internal logic of the operation itself.\n";
	}
	else if (ios::badbit) {
		cerr << "Error due to the failure of an input/output operation on the stream buffer.\n";
	}
	else {
		cerr << "Unknown exception\n";
	}
	exit(1);
}

int main(int argc, char** argv)
{
	if (argc < 4) {
		std::cerr << "Must be at least 4 arguments, but only " << argc << " were received\n";
		exit(1);
	}

	int threads_count = atoi(argv[1]);
	if (threads_count < -1 || threads_count > MAX_TREADS_COUNT) {
		cerr << "Number of threads must lie in range [1, 1000] or [-1, -1]\n";
		exit(1);
	}

	ifstream input_file;
	ofstream output_file;
	try {
		input_file = open_for_input(argv[2]);
		output_file = open_for_output(argv[3]);
	}
	catch (ios_base::failure e) {
		cerr << e.what() << "\n" << strerror(errno) << "\n";
		exit(1);
	}

	char* image = nullptr;
	unsigned int width, height, max_color;
	string type;

	try {
		input_file >> type >> width >> height >> max_color;
		if (type != "P5" || max_color != 255 || width == 0 || height == 0 || width * 1ll * height > MAX_IMAGE_SIZE) {
			throw invalid_argument("wrong file format");
		}
		image = new char[width * height];
		input_file.read(image, 1); // skipping \n
		input_file.read(image, width * 1ll * height);
	}
	catch (ios_base::failure e) {
		cerr << e.what() << "\n";
		print_file_error_message_and_exit();
	}
	catch (invalid_argument e) {
		cerr << e.what() << "\n";
		exit(1);
	}

	Threshold threshold;

	int epochs = 50;
	double start_time = omp_get_wtime();
	for (int i = 0; i < epochs; i++) {
		threshold = get_threshold(image, width * height, threads_count);
	}
	double end_time = omp_get_wtime();
	double delta = end_time - start_time;
	printf("Time(% i thread(s)) : % g ms\n", threads_count, delta * 1000 / epochs);
	printf("%u %u %u\n", threshold.t0, threshold.t1, threshold.t2);

	try {
		output_file.write("P5\n", 3);
		output_file.write(to_string(width).c_str(), to_string(width).size());
		output_file.write(" ", 1);
		output_file.write(to_string(height).c_str(), to_string(height).size());
		output_file.write("\n255\n", 5);
		for (int i = 0; i < width * height; i++) {
			int pixel = image[i];
			if (pixel <= threshold.t0)
				pixel = 0;
			else if (pixel <= threshold.t1)
				pixel = 84;
			else if (pixel <= threshold.t2)
				pixel = 170;
			else
				pixel = 255;
			char temp[1];
			temp[0] = pixel >= 128 ? pixel - 256 : pixel;
			output_file.write(temp, 1);
		}

		input_file.close();
		output_file.close();
	}
	catch (ios_base::failure e) {
		cerr << e.what() << "\n";
		print_file_error_message_and_exit();
	}

	delete[] image;

	return 0;
}

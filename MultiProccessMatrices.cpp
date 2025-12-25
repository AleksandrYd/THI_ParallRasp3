#include <iostream>
#include <string>
#include <cctype>
#include <vector>
#include <algorithm>
#include <Windows.h>
#include <conio.h>
#include <chrono>

#include <mpi.h>

#include "cryptlib.h"
#include "md5.h"
#include "hex.h"
#include "filters.h"

#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1

using namespace CryptoPP;

int n = 1024;

void serial_mul(float* A, float* B, float* C)
{
	float summa;

	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < n; j++)
		{
			summa = 0;
			for (int k = 0; k < n; k++)
			{
				summa += A[i * n + k] * B[k * n + j];
			}
			C[i * n + j] = summa;
		}
	}
}


bool check_matrices_function(float* C_multithreads, float* C_serial)
{
	for (int i = 0; i < 10; i++)
	{
		for (int j = 0; j < 10; j++)
		{
			if (C_multithreads[i * n + j] != C_serial[i * n + j])
				return false;
		}
	}
	return true;
}

void thread_proccess(float* A, float* B, float* C, int block, int thread_num, int n)
{
	float summa;
	int first_row = block * thread_num;
	int last_row = block * (thread_num + 1);

	for (int i = first_row; i < last_row; i++)
	{
		for (int j = 0; j < n; j++)
		{
			summa = 0;
			for (int k = 0; k < n; k++)
			{

				summa += A[i * n + k] * B[k * n + j];
			}
			C[i * n + j] = summa;
		}
	}
}


int main(int* argc, char** argv)
{
	MPI_Init(argc, &argv);
	int numtasks, rank;
	float* C_serial;
	float* C_multithreads;
	float* C_multithreads_root;
	float* A;
	float* B;
	float* A_local;
	float* C_local;

	A = new float[n * n];
	B = new float[n * n];
	

	C_serial = new float[n * n];
	C_multithreads = new float[n * n];
	C_multithreads_root = new float[n * n];

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

	A_local = new float[n * n / numtasks];
	C_local = new float[n * n / numtasks];

	srand(time(NULL) % 100);
	if (rank == 0)
	{
		for (int i = 0; i < n; i++)
		{
			for (int j = 0; j < n; j++)
			{
				A[i * n + j] = rand() % 10;
				B[i * n + j] = rand() % 10;

				C_multithreads[i * n + j] = 0;
			}
		}
	}

	float average = 0;

	MPI_Scatter(&A[0], n * n / numtasks, MPI_FLOAT, &A_local[0], n * n / numtasks, MPI_FLOAT, 0, MPI_COMM_WORLD);

	MPI_Bcast(&B[0], n * n, MPI_FLOAT, 0, MPI_COMM_WORLD);

	MPI_Bcast(&C_multithreads[0], n * n, MPI_FLOAT, 0, MPI_COMM_WORLD);
	float duration_f = 0;

	MPI_Barrier(MPI_COMM_WORLD);
	for (int i = 0; i < 10; i++)
	{
		int block = n / numtasks;

		auto start_time = std::chrono::high_resolution_clock::now();

		float summa;
		int first_row = block * rank;
		int last_row = block * (rank + 1);
		int local_index = 0;

		for (int i = first_row; i < last_row; i++)
		{
			for (int j = 0; j < n; j++)
			{
				summa = 0;
				for (int k = 0; k < n; k++)
				{
					summa += A_local[local_index * n + k] * B[k * n + j];
				}
				C_local[local_index * n + j] = summa;
			}
			local_index++;
		}

		auto end_time = std::chrono::high_resolution_clock::now();
		std::chrono::microseconds duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

		duration_f += (float)duration.count() / 1000000;
		std::cout << "[MULTIPROCCESS MATRICES] Calculation time  " << rank << "-th proccess, " << i << "-th attempt:  " << (float)duration.count() / (float)1000000 << " sec\n";
	}


	MPI_Gather(&C_local[0], n * n / numtasks, MPI_FLOAT, &C_multithreads_root[0], n * n / numtasks, MPI_FLOAT, 0, MPI_COMM_WORLD);
	MPI_Reduce(&duration_f, &average, 1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD);


	MPI_Barrier(MPI_COMM_WORLD);
	if (rank == 0)
	{
		std::cout << "[MULTIPROCCESS MATRICES] Average time of multiproccess calculation: " << average / ((float)10 * 8) << '\n';
		serial_mul(A, B, C_serial);
		if (check_matrices_function(C_multithreads_root, C_serial))
		{
			std::cout << "Multiproccess calculation approved";
		}
		else
		{
			std::cout << "Multiproccess calculation false";
		}
	}
	MPI_Finalize();
}
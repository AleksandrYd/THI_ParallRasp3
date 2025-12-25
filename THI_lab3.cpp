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

int stop_signal = 0;

bool check_word(std::string& word, CryptoPP::Weak1::MD5* hash, std::string& in_hash)
{
    std::string digest;

    hash->Update((const CryptoPP::byte*)&word[0], word.size());
    digest.resize(hash->DigestSize());
    hash->Final((CryptoPP::byte*)&digest[0]);

    std::string computed_hex;
    CryptoPP::StringSource(digest, true,
        new CryptoPP::HexEncoder(
            new CryptoPP::StringSink(computed_hex)
        )
    );

    for (auto& c : computed_hex) c = toupper(c);
    for (auto& c : in_hash) c = toupper(c);

    if (computed_hex == in_hash)
    {
        return 1;
    }
    return 0;
}

std::string brute_force(int begin, int block, CryptoPP::Weak1::MD5* hash, std::string& in_hash, int length, char* alphabet)
{
    std::string word1;
    std::string word2;
    std::string word3;
    std::string word4;
    std::string word5;

    for (int i1 = begin; i1 < begin + block; i1++)
    {

        if (stop_signal)
            return std::string(1, ' ');

        word1 = std::string(1, alphabet[i1]);

        if (length > 1)
        {
            for (int i2 = 0; i2 < 62; i2++)
            {

                if (stop_signal)
					return std::string(1, ' ');

                word2 = word1 + std::string(1, alphabet[i2]);

                if (length > 2)
                {
                    for (int i3 = 0; i3 < 62; i3++)
                    {

                        if (stop_signal)
							return std::string(1, ' ');

                        word3 = word2 + std::string(1, alphabet[i3]);

                        if (length > 3)
                        {
                            for (int i4 = 0; i4 < 62; i4++)
                            {

                                if (stop_signal)
									return std::string(1, ' ');

                                word4 = word3 + std::string(1, alphabet[i4]);

                                if (length > 4)
                                {
                                    for (int i5 = 0; i5 < 62; i5++)
                                    {

                                        if (stop_signal)
											return std::string(1, ' ');

                                        word5 = word4 + std::string(1, alphabet[i5]);
                                        if (check_word(word5, hash, in_hash))
                                            return word5;
                                    }
                                }
                                else
                                {
                                    if (check_word(word4, hash, in_hash))
                                        return word4;
                                }

                            }
                        }
                        else
                        {
                            if (check_word(word3, hash, in_hash))
                                return word3;
                        }

                    }
                }
                else
                {
                    if (check_word(word2, hash, in_hash))
                        return word2;
                }

            }
        }
        else
        {
            if (check_word(word1, hash, in_hash))
                return word1;
        }

    }
	return std::string(1, ' ');

}

int main(int* argc, char** argv)
{
	MPI_Init(argc, &argv);
	int numtasks, rank;

	int length;
	std::string in_hash;
	std::string result;


	char alphabet[62] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
						'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
						'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
						'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
						'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z' };

	setlocale(LC_ALL, "Russian");

	
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

	if (rank == 0)									// Если главный процесс (процесс 0)
	{
		std::cout << "Enter password length...\n";
		std::cin >> length;
		std::cout << "Enter MD5 hash...\n";
		std::cin >> in_hash;
	}

	MPI_Bcast(&length, 1, MPI_INT, 0, MPI_COMM_WORLD);
	int hash_size = in_hash.size();
	if (rank == 0) hash_size = in_hash.size();
	MPI_Bcast(&hash_size, 1, MPI_INT, 0, MPI_COMM_WORLD);

	if (rank != 0) in_hash.resize(hash_size);
	MPI_Bcast(&in_hash[0], hash_size, MPI_CHAR, 0, MPI_COMM_WORLD);


	if (rank == 0)									// Если главный процесс (процесс 0)
	{
		CryptoPP::Weak1::MD5* hash = new CryptoPP::Weak1::MD5;
		std::string word = brute_force(0, 8, hash, in_hash, length, alphabet);
		

		 if (word != std::string(1, ' ') && stop_signal == 0)
		{
			stop_signal = 1;
			printf("Process %d: the word was found\n", rank);
			printf("The found word: %s.\nThe entered hash: %s\n", word.c_str(), in_hash.c_str());
		}
	}
	else if (rank == 1)								// Если процесс 1					
	{
		CryptoPP::Weak1::MD5* hash = new CryptoPP::Weak1::MD5;
		std::string word = brute_force(8, 8, hash, in_hash, length, alphabet);
		

		if (word != std::string(1, ' ') && stop_signal == 0)
		{
			stop_signal = 1;
			printf("Process %d: the word was found\n", rank);
			printf("The found word: %s.\nThe entered hash: %s\n", word.c_str(), in_hash.c_str());
		}
	}
	else if (rank == 2)								// Если процесс 2					
	{
		CryptoPP::Weak1::MD5* hash = new CryptoPP::Weak1::MD5;
		std::string word = brute_force(16, 8, hash, in_hash, length, alphabet);
		

		if (word != std::string(1, ' ') && stop_signal == 0)
		{
			stop_signal = 1;
			printf("Process %d: the word was found\n", rank);
			printf("The found word: %s.\nThe entered hash: %s\n", word.c_str(), in_hash.c_str());
		}
	}
	else if (rank == 3)								// Если процесс 3					
	{
		CryptoPP::Weak1::MD5* hash = new CryptoPP::Weak1::MD5;
		std::string word = brute_force(24, 8, hash, in_hash, length, alphabet);
		

		if (word != std::string(1, ' ') && stop_signal == 0)
		{
			stop_signal = 1;
			printf("Process %d: the word was found\n", rank);
			printf("The found word: %s.\nThe entered hash: %s\n", word.c_str(), in_hash.c_str());
		}
	}
	else if (rank == 4)								// Если процесс 4					
	{
		CryptoPP::Weak1::MD5* hash = new CryptoPP::Weak1::MD5;
		std::string word = brute_force(32, 8, hash, in_hash, length, alphabet);
		

		if (word != std::string(1, ' ') && stop_signal == 0)
		{
			stop_signal = 1;
			printf("Process %d: the word was found\n", rank);
			printf("The found word: %s.\nThe entered hash: %s\n", word.c_str(), in_hash.c_str());
		}
	}
	else if (rank == 5)								// Если процесс 5					
	{
		CryptoPP::Weak1::MD5* hash = new CryptoPP::Weak1::MD5;
		std::string word = brute_force(40, 8, hash, in_hash, length, alphabet);
		

		if (word != std::string(1, ' ') && stop_signal == 0)
		{
			stop_signal = 1;
			printf("Process %d: the word was found\n", rank);
			printf("The found word: %s.\nThe entered hash: %s\n", word.c_str(), in_hash.c_str());
		}
	}
	else if (rank == 6)								// Если процесс 6					
	{
		CryptoPP::Weak1::MD5* hash = new CryptoPP::Weak1::MD5;
		std::string word = brute_force(48, 8, hash, in_hash, length, alphabet);
		

		if (word != std::string(1, ' ') && stop_signal == 0)
		{
			stop_signal = 1;
			printf("Process %d: the word was found\n", rank);
			printf("The found word: %s.\nThe entered hash: %s\n", word.c_str(), in_hash.c_str());
		}
	}
	else if (rank == 7)								// Если процесс 7						
	{
		CryptoPP::Weak1::MD5* hash = new CryptoPP::Weak1::MD5;
		std::string word = brute_force(56, 6, hash, in_hash, length, alphabet);
		

		if (word != std::string(1, ' ') && stop_signal == 0)
		{
			stop_signal = 1;
			printf("Process %d: the word was found\n", rank);
			printf("The found word: %s.\nThe entered hash: %s\n", word.c_str(), in_hash.c_str());
		}
	}

	MPI_Finalize();

	
}

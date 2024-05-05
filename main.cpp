#include <iostream>
#include <mpi.h>
#include "students.h"

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " A" << std::endl;
        return 1;
    }
    int N = std::stoi(argv[1]);
    int A = std::stoi(argv[2]);

    MPI::Init();
    int rank = MPI::COMM_WORLD.Get_rank(); // Get the rank of the current process
    int size = MPI::COMM_WORLD.Get_size(); // Get the total number of processes

    Students students(N, A, rank);

    MPI::Finalize();

    return 0;
}
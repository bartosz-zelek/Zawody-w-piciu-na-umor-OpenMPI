#include <iostream>
#include <mpi.h>
#include "students.h"

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::cout << "Usage: " << argv[0] << " A" << std::endl;
        return 1;
    }
    int A = std::stoi(argv[1]);

    MPI::Init();
    int rank = MPI::COMM_WORLD.Get_rank(); // Get the rank of the current process
    int size = MPI::COMM_WORLD.Get_size(); // Get the total number of processes

    Students students(size, A, rank);
    students.want_to_drink();

    int response;
    MPI::Status response_status;

    // #pragma omp parallel num_threads(2)
    while (true)
    {
        // #pragma omp single
        MPI::COMM_WORLD.Recv(&response, 1, MPI::INT, MPI::ANY_SOURCE, MPI::ANY_TAG, response_status);

        switch (response_status.Get_tag())
        {
        case MessageType::REQUEST:
            // #pragma omp single
            students.handle_request(response_status.Get_source(), response);
            break;
        case MessageType::REPLY:
            // #pragma omp single
            students.handle_reply(response_status.Get_source(), response);
            break;
        default:
            break;
        }
    }

    MPI::Finalize();

    return 0;
}
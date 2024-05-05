#pragma once

#include <vector>
#include <mpi.h>
#include "common.h"

class Students
{
    enum class StudentsState
    {
        WANT_TO_DRINK = 42,
        DRINKING,
        IDLE
    };

    StudentsState state;
    unsigned global_counter;
    unsigned local_counter;
    std::vector<unsigned> reply_counter;
    std::vector<unsigned> defer_counter;
    unsigned rank;
    unsigned N;
    unsigned A;

public:
    Students(unsigned _N, unsigned _A, unsigned _rank) : reply_counter(N),
                                                         defer_counter(N),
                                                         state(StudentsState::IDLE),
                                                         global_counter(0),
                                                         local_counter(0),
                                                         rank(_rank),
                                                         N(_N),
                                                         A(_A)
    {
        std::fill(reply_counter.begin(), reply_counter.end(), 0);
        std::fill(defer_counter.begin(), defer_counter.end(), 0);
    }

    void want_to_drink()
    {
        std::cout << "[" << local_counter << "] Students " << rank << " wants to drink" << std::endl;
        state = StudentsState::WANT_TO_DRINK;
        local_counter = global_counter + 1;
        for (unsigned i = 0; i < N; ++i)
        {
            if (i != rank)
            {
                MPI::COMM_WORLD.Send(&local_counter, 1, MPI::INT, i, MessageType::REQUEST);
                ++reply_counter[i];
            }
        }
    }

    void handle_request(unsigned Y_rank, unsigned Y_counter)
    {
        std::cout << "[" << local_counter << "] Students " << rank << " received request from " << Y_rank << " with counter " << Y_counter << std::endl;
        global_counter = std::max(global_counter, Y_counter);
        if (state == StudentsState::DRINKING || (state == StudentsState::WANT_TO_DRINK) && (local_counter < Y_counter || (local_counter == Y_counter && rank < Y_rank)))
        {
            ++defer_counter[Y_rank];
        }
        else
        {
            unsigned no_replays = 1;
            MPI::COMM_WORLD.Send(&no_replays, 1, MPI::INT, Y_rank, MessageType::REPLY);
        }
    }

    unsigned count_not_drinking()
    {
        unsigned not_drinking = 0;
        for (unsigned i = 0; i < N; ++i)
        {
            if (i != rank && reply_counter[i] == 0)
            {
                ++not_drinking;
            }
        }
        return not_drinking;
    }

    void handle_reply(unsigned Y_rank, unsigned no_replays)
    {
        std::cout << "[" << local_counter << "] Students " << rank << " received reply from " << Y_rank << " with no_replays " << no_replays << std::endl;
        reply_counter[Y_rank] -= no_replays;
        if (state == StudentsState::WANT_TO_DRINK && count_not_drinking() >= N - A)
        {
            state = StudentsState::DRINKING;
            std::cout << "Students " << rank << " is drinking" << std::endl;
        }
    }

    void stop_drinking()
    {
        std::cout << "[" << local_counter << "] Students " << rank << " stopped drinking" << std::endl;
        state = StudentsState::IDLE;
        for (unsigned i = 0; i < N; ++i)
        {
            if (i != rank)
            {
                if (defer_counter[i] > 0)
                {
                    MPI::COMM_WORLD.Send(&defer_counter[i], 1, MPI::INT, i, MessageType::REPLY);
                    defer_counter[i] = 0;
                }
            }
        }
    }
};
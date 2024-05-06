#pragma once

#include <vector>
#include <mpi.h>
#include "common.h"
#include <unistd.h>

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
    Students(unsigned _N, unsigned _A, unsigned _rank) : state(StudentsState::IDLE),
                                                         global_counter(0),
                                                         local_counter(0),
                                                         rank(_rank),
                                                         N(_N),
                                                         A(_A)
    {
        reply_counter.resize(N);
        defer_counter.resize(N);
        std::fill(reply_counter.begin(), reply_counter.end(), 0);
        std::fill(defer_counter.begin(), defer_counter.end(), 0);
    }

    void want_to_drink()
    {
        std::cerr << "[" << global_counter << "] Students " << rank << " wants to drink" << std::endl;
        state = StudentsState::WANT_TO_DRINK;
        local_counter = global_counter + 1;
        for (unsigned i = 0; i < N; ++i)
        {
            if (i != rank)
            {
                std::cerr << "[" << local_counter << "] Students " << rank << " sent request to " << i << std::endl;
                MPI::COMM_WORLD.Send(&local_counter, 1, MPI::INT, i, MessageType::REQUEST);
                ++reply_counter[i];
            }
        }
        conditional_drink();
    }

    void handle_request(unsigned Y_rank, unsigned Y_counter)
    {
        std::cerr << "[" << global_counter << "] Students " << rank << " received request from " << Y_rank << " with counter " << Y_counter << std::endl;
        global_counter = std::max(global_counter, Y_counter);
        if (state == StudentsState::DRINKING || ((state == StudentsState::WANT_TO_DRINK) && (local_counter < Y_counter || (local_counter == Y_counter && rank < Y_rank))))
        {
            std::cerr << "[" << global_counter << "] Students " << rank << " deferred reply to " << Y_rank << std::endl;
            ++defer_counter[Y_rank];
        }
        else
        {
            std::cerr << "[" << global_counter << "] Students " << rank << " replied to " << Y_rank << std::endl;
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
        std::cerr << "[" << global_counter << "] Students " << rank << " count not drinking " << not_drinking << std::endl;
        return not_drinking;
    }

    void conditional_drink()
    {
        if (state == StudentsState::WANT_TO_DRINK && count_not_drinking() >= N - A)
        {
            state = StudentsState::DRINKING;
            std::cerr << "Students " << rank << " is drinking" << std::endl;
            sleep(5);
            stop_drinking();
        }
    }

    void handle_reply(unsigned Y_rank, unsigned no_replays)
    {
        std::cerr << "[" << global_counter << "] Students " << rank << " received reply from " << Y_rank << " with no_replays " << no_replays << std::endl;
        reply_counter[Y_rank] -= no_replays;
        conditional_drink();
    }

    void stop_drinking()
    {
        std::cerr << "[" << global_counter << "] Students " << rank << " stopped drinking" << std::endl;
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
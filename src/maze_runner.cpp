#include "maze_runner_common.h"

#include <cpen333/process/shared_memory.h>
#include <cpen333/process/mutex.h>
#include <cstring>
#include <chrono>
#include <thread>
#include <random>

class MazeRunner {

    cpen333::process::shared_object<SharedData> memory_;
    cpen333::process::mutex mutex_;

    // local copy of maze
    MazeInfo minfo_;

    // runner info
    size_t idx_;   // runner index
    int loc_[2];   // current location

    std::default_random_engine rnd;
    std::uniform_int_distribution<int> diceDist;

public:

    MazeRunner() : memory_(MAZE_MEMORY_NAME), mutex_(MAZE_MUTEX_NAME),
                   minfo_(), idx_(0), loc_() {

        // copy maze contents
        minfo_ = memory_->minfo;

        {
            // protect access of number of runners
            std::lock_guard<decltype(mutex_)> lock(mutex_);
            idx_ = memory_->rinfo.nrunners;
            memory_->rinfo.nrunners++;
        }

        // get current location
        loc_[COL_IDX] = memory_->rinfo.rloc[idx_][COL_IDX];
        loc_[ROW_IDX] = memory_->rinfo.rloc[idx_][ROW_IDX];

        rnd = std::default_random_engine((unsigned int) std::chrono::system_clock::now().time_since_epoch().count());
        diceDist =  std::uniform_int_distribution<int>(0,4);
    }

    bool movable(int col, int row){
        return atExit(col, row) || ((minfo_.maze[col][row] != WALL_CHAR) && !isOccupied(col, row));
    }

    bool isOccupied(int col, int row) {
        {
            std::lock_guard<decltype(mutex_)> lock(mutex_);
            for (int runnerIdx = 0; runnerIdx < memory_->rinfo.nrunners; ++ runnerIdx) {
                if ((memory_->rinfo.rloc[runnerIdx][COL_IDX] == col) && (memory_->rinfo.rloc[runnerIdx][ROW_IDX] == row)) {
                    return true;
                }
            }
            return false;
        }
    }

    bool atExit(int col, int row) {
        return (minfo_.maze[col][row] == EXIT_CHAR);
    }

    /**
     * Solves the maze, taking time between each step so we can actually see progress in the UI
     * @return 1 for success, 0 for failure, -1 to quit
     */
    int go() {
        while (!atExit(loc_[COL_IDX], loc_[ROW_IDX])) {
            randomMove();
        }

        if (atExit(loc_[COL_IDX], loc_[ROW_IDX])){
            return 1;
        }
        else {
            return 0;
        }
        // failed to find exit
        return 0;
    }

    void randomMove() {
        int diceRoll = diceDist(rnd);
        switch (diceRoll) {
            case 0: {
                if (movable(loc_[COL_IDX] + 1, loc_[ROW_IDX])) {
                    loc_[COL_IDX] = loc_[COL_IDX] + 1;
                }
                break;
            }
            case 1: {
                if (movable(loc_[COL_IDX], loc_[ROW_IDX] + 1)) {
                    loc_[ROW_IDX] = loc_[ROW_IDX] + 1;
                }
                break;
            }
            case 2: {
                if (movable(loc_[COL_IDX] - 1, loc_[ROW_IDX])) {
                    loc_[COL_IDX] = loc_[COL_IDX] - 1;
                }
                break;
            }
            case 3: {
                if (movable(loc_[COL_IDX], loc_[ROW_IDX] - 1)) {
                    loc_[ROW_IDX] = loc_[ROW_IDX] - 1;
                }
                break;
            }


        }
        {
            std::lock_guard<decltype(mutex_)> lock(mutex_);
            memory_->rinfo.rloc[idx_][COL_IDX] = loc_[COL_IDX];
            memory_->rinfo.rloc[idx_][ROW_IDX] = loc_[ROW_IDX];
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

};

int main() {

    MazeRunner runner;
    runner.go();

    return 0;
}
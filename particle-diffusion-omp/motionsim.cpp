/***************************************************************************
 *
Copyright 2020 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 * motionsim.cpp
 *
 * Description:
 *   This code sample will implement a simple example of a Monte Carlo
 *   simulation of the diffusion of water molecules in tissue.
 *
 **************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <chrono>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <iostream>


// Helper functions

// This function displays correct usage and parameters
void usage(std::string programName) {
  std::cout << " Incorrect number of parameters " << std::endl;
  std::cout << " Usage: ";
  std::cout << programName << " <Numbeof Iterations> " << std::endl << std::endl;
}

// This function prints a 2D matrix
template <typename T>
void print_matrix(T** matrix, size_t size_X, size_t size_Y) {
  std::cout << std::endl;
  for (size_t i = 0; i < size_X; ++i) {
    for (size_t j = 0; j < size_Y; ++j) {
      std::cout << std::setw(3) << matrix[i][j] << " ";
    }
    std::cout << std::endl;
  }
}

// This function prints a vector
template <typename T>
void print_vector(T* vector, size_t n) {
  std::cout << std::endl;
  for (size_t i = 0; i < n; ++i) {
    std::cout << vector[i] << " ";
  }
  std::cout << std::endl;
}


// This function distributes simulation work across workers
void motion_device(float* particleX, float* particleY,
                   float* randomX, float* randomY, int** grid, int grid_size,
                   size_t n_particles, unsigned int nIterations, float radius,
                   size_t* map) {

  srand(17);
  
  // Scale of random numbers
  const size_t scale = 100;

  // Compute vectors of random values for X and Y directions
  for (size_t i = 0; i < n_particles * nIterations; i++) {
    randomX[i] = rand() % scale;
    randomY[i] = rand() % scale;
  }

  const size_t MAP_SIZE = n_particles * grid_size * grid_size;

#pragma omp target map(to: randomX[0:n_particles * nIterations], randomY[0:n_particles * nIterations]) \
                   map(tofrom: particleX[0:n_particles], particleY[0:n_particles], map[0:MAP_SIZE])
{
  #pragma omp teams distribute parallel for simd thread_limit(256) 
  for (int ii = 0; ii < n_particles; ii++) {

    size_t randnumX = 0;
    size_t randnumY = 0;
    float displacementX = 0.0f;
    float displacementY = 0.0f;

    // Start iterations
    // Each iteration:
    //  1. Updates the position of all water molecules
    //  2. Checks if water molecule is inside a cell or not.
    //  3. Updates counter in cells array
    size_t iter = 0;
    while (iter < nIterations) {
            // Computes random displacement for each molecule
            // This example shows random distances between
            // -0.05 units and 0.05 units in both X and Y directions
            // Moves each water molecule by a random vector

            randnumX = randomX[iter * n_particles + ii];
            randnumY = randomY[iter * n_particles + ii];

            // Transform the scaled random numbers into small displacements
            displacementX = (float)randnumX / 1000.0f - 0.0495;
            displacementY = (float)randnumY / 1000.0f - 0.0495;

            // Move particles using random displacements
            particleX[ii] += displacementX;
            particleY[ii] += displacementY;

            // Compute distances from particle position to grid point
            float dX = particleX[ii] - truncf(particleX[ii]);
            float dY = particleY[ii] - truncf(particleY[ii]);

            // Compute grid point indices
            int iX = floorf(particleX[ii]);
            int iY = floorf(particleY[ii]);

            // Check if particle is still in computation grid
            if ((particleX[ii] < grid_size) &&
        	(particleY[ii] < grid_size) && (particleX[ii] >= 0) &&
        	(particleY[ii] >= 0)) {
        	    // Check if particle is (or remained) inside cell.
        	    // Increment cell counter in map array if so
        	    if ((dX * dX + dY * dY <= radius * radius)) {
        		    // The map array is organized as (particle, y, x)
        		    map[ii * grid_size * grid_size + iY * grid_size + iX]++;
        	    }
            }

            iter++;

       }  // Next iteration
     }
  }



  // For every cell in the grid, add all the counters from different
  // particles (workers) which are stored in the 3rd dimension of the 'map'
  // array
  for (size_t i = 0; i < n_particles; ++i) {
    for (size_t y = 0; y < grid_size; y++) {
      for (size_t x = 0; x < grid_size; x++) {
        if (map[i * grid_size * grid_size + y * grid_size + x] > 0) {
          grid[y][x] += map[i * grid_size * grid_size + y * grid_size + x];
        }
      }
    }
  }  // End loop for number of particles

}  // End of function motion_device()

int main(int argc, char* argv[]) {
  // Cell and Particle parameters
  const size_t grid_size = 21;    // Size of square grid
  const size_t n_particles = 147456;  // Number of particles
  const float radius = 0.5;       // Cell radius = 0.5*(grid spacing)

  // Default number of operations
  size_t nIterations = 50;

  // Read command-line arguments
  try {
    nIterations = std::stoi(argv[1]);
  }

  catch (...) {
    usage(argv[0]);
    return 1;
  }

  // Allocate arrays

  // Stores a grid of cells
  int** grid = new int*[grid_size];
  for (size_t i = 0; i < grid_size; i++) grid[i] = new int[grid_size];

  // Stores all random numbers to be used in the simulation
  float* randomX = new float[n_particles * nIterations];
  float* randomY = new float[n_particles * nIterations];

  // Stores X and Y position of particles in the cell grid
  float* particleX = new float[n_particles];
  float* particleY = new float[n_particles];

  // 'map' array replicates grid to be used by each particle
  const size_t MAP_SIZE = n_particles * grid_size * grid_size;
  size_t* map = new size_t[MAP_SIZE];

  // Initialize arrays
  for (size_t i = 0; i < n_particles; i++) {
    // Initial position of particles in cell grid
    particleX[i] = 10.0;
    particleY[i] = 10.0;

    for (size_t y = 0; y < grid_size; y++) {
      for (size_t x = 0; x < grid_size; x++) {
        map[i * grid_size * grid_size + y * grid_size + x] = 0;
      }
    }
  }

  for (size_t y = 0; y < grid_size; y++) {
    for (size_t x = 0; x < grid_size; x++) {
      grid[y][x] = 0;
    }
  }

  // Start timers
  auto start = std::chrono::steady_clock::now();

  // Call simulation function
  motion_device(particleX, particleY, randomX, randomY, grid, grid_size,
                n_particles, nIterations, radius, map);


  auto end = std::chrono::steady_clock::now();
  auto time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
                  .count();
  std::cout << std::endl;
  std::cout << "Time: " << time << " ms " << std::endl;
  std::cout << std::endl;

  // Displays final grid only if grid small.
  if (grid_size <= 64) {
    std::cout << "\n ********************** OUTPUT GRID: " << std::endl;
    print_matrix<int>(grid, grid_size, grid_size);
  }

  // Cleanup
  for (size_t i = 0; i < grid_size; i++) delete grid[i];

  delete[] grid;
  delete[] particleX;
  delete[] particleY;
  delete[] randomX;
  delete[] randomY;
  delete[] map;

  return 0;
}

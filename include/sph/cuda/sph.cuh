#pragma once

#include <vector>

#include "Task.hpp"
#include "LinearOctree.hpp"

namespace sphexa
{
namespace sph
{
namespace cuda
{
template <typename T, class Dataset>
extern void computeFindNeighbors2(const LinearOctree<T> &o, std::vector<Task> &taskList, Dataset &d);

template <typename T, class Dataset>
extern void computeDensity(const std::vector<Task> &taskList, Dataset &d);

template <typename T, class Dataset>
extern void computeMomentumAndEnergy(const std::vector<Task> &taskList, Dataset &d);

template <typename T, class Dataset>
extern void computeIAD(const std::vector<Task> &taskList, Dataset &d);

template <typename T, class Dataset>
extern void computeMomentumAndEnergyIAD(const std::vector<Task> &taskList, Dataset &d);

} // namespace cuda
} // namespace sph
} // namespace sphexa
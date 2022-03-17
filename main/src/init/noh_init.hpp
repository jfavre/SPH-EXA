/*
 * MIT License
 *
 * Copyright (c) 2021 CSCS, ETH Zurich
 *               2021 University of Basel
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*! @file
 * @brief Noh implosion simulation data initialization
 *
 * @author Jose A. Escartin <ja.escartin@gmail.com>"
 */

#pragma once

#include <map>

#include "cstone/sfc/box.hpp"

#include "isim_init.hpp"
#include "grid.hpp"

namespace sphexa
{

template<class Dataset>
void initNohFields(Dataset& d, const std::map<std::string, double>& constants)
{
    using T = typename Dataset::RealType;

    int    ng0         = 100;
    double r1          = constants.at("r1");
    double totalVolume = std::pow(2 * r1, 3);
    double hInit       = std::cbrt(3.0 / (4 * M_PI) * ng0 * totalVolume / d.numParticlesGlobal) * 0.5;

    double mPart         = constants.at("mTotal") / d.numParticlesGlobal;
    double firstTimeStep = constants.at("firstTimeStep");

    std::fill(d.m.begin(), d.m.end(), mPart);
    std::fill(d.h.begin(), d.h.end(), hInit);
    std::fill(d.du_m1.begin(), d.du_m1.end(), 0.0);
    std::fill(d.mui.begin(), d.mui.end(), 10.0);
    std::fill(d.dt.begin(), d.dt.end(), firstTimeStep);
    std::fill(d.dt_m1.begin(), d.dt_m1.end(), firstTimeStep);
    std::fill(d.alpha.begin(), d.alpha.end(), d.alphamin);
    d.minDt = firstTimeStep;

#pragma omp parallel for schedule(static)
    for (size_t i = 0; i < d.x.size(); i++)
    {
        T radius = std::sqrt((d.x[i] * d.x[i]) + (d.y[i] * d.y[i]) + (d.z[i] * d.z[i]));
        radius   = std::max(radius, 1e-10);

        d.u[i] = constants.at("u0");

        d.vx[i] = constants.at("vr0") * (d.x[i] / radius);
        d.vy[i] = constants.at("vr0") * (d.y[i] / radius);
        d.vz[i] = constants.at("vr0") * (d.z[i] / radius);

        d.x_m1[i] = d.x[i] - d.vx[i] * firstTimeStep;
        d.y_m1[i] = d.y[i] - d.vy[i] * firstTimeStep;
        d.z_m1[i] = d.z[i] - d.vz[i] * firstTimeStep;
    }
}

template<class Dataset>
class NohGrid : public ISimInitializer<Dataset>
{
    std::map<std::string, double> constants_{{"r0", 0},
                                             {"r1", 0.5},
                                             {"mTotal", 1.},
                                             {"dim", 3},
                                             {"gamma", 5.0 / 3.0},
                                             {"rho0", 1.},
                                             {"u0", 1e-20},
                                             {"p0", 0.},
                                             {"vr0", -1.},
                                             {"cs0", 0.},
                                             {"firstTimeStep", 1e-4}};

public:
    cstone::Box<typename Dataset::RealType> init(int rank, int numRanks, size_t cubeSide, Dataset& d) const override
    {
        using T              = typename Dataset::RealType;
        d.numParticlesGlobal = cubeSide * cubeSide * cubeSide;

        auto [first, last] = partitionRange(d.numParticlesGlobal, rank, numRanks);
        resize(d, last - first);

        T r = constants_.at("r1");

        regularGrid(r, cubeSide, first, last, d.x, d.y, d.z);
        initNohFields(d, constants_);

        return cstone::Box<T>(-r, r, false);
    }

    const std::map<std::string, double>& constants() const override { return constants_; }
};

} // namespace sphexa
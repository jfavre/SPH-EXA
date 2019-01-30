#pragma once

#include <cmath>
#include <vector>
#include <fstream>
#include <sstream>

#include "sphexa.hpp"

template<typename T>
class Evrard
{
public:
    #ifdef USE_MPI
        Evrard(int n, const std::string &filename, MPI_Comm comm) : 
            n(n), count(n), comm(comm),data({&x, &y, &z, &x_m1, &y_m1, &z_m1, &vx, &vy, &vz, 
                &ro, &u, &p, &h, &m, &c, &cv, &temp, &mue, &mui, 
                &grad_P_x, &grad_P_y, &grad_P_z, &du, &du_m1, &dt, &dt_m1})
        {
            MPI_Comm_size(comm, &nrank);
            MPI_Comm_rank(comm, &rank);
            MPI_Get_processor_name(pname, &pnamelen);
            loadMPI(filename);
            init();
        }
    #else
         Evrard(int n, const std::string &filename) : 
            n(n), count(n), data({&x, &y, &z, &x_m1, &y_m1, &z_m1, &vx, &vy, &vz, 
                &ro, &u, &p, &h, &m, &c, &cv, &temp, &mue, &mui, 
                &grad_P_x, &grad_P_y, &grad_P_z, &du, &du_m1, &dt, &dt_m1})
        {
            resize(n);
            load(filename);
            init();
        }
    #endif

    inline void resize(unsigned int size)
    {
        for(unsigned int i=0; i<data.size(); i++)
            data[i]->resize(size);
        neighbors.resize(size);
    }

    #ifdef USE_MPI
    void loadMPI(const std::string &filename)
    {
        count = n / nrank;
        int offset = n % count;
        
        workload.resize(nrank);
        std::vector<int> displs(nrank);

        workload[0] = count+offset;
        displs[0] = 0;

        for(int i=1; i<nrank; i++)
        {
            workload[i] = count;
            displs[i] = displs[i-1] + workload[i-1];
        }

        if(rank == 0)
        {
            count += offset;

            resize(n);
            load(filename);

            MPI_Scatterv(&x[0], &workload[0], &displs[0], MPI_DOUBLE, MPI_IN_PLACE, count, MPI_DOUBLE, 0, comm);
            MPI_Scatterv(&y[0], &workload[0], &displs[0], MPI_DOUBLE, MPI_IN_PLACE, count, MPI_DOUBLE, 0, comm);
            MPI_Scatterv(&z[0], &workload[0], &displs[0], MPI_DOUBLE, MPI_IN_PLACE, count, MPI_DOUBLE, 0, comm);
            MPI_Scatterv(&vx[0], &workload[0], &displs[0], MPI_DOUBLE, MPI_IN_PLACE, count, MPI_DOUBLE, 0, comm);
            MPI_Scatterv(&vy[0], &workload[0], &displs[0], MPI_DOUBLE, MPI_IN_PLACE, count, MPI_DOUBLE, 0, comm);
            MPI_Scatterv(&vz[0], &workload[0], &displs[0], MPI_DOUBLE, MPI_IN_PLACE, count, MPI_DOUBLE, 0, comm);
            MPI_Scatterv(&ro[0], &workload[0], &displs[0], MPI_DOUBLE, MPI_IN_PLACE, count, MPI_DOUBLE, 0, comm);
            MPI_Scatterv(&u[0], &workload[0], &displs[0], MPI_DOUBLE, MPI_IN_PLACE, count, MPI_DOUBLE, 0, comm);
            MPI_Scatterv(&p[0], &workload[0], &displs[0], MPI_DOUBLE, MPI_IN_PLACE, count, MPI_DOUBLE, 0, comm);
            MPI_Scatterv(&h[0], &workload[0], &displs[0], MPI_DOUBLE, MPI_IN_PLACE, count, MPI_DOUBLE, 0, comm);
            MPI_Scatterv(&m[0], &workload[0], &displs[0], MPI_DOUBLE, MPI_IN_PLACE, count, MPI_DOUBLE, 0, comm);

            resize(count);
        }
        else
        {
            resize(count);

            MPI_Scatterv(NULL, &workload[0], &displs[0], MPI_DOUBLE, &x[0], count, MPI_DOUBLE, 0, comm);
            MPI_Scatterv(NULL, &workload[0], &displs[0], MPI_DOUBLE, &y[0], count, MPI_DOUBLE, 0, comm);
            MPI_Scatterv(NULL, &workload[0], &displs[0], MPI_DOUBLE, &z[0], count, MPI_DOUBLE, 0, comm);
            MPI_Scatterv(NULL, &workload[0], &displs[0], MPI_DOUBLE, &vx[0], count, MPI_DOUBLE, 0, comm);
            MPI_Scatterv(NULL, &workload[0], &displs[0], MPI_DOUBLE, &vy[0], count, MPI_DOUBLE, 0, comm);
            MPI_Scatterv(NULL, &workload[0], &displs[0], MPI_DOUBLE, &vz[0], count, MPI_DOUBLE, 0, comm);
            MPI_Scatterv(NULL, &workload[0], &displs[0], MPI_DOUBLE, &ro[0], count, MPI_DOUBLE, 0, comm);
            MPI_Scatterv(NULL, &workload[0], &displs[0], MPI_DOUBLE, &u[0], count, MPI_DOUBLE, 0, comm);
            MPI_Scatterv(NULL, &workload[0], &displs[0], MPI_DOUBLE, &p[0], count, MPI_DOUBLE, 0, comm);
            MPI_Scatterv(NULL, &workload[0], &displs[0], MPI_DOUBLE, &h[0], count, MPI_DOUBLE, 0, comm);
            MPI_Scatterv(NULL, &workload[0], &displs[0], MPI_DOUBLE, &m[0], count, MPI_DOUBLE, 0, comm);
        }
    }
    #endif

    void load(const std::string &filename)
    {
        // input file stream
        std::ifstream inputfile(filename, std::ios::binary);

        if(inputfile.is_open())
        {
            // read the contents of the file into the vectors
            inputfile.read(reinterpret_cast<char*>(x.data()), sizeof(T)*x.size());
            inputfile.read(reinterpret_cast<char*>(y.data()), sizeof(T)*y.size());
            inputfile.read(reinterpret_cast<char*>(z.data()), sizeof(T)*z.size());
            inputfile.read(reinterpret_cast<char*>(vx.data()), sizeof(T)*vx.size());
            inputfile.read(reinterpret_cast<char*>(vy.data()), sizeof(T)*vy.size());
            inputfile.read(reinterpret_cast<char*>(vz.data()), sizeof(T)*vz.size());
            inputfile.read(reinterpret_cast<char*>(ro.data()), sizeof(T)*ro.size());
            inputfile.read(reinterpret_cast<char*>(u.data()), sizeof(T)*u.size());
            inputfile.read(reinterpret_cast<char*>(p.data()), sizeof(T)*p.size());
            inputfile.read(reinterpret_cast<char*>(h.data()), sizeof(T)*h.size());
            inputfile.read(reinterpret_cast<char*>(m.data()), sizeof(T)*m.size());
        }
        else
            std::cout << "ERROR: " << "in opening file " << filename << std::endl;
    }

    void init()
    {
        std::fill(temp.begin(), temp.end(), 1.0);
        std::fill(mue.begin(), mue.end(), 2.0);
        std::fill(mui.begin(), mui.end(), 10.0);
        std::fill(vx.begin(), vx.end(), 0.0);
        std::fill(vy.begin(), vy.end(), 0.0);
        std::fill(vz.begin(), vz.end(), 0.0);

        std::fill(grad_P_x.begin(), grad_P_x.end(), 0.0);
        std::fill(grad_P_y.begin(), grad_P_y.end(), 0.0);
        std::fill(grad_P_z.begin(), grad_P_z.end(), 0.0);

        std::fill(du.begin(), du.end(), 0.0);
        std::fill(du_m1.begin(), du_m1.end(), 0.0);

        std::fill(dt.begin(), dt.end(), 0.0001);
        std::fill(dt_m1.begin(), dt_m1.end(), 0.0001);

        for(unsigned int i=0; i<count; i++)
        {
            x_m1[i] = x[i] - vx[i] * dt[0];
            y_m1[i] = y[i] - vy[i] * dt[0];
            z_m1[i] = z[i] - vz[i] * dt[0];
        }

        etot = ecin = eint = 0.0;
        ttot = 0.0;

        for(auto i : neighbors)
            i.reserve(ngmax);
    }

    void writeFile(const std::vector<int> &clist, std::ofstream &outputFile)
    {
        #ifdef USE_MPI
            std::vector<int> workload(nrank);

            int load = (int)clist.size();
            MPI_Allgather(&load, 1, MPI_INT, &workload[0], 1, MPI_INT, MPI_COMM_WORLD);

            std::vector<int> displs(nrank);

            displs[0] = 0;
            for(int i=1; i<nrank; i++)
                displs[i] = displs[i-1]+workload[i-1];

            if(rank == 0)
            {
                resize(n);

                MPI_Gatherv(MPI_IN_PLACE, (int)clist.size(), MPI_DOUBLE, &x[0], &workload[0], &displs[0], MPI_DOUBLE, 0, MPI_COMM_WORLD);
                MPI_Gatherv(MPI_IN_PLACE, (int)clist.size(), MPI_DOUBLE, &y[0], &workload[0], &displs[0], MPI_DOUBLE, 0, MPI_COMM_WORLD);
                MPI_Gatherv(MPI_IN_PLACE, (int)clist.size(), MPI_DOUBLE, &z[0], &workload[0], &displs[0], MPI_DOUBLE, 0, MPI_COMM_WORLD);
                MPI_Gatherv(MPI_IN_PLACE, (int)clist.size(), MPI_DOUBLE, &vx[0], &workload[0], &displs[0], MPI_DOUBLE, 0, MPI_COMM_WORLD);
                MPI_Gatherv(MPI_IN_PLACE, (int)clist.size(), MPI_DOUBLE, &vy[0], &workload[0], &displs[0], MPI_DOUBLE, 0, MPI_COMM_WORLD);
                MPI_Gatherv(MPI_IN_PLACE, (int)clist.size(), MPI_DOUBLE, &vz[0], &workload[0], &displs[0], MPI_DOUBLE, 0, MPI_COMM_WORLD);
                MPI_Gatherv(MPI_IN_PLACE, (int)clist.size(), MPI_DOUBLE, &h[0], &workload[0], &displs[0], MPI_DOUBLE, 0, MPI_COMM_WORLD);
                MPI_Gatherv(MPI_IN_PLACE, (int)clist.size(), MPI_DOUBLE, &ro[0], &workload[0], &displs[0], MPI_DOUBLE, 0, MPI_COMM_WORLD);
                MPI_Gatherv(MPI_IN_PLACE, (int)clist.size(), MPI_DOUBLE, &u[0], &workload[0], &displs[0], MPI_DOUBLE, 0, MPI_COMM_WORLD);
                MPI_Gatherv(MPI_IN_PLACE, (int)clist.size(), MPI_DOUBLE, &p[0], &workload[0], &displs[0], MPI_DOUBLE, 0, MPI_COMM_WORLD);
                MPI_Gatherv(MPI_IN_PLACE, (int)clist.size(), MPI_DOUBLE, &c[0], &workload[0], &displs[0], MPI_DOUBLE, 0, MPI_COMM_WORLD);
                MPI_Gatherv(MPI_IN_PLACE, (int)clist.size(), MPI_DOUBLE, &grad_P_x[0], &workload[0], &displs[0], MPI_DOUBLE, 0, MPI_COMM_WORLD);
                MPI_Gatherv(MPI_IN_PLACE, (int)clist.size(), MPI_DOUBLE, &grad_P_y[0], &workload[0], &displs[0], MPI_DOUBLE, 0, MPI_COMM_WORLD);
                MPI_Gatherv(MPI_IN_PLACE, (int)clist.size(), MPI_DOUBLE, &grad_P_z[0], &workload[0], &displs[0], MPI_DOUBLE, 0, MPI_COMM_WORLD);
            }
            else
            {
                MPI_Gatherv(&x[0], (int)clist.size(), MPI_DOUBLE, NULL, &workload[0], &displs[0], MPI_DOUBLE, 0, MPI_COMM_WORLD);
                MPI_Gatherv(&y[0], (int)clist.size(), MPI_DOUBLE, NULL, &workload[0], &displs[0], MPI_DOUBLE, 0, MPI_COMM_WORLD);
                MPI_Gatherv(&z[0], (int)clist.size(), MPI_DOUBLE, NULL, &workload[0], &displs[0], MPI_DOUBLE, 0, MPI_COMM_WORLD);
                MPI_Gatherv(&vx[0], (int)clist.size(), MPI_DOUBLE, NULL, &workload[0], &displs[0], MPI_DOUBLE, 0, MPI_COMM_WORLD);
                MPI_Gatherv(&vy[0], (int)clist.size(), MPI_DOUBLE, NULL, &workload[0], &displs[0], MPI_DOUBLE, 0, MPI_COMM_WORLD);
                MPI_Gatherv(&vz[0], (int)clist.size(), MPI_DOUBLE, NULL, &workload[0], &displs[0], MPI_DOUBLE, 0, MPI_COMM_WORLD);
                MPI_Gatherv(&h[0], (int)clist.size(), MPI_DOUBLE, NULL, &workload[0], &displs[0], MPI_DOUBLE, 0, MPI_COMM_WORLD);
                MPI_Gatherv(&ro[0], (int)clist.size(), MPI_DOUBLE, NULL, &workload[0], &displs[0], MPI_DOUBLE, 0, MPI_COMM_WORLD);
                MPI_Gatherv(&u[0], (int)clist.size(), MPI_DOUBLE, NULL, &workload[0], &displs[0], MPI_DOUBLE, 0, MPI_COMM_WORLD);
                MPI_Gatherv(&p[0], (int)clist.size(), MPI_DOUBLE, NULL, &workload[0], &displs[0], MPI_DOUBLE, 0, MPI_COMM_WORLD);
                MPI_Gatherv(&c[0], (int)clist.size(), MPI_DOUBLE, NULL, &workload[0], &displs[0], MPI_DOUBLE, 0, MPI_COMM_WORLD);
                MPI_Gatherv(&grad_P_x[0], (int)clist.size(), MPI_DOUBLE, NULL, &workload[0], &displs[0], MPI_DOUBLE, 0, MPI_COMM_WORLD);
                MPI_Gatherv(&grad_P_y[0], (int)clist.size(), MPI_DOUBLE, NULL, &workload[0], &displs[0], MPI_DOUBLE, 0, MPI_COMM_WORLD);
                MPI_Gatherv(&grad_P_z[0], (int)clist.size(), MPI_DOUBLE, NULL, &workload[0], &displs[0], MPI_DOUBLE, 0, MPI_COMM_WORLD);
            }
        #endif

        if(rank == 0)
        {
            for(unsigned int i=0; i<n; i++)
            {
                outputFile << x[i] << ' ' << y[i] << ' ' << z[i] << ' ';
                outputFile << vx[i] << ' ' << vy[i] << ' ' << vz[i] << ' ';
                outputFile << h[i] << ' ' << ro[i] << ' ' << u[i] << ' ' << p[i] << ' ' << c[i] << ' ';
                outputFile << grad_P_x[i] << ' ' << grad_P_y[i] << ' ' << grad_P_z[i] << ' ';
                T rad = sqrt(x[i] * x[i] + y[i] * y[i] + z[i] * z[i]);
                T vrad = (vx[i] *  x[i] + vy[i] * y[i] + vz[i] * z[i]) / rad;
                outputFile << rad << ' ' << vrad << std::endl;  
            }  
        }

        #ifdef USE_MPI
            if(rank == 0) resize(count);
        #endif
    }

    unsigned int n, count; // Number of particles
    std::vector<T> x, y, z, x_m1, y_m1, z_m1; // Positions
    std::vector<T> vx, vy, vz; // Velocities
    std::vector<T> ro; // Density
    std::vector<T> u; // Internal Energy
    std::vector<T> p; // Pressure
    std::vector<T> h; // Smoothing Length
    std::vector<T> m; // Mass
    std::vector<T> c; // Speed of sound
    std::vector<T> cv; // Specific heat
    std::vector<T> temp; // Temperature
    std::vector<T> mue; // Mean molecular weigh of electrons
    std::vector<T> mui; // Mean molecular weight of ions
    std::vector<T> grad_P_x, grad_P_y, grad_P_z; //gradient of the pressure
    std::vector<T> du, du_m1; //variation of the energy
    std::vector<T> dt, dt_m1;

    T etot, ecin, eint;
    T ttot;

    sphexa::BBox<T> bbox;
    std::vector<std::vector<int>> neighbors; // List of neighbor indices per particle.

    #ifdef USE_MPI
        MPI_Comm comm;
        int nrank = 0, pnamelen = 0;
        char pname[MPI_MAX_PROCESSOR_NAME];
        std::vector<int> workload;
    #endif
    
    int rank = 0;

    std::vector<std::vector<T>*> data;

    const T sincIndex = 6.0;
    const T K = sphexa::compute_3d_k(sincIndex);
    const T Kcour = 0.2;
    const T maxDtIncrease = 1.1;
    const int stabilizationTimesteps = -1;
    const unsigned int ngmin = 50, ng0 = 100, ngmax = 150; // Minimum, target and maximum number of neighbors per particle
};
#include "dense_op.hh"
#include "RandBLAS.hh"
#include <iostream>
#include <stdio.h>
#include <omp.h>

#include <math.h>
#include <typeinfo>

#include <Random123/philox.h>
#include <Random123/boxmuller.hpp>
#include <Random123/uniform.hpp>


namespace RandBLAS::dense_op {

/*
Note from Random123: Simply incrementing the counter (or key) is effectively indistinguishable
from a sequence of samples of a uniformly distributed random variable.

Notes:
1. Currently, Not sure how to make generalized implementations to support both 2 and 4 random number generations.
2. Note on eventually needing to add 8 and 16-bit generations.
*/



// Actual work - uniform dirtibution
template <typename T, typename T_gen>
static void gen_unif(int64_t n_rows, int64_t n_cols, T* mat, uint32_t seed)
{
        typename T_gen::key_type key = {{seed}};
        // Definde the generator
        T_gen gen;

        int64_t dim = n_rows * n_cols;

        // Need every thread to have its own version of key for the outer loop to be parallelizable
        // Need to figure out when fork/join overhead becomes less than time saved by parallelization

        // Effectively, below structure is similar to unrolling by a factor of 4
        uint32_t i = 0;
        // Compensation code - we would effectively not use at least 1 and up to 3 generated random numbers 
        int comp = dim % 4;

        if (comp){
                // Below array represents counter unpating
                typename T_gen::ctr_type r = gen({{1,0,0,0}}, key);

                // Only 3 cases here, so using nested ifs
                mat[i] = r123::uneg11<T>(r.v[0]);
                ++i;
                if (i < comp)
                {
                        mat[i] = r123::uneg11<T>(r.v[1]);
                        ++i;
                        if (i < comp)
                        {
                                mat[i] = r123::uneg11<T>(r.v[2]);
                                ++i;
                        }
                }
        }
        for (; i < dim; i += 4)
        {
                // Adding critical section around the increment should make outer loop parallelizable?
                // Below array represents counter updating
                typename T_gen::ctr_type r = gen({{i,0,0,0}}, key);

                mat[i] = r123::uneg11<T>(r.v[0]);
                mat[i + 1] = r123::uneg11<T>(r.v[1]);
                mat[i + 2] = r123::uneg11<T>(r.v[2]);
                mat[i + 3] = r123::uneg11<T>(r.v[3]);
        }
}

template <typename T>
void gen_rmat_unif(int64_t n_rows, int64_t n_cols, T* mat, uint32_t seed)
{
        if (typeid(T) == typeid(float))
        {
                // 4 32-bit generated values
                typedef r123::Philox4x32 CBRNG;
                // Casting is done only so that the compiler does not throw an error
                gen_unif<float, CBRNG>(n_rows, n_cols, (float*) mat, seed);
        }
        else if (typeid(T) == typeid(double))
        {
                // 4 64-bit generated values
                typedef r123::Philox4x64 CBRNG;
                // Casting is done only so that the compiler does not throw an error
                gen_unif<double, CBRNG>(n_rows, n_cols, (double*) mat, seed);
        }
        else
        {
                printf("\nType error. Only float and double are currently supported.\n");
        }
}

// Actual work - normal distribution
template <typename T, typename T_gen, typename T_fun>
static void gen_norm(int64_t n_rows, int64_t n_cols, T* mat, uint32_t seed)
{
        typename T_gen::key_type key = {{seed}};
        // Definde the generator
        T_gen gen;

        uint64_t dim = n_rows * n_cols;

        // Effectively, below structure is similar to unrolling by a factor of 4
        uint32_t i = 0;
        // Compensation code - we would effectively not use at least 1 and up to 3 generated random numbers 
        int comp = dim % 4;
        if (comp){

                // Below array represents counter updating
                typename T_gen::ctr_type r = gen({{1, 0, 0, 0}}, key);

                // Take 2 32 or 64-bit unsigned random vals, return 2 random floats/doubles
                // Since generated vals are indistinguishable form uniform, feed them into box-muller right away
                // Uses uneg11 & u01 under the hood
                T_fun pair_1 = r123::boxmuller(r.v[0], r.v[1]);
                T_fun pair_2 = r123::boxmuller(r.v[2], r.v[3]);

                // Only 3 cases here, so using nested ifs
                mat[i] = pair_1.x;
                ++i;
                if (i < comp)
                {
                        mat[i] = pair_1.y;
                        ++i;
                        if (i < comp)
                        {
                                mat[i] = pair_2.x;
                                ++i;
                        }
                }
        }
        // Unrolling
        for (; i < dim; i += 4)
        {
                // Below array represents counter updating
                typename T_gen::ctr_type r = gen({{i, 0, 0, 0}}, key);
                // Paralleleize

                // Take 2 32 or 64-bit unsigned random vals, return 2 random floats/doubles
                // Since generated vals are indistinguishable form uniform, feed them into box-muller right away
                // Uses uneg11 & u01 under the hood
                T_fun pair_1 = r123::boxmuller(r.v[0], r.v[1]);
                T_fun pair_2 = r123::boxmuller(r.v[2], r.v[3]);

                mat[i] = pair_1.x;
                mat[i + 1] = pair_1.y;
                mat[i + 2] = pair_2.x;
                mat[i + 3] = pair_2.y;
        }
}

template <typename T>
void gen_rmat_norm(int64_t n_rows, int64_t n_cols, T* mat, uint32_t seed)
{
        if (typeid(T) == typeid(float))
        {
                // 4 32-bit generated values
                typedef r123::Philox4x32 CBRNG;
                // Casting is done only so that the compiler does not throw an error
                gen_norm<float, CBRNG, r123::float2>(n_rows, n_cols, (float *) mat, seed);

        }
        else if (typeid(T) == typeid(double))
        {
                // 4 32-bit generated values
                typedef r123::Philox4x64 CBRNG;
                // Casting is done only so that the compiler does not throw an error
                gen_norm<double, CBRNG, r123::double2>(n_rows, n_cols, (double *) mat, seed);
        }        
        else
        {
                printf("\nType error. Only float and double are currently supported.\n");
        }
}

template<typename T>
void apply_haar(int64_t n_rows, int64_t n_cols, T *mat, int64_t ldc, uint32_t seed) {
    int i,j;
    T signu0;              // Stores the sign of u[0] to avoid cancellation
    T u[n_cols];           // vector u to store random normal values. Used in the construction of Householder.
    T norm;                // Stores the norm of u to normalize u
    for (i=n_cols-1; i>0; i--) {
        RandBLAS::dense_op::gen_rmat_norm<T>(1, i+1, u, seed);  
        signu0 = RandBLAS::osbm::sgn<T>(u[0]);                  
        u[0] = u[0] + signu0 * blas::nrm2(i+1,u,1);
        norm = blas::nrm2(i+1, u, 1);
        blas::scal(i+1, 1/norm, u, 1);
        RandBLAS::util::larf<T>('R', n_rows, i+1, &u[0], 1, 2, &mat[n_cols-i-1], ldc); 
        blas::scal(n_rows, signu0, &mat[n_cols-i+1], ldc);
    }
}
//
template<typename T>
void gen_rmat_haar(int64_t n_rows, int64_t n_cols, T *mat, int32_t seed) {
    int i;
    for (i=0; i<n_rows*n_cols; i++) {
        mat[i] = 0; 
    }
    for (i=0;i<n_cols; i++) {
        mat[i*n_cols + i] = 1;
    }
    apply_haar<T>(n_rows, n_cols, mat, n_cols, seed);
}

// Explicit instantiation of template functions - workaround to avoid header implementations
template void apply_haar<float>(int64_t n_rows, int64_t n_cols, float *mat, int64_t ldc, uint32_t seed);
template void apply_haar<double>(int64_t n_rows, int64_t n_cols, double *mat, int64_t ldc, uint32_t seed);

template void gen_rmat_haar<float>(int64_t n_rows, int64_t n_cols, float *mat, int32_t seed);
template void gen_rmat_haar<double>(int64_t n_rows, int64_t n_cols, double *mat, int32_t seed);

template void gen_rmat_unif<float>(int64_t n_rows, int64_t n_cols, float* mat, uint32_t seed);
template void gen_rmat_unif<double>(int64_t n_rows, int64_t n_cols, double* mat, uint32_t seed);

template void gen_rmat_norm<float>(int64_t n_rows, int64_t n_cols, float* mat, uint32_t seed);
template void gen_rmat_norm<double>(int64_t n_rows, int64_t n_cols, double* mat, uint32_t seed);

} // end namespace RandBLAS::dense_op

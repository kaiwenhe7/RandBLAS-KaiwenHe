#include <rblasutil.hh>
#include <determiter.hh>
#include <sparseops.hh>
#include <blas.hh>
#include "gtest/gtest.h"


int main(int argc, char *argv[]) {
    //int64_t n = 20;
    //int64_t m = 2*n;
    //run_pcgls_ex(n, m);

    double a[25];
    int64_t k = 5;
	// genmat(k, k, a, 0); 

    struct SJLT sjl;
    sjl.ori = ColumnWise;
    sjl.n_rows = 6000;
    sjl.n_cols = 100000;
    sjl.vec_nnz = 8;
    uint64_t *rows = new u_int64_t[sjl.vec_nnz * sjl.n_cols];
    sjl.rows = rows;
    uint64_t *cols = new u_int64_t[sjl.vec_nnz * sjl.n_cols];
    sjl.cols = cols;
    double *vals = new double[sjl.vec_nnz * sjl.n_cols];
    sjl.vals = vals;

    fill_colwise_sjlt(sjl, 0, 0);
    //print_sjlt(sjl);

    ::testing::InitGoogleTest(&argc, argv);
    int res = RUN_ALL_TESTS();
    return res;
}

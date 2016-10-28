#include <gtest/gtest.h>
#include "example.cpp"

int ex_argc;
char** ex_argv;
bool ex_runs;
bool run_example(int, char**);

#if (NDIM == 2)
#define TEST_CASE_NAME IBFE_ex7_2d
#endif
#if (NDIM == 3)
#define TEST_CASE_NAME IBFE_ex7_3d
#endif

TEST(TEST_CASE_NAME, example_runs) {
    ex_runs = run_example(ex_argc, ex_argv);
    EXPECT_EQ(ex_runs, true);
}

int main( int argc, char** argv ) {
    testing::InitGoogleTest( &argc, argv ); 
    ex_argc = argc;
    ex_argv = argv;
    return RUN_ALL_TESTS( );
}

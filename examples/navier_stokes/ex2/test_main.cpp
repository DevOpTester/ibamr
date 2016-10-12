#include <gtest/gtest.h>
#include "example.cpp"

int ex_argc;
char** ex_argv;
bool ex_runs;
bool run_example(int, char**);

TEST(navier_stokes_ex2, 2d) {
    ex_runs = run_example(ex_argc, ex_argv);
    EXPECT_EQ(ex_runs, true);
}

int main( int argc, char** argv ) {
    testing::InitGoogleTest( &argc, argv ); 
    ex_argc = argc;
    ex_argv = argv;
    return RUN_ALL_TESTS( );
}
#include <gtest/gtest.h>
#include "ex0.cpp"

int example_argc;
char** example_argv;
bool ExampleRuns;
bool runExample(int, char**);

TEST(IBFE_ex1, 2d) {
    ExampleRuns = runExample(example_argc, example_argv);
    EXPECT_EQ(ExampleRuns, true);
}

/*TEST(IBFE_ex1, 3d) {
    ExampleRuns = runExample(example_argc, example_argv);
    EXPECT_EQ(ExampleRuns, true);
}*/

int main( int argc, char** argv ) {
    testing::InitGoogleTest( &argc, argv ); 
    example_argc = argc;
    example_argv = argv;
    return RUN_ALL_TESTS( );
}

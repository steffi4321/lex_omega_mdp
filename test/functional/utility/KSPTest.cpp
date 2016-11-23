#include "gtest/gtest.h"
#include "storm-config.h"

#include "src/builder/ExplicitModelBuilder.h"
#include "src/models/sparse/Dtmc.h"
#include "src/parser/PrismParser.h"
#include "src/storage/SymbolicModelDescription.h"
#include "src/utility/graph.h"
#include "src/utility/shortestPaths.cpp"

// NOTE: The KSPs / distances of these tests were generated by the
//       KSP-Generator itself and checked for gross implausibility, but no
//       more than that.
//       An independent verification of the values would be really nice ...

// FIXME: (almost) all of these fail; the question is: is there actually anything wrong or does the new parser yield a different order of states?

std::shared_ptr<storm::models::sparse::Model<double>> buildExampleModel() {
	std::string prismModelPath = STORM_CPP_TESTS_BASE_PATH "/functional/builder/brp-16-2.pm";
    storm::storage::SymbolicModelDescription modelDescription = storm::parser::PrismParser::parse(prismModelPath);
    storm::prism::Program program = modelDescription.preprocess().asPrismProgram();
    return storm::builder::ExplicitModelBuilder<double>(program).build();
}

TEST(KSPTest, dijkstra) {
	auto model = buildExampleModel();

    storm::utility::ksp::state_t testState = 300;
    storm::utility::ksp::ShortestPathsGenerator<double> spg(*model, testState);

    double dist = spg.getDistance(1);
    EXPECT_DOUBLE_EQ(0.015859334652581887, dist);
}

TEST(KSPTest, singleTarget) {
	auto model = buildExampleModel();

    storm::utility::ksp::state_t testState = 300;
    storm::utility::ksp::ShortestPathsGenerator<double> spg(*model, testState);

    double dist = spg.getDistance(100);
    EXPECT_DOUBLE_EQ(1.5231305000339649e-06, dist);
}

TEST(KSPTest, reentry) {
	auto model = buildExampleModel();

    storm::utility::ksp::state_t testState = 300;
    storm::utility::ksp::ShortestPathsGenerator<double> spg(*model, testState);

    double dist = spg.getDistance(100);
    EXPECT_DOUBLE_EQ(1.5231305000339649e-06, dist);

    // get another distance to ensure re-entry is no problem
    double dist2 = spg.getDistance(500);
    EXPECT_DOUBLE_EQ(3.0462610000679282e-08, dist2);
}

TEST(KSPTest, groupTarget) {
	auto model = buildExampleModel();

    auto groupTarget = std::vector<storm::utility::ksp::state_t>{50, 90};
    auto spg = storm::utility::ksp::ShortestPathsGenerator<double>(*model, groupTarget);

    // this path should lead to 90
    double dist1 = spg.getDistance(8);
    EXPECT_DOUBLE_EQ(7.3796982335999988e-06, dist1);

    // this one to 50
    double dist2 = spg.getDistance(9);
    EXPECT_DOUBLE_EQ(3.92e-06, dist2);

    // this one to 90 again
    double dist3 = spg.getDistance(12);
    EXPECT_DOUBLE_EQ(3.6160521344640002e-06, dist3);
}

TEST(KSPTest, kTooLargeException) {
	auto model = buildExampleModel();

    storm::utility::ksp::state_t testState = 1;
    storm::utility::ksp::ShortestPathsGenerator<double> spg(*model, testState);

    ASSERT_THROW(spg.getDistance(2), std::invalid_argument);
}

TEST(KSPTest, kspStateSet) {
	auto model = buildExampleModel();

    storm::utility::ksp::state_t testState = 300;
    storm::utility::ksp::ShortestPathsGenerator<double> spg(*model, testState);

    storm::storage::BitVector referenceBV(model->getNumberOfStates(), false);
    for (auto s : std::vector<storm::utility::ksp::state_t>{300, 293, 285, 279, 271, 265, 259, 252, 244, 237, 229, 223, 217, 210, 202, 195, 187, 181, 175, 168, 160, 153, 145, 139, 133, 126, 118, 111, 103, 97, 91, 84, 76, 69, 61, 55, 49, 43, 35, 41, 32, 25, 19, 14, 10, 7, 4, 2, 1, 0}) {
        referenceBV.set(s, true);
    }

    auto bv = spg.getStates(7);

    EXPECT_EQ(bv, referenceBV);
}

TEST(KSPTest, kspPathAsList) {
	auto model = buildExampleModel();

    storm::utility::ksp::state_t testState = 300;
    storm::utility::ksp::ShortestPathsGenerator<double> spg(*model, testState);

    // TODO: use path that actually has a loop or something to make this more interesting
    auto reference = storm::utility::ksp::OrderedStateList{300, 293, 285, 279, 271, 265, 259, 252, 244, 237, 229, 223, 217, 210, 202, 195, 187, 181, 175, 168, 160, 153, 145, 139, 133, 126, 118, 111, 103, 97, 91, 84, 76, 69, 61, 55, 49, 43, 35, 41, 32, 25, 19, 14, 10, 7, 4, 2, 1, 0};
    auto list = spg.getPathAsList(7);

    EXPECT_EQ(list, reference);
}

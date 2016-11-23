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

// NOTE: these are hardcoded (obviously), but the model's state indices might change
// (e.g., when the parser or model builder are changed)
// [state 296 seems to be the new index of the old state 300 (checked a few ksps' probs)]
const storm::utility::ksp::state_t testState = 296;
const storm::utility::ksp::state_t stateWithOnlyOnePath = 1;

TEST(KSPTest, dijkstra) {
    auto model = buildExampleModel();
    storm::utility::ksp::ShortestPathsGenerator<double> spg(*model, testState);

    double dist = spg.getDistance(1);
    EXPECT_DOUBLE_EQ(0.015859334652581887, dist);
}

TEST(KSPTest, singleTarget) {
    auto model = buildExampleModel();
    storm::utility::ksp::ShortestPathsGenerator<double> spg(*model, testState);

    double dist = spg.getDistance(100);
    EXPECT_DOUBLE_EQ(1.5231305000339649e-06, dist);
}

TEST(KSPTest, reentry) {
    auto model = buildExampleModel();
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

    // FIXME comments are outdated (but does it even matter?)
    // this path should lead to 90
    double dist1 = spg.getDistance(8);
    EXPECT_DOUBLE_EQ(0.00018449245583999996, dist1);

    // this one to 50
    double dist2 = spg.getDistance(9);
    EXPECT_DOUBLE_EQ(0.00018449245583999996, dist2);

    // this one to 90 again
    double dist3 = spg.getDistance(12);
    EXPECT_DOUBLE_EQ(7.5303043199999984e-06, dist3);
}

TEST(KSPTest, kTooLargeException) {
    auto model = buildExampleModel();
    storm::utility::ksp::ShortestPathsGenerator<double> spg(*model, stateWithOnlyOnePath);

    ASSERT_THROW(spg.getDistance(2), std::invalid_argument);
}

TEST(KSPTest, kspStateSet) {
    auto model = buildExampleModel();
    storm::utility::ksp::ShortestPathsGenerator<double> spg(*model, testState);

    storm::storage::BitVector referenceBV(model->getNumberOfStates(), false);
    for (auto s : std::vector<storm::utility::ksp::state_t>{0, 1, 3, 5, 7, 10, 14, 19, 25, 29, 33, 36, 40, 44, 50, 56, 62, 70, 77, 85, 92, 98, 104, 112, 119, 127, 134, 140, 146, 154, 161, 169, 176, 182, 188, 196, 203, 211, 218, 224, 230, 238, 245, 253, 260, 266, 272, 281, 288, 296}) {
        referenceBV.set(s, true);
    }

    auto bv = spg.getStates(7);

    EXPECT_EQ(referenceBV, bv);
}

TEST(KSPTest, kspPathAsList) {
    auto model = buildExampleModel();
    storm::utility::ksp::ShortestPathsGenerator<double> spg(*model, testState);

    // TODO: use path that actually has a loop or something to make this more interesting
    auto reference = storm::utility::ksp::OrderedStateList{296, 288, 281, 272, 266, 260, 253, 245, 238, 230, 224, 218, 211, 203, 196, 188, 182, 176, 169, 161, 154, 146, 140, 134, 127, 119, 112, 104, 98, 92, 85, 77, 70, 62, 56, 50, 44, 36, 29, 40, 33, 25, 19, 14, 10, 7, 5, 3, 1, 0};
    auto list = spg.getPathAsList(7);

    EXPECT_EQ(reference, list);
}

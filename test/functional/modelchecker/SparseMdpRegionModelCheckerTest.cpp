#include "gtest/gtest.h"
#include "storm-config.h"

#ifdef STORM_HAVE_CARL

#include "src/adapters/CarlAdapter.h"

#include "src/settings/SettingsManager.h"
#include "src/settings/modules/GeneralSettings.h"
#include "src/settings/modules/RegionSettings.h"

#include "src/models/sparse/Dtmc.h"
#include "src/parser/PrismParser.h"
#include "src/parser/FormulaParser.h"
#include "src/logic/Formulas.h"
#include "src/models/ModelBase.h"
#include "src/models/sparse/Model.h"
#include "src/models/sparse/Dtmc.h"
#include "builder/ExplicitPrismModelBuilder.h"
#include "modelchecker/region/SparseMdpRegionModelChecker.h"
#include "modelchecker/region/ParameterRegion.h"

TEST(SparseMdpRegionModelCheckerTest, two_dice_Prob) {
    
    std::string const& programFile = STORM_CPP_BASE_PATH "/examples/pmdp/two_dice/two_dice.nm";
    std::string const& formulaAsString = "P<=0.17 [F \"doubles\" ]";
    std::string const& constantsAsString = ""; //e.g. pL=0.9,TOACK=0.5
    
    //Build model, formula, region model checker
    boost::optional<storm::prism::Program> program=storm::parser::PrismParser::parse(programFile).simplify().simplify();
    program->checkValidity();
    storm::parser::FormulaParser formulaParser(program.get().getManager().getSharedPointer());
    std::vector<std::shared_ptr<storm::logic::Formula>> formulas = formulaParser.parseFromString(formulaAsString);
    typename storm::builder::ExplicitPrismModelBuilder<storm::RationalFunction>::Options options=storm::builder::ExplicitPrismModelBuilder<storm::RationalFunction>::Options(*formulas[0]);
    options.addConstantDefinitionsFromString(program.get(), constantsAsString); 
    options.preserveFormula(*formulas[0]);
    std::shared_ptr<storm::models::sparse::Model<storm::RationalFunction>> model = storm::builder::ExplicitPrismModelBuilder<storm::RationalFunction>().translateProgram(program.get(), options)->as<storm::models::sparse::Model<storm::RationalFunction>>();
    ASSERT_EQ(storm::models::ModelType::Mdp, model->getType());
    std::shared_ptr<storm::models::sparse::Mdp<storm::RationalFunction>> mdp = model->template as<storm::models::sparse::Mdp<storm::RationalFunction>>();
    storm::modelchecker::region::SparseMdpRegionModelChecker<storm::models::sparse::Mdp<storm::RationalFunction>, double> modelchecker(*mdp);
    ASSERT_TRUE(modelchecker.canHandle(*formulas[0]));
    modelchecker.specifyFormula(formulas[0]);
    
    //start testing
    auto allSatRegion=storm::modelchecker::region::ParameterRegion<storm::RationalFunction>::parseRegion("0.495<=p1<=0.5,0.5<=p2<=0.505");
    auto exBothRegion=storm::modelchecker::region::ParameterRegion<storm::RationalFunction>::parseRegion("0.45<=p1<=0.55,0.45<=p2<=0.55");
    auto allVioRegion=storm::modelchecker::region::ParameterRegion<storm::RationalFunction>::parseRegion("0.6<=p1<=0.7,0.6<=p2<=0.6");

    EXPECT_NEAR(0.1666665285, modelchecker.getReachabilityValue(allSatRegion.getLowerBounds()), storm::settings::generalSettings().getPrecision());
    EXPECT_NEAR(0.1666665529, modelchecker.getReachabilityValue(allSatRegion.getUpperBounds()),  storm::settings::generalSettings().getPrecision());
    EXPECT_NEAR(0.1716553235, modelchecker.getReachabilityValue(exBothRegion.getLowerBounds()), storm::settings::generalSettings().getPrecision());
    EXPECT_NEAR(0.1709666953, modelchecker.getReachabilityValue(exBothRegion.getUpperBounds()),  storm::settings::generalSettings().getPrecision());
    EXPECT_NEAR(0.1826972576, modelchecker.getReachabilityValue(allVioRegion.getLowerBounds()), storm::settings::generalSettings().getPrecision());
    EXPECT_NEAR(0.1964429282, modelchecker.getReachabilityValue(allVioRegion.getUpperBounds()),  storm::settings::generalSettings().getPrecision());
   
    //test approximative method
    storm::settings::mutableRegionSettings().modifyModes(storm::settings::modules::RegionSettings::ApproxMode::TESTFIRST, storm::settings::modules::RegionSettings::SampleMode::INSTANTIATE, storm::settings::modules::RegionSettings::SmtMode::OFF);
    ASSERT_TRUE(storm::settings::regionSettings().doApprox());
    ASSERT_TRUE(storm::settings::regionSettings().doSample());
    ASSERT_FALSE(storm::settings::regionSettings().doSmt());
    modelchecker.checkRegion(allSatRegion);
    EXPECT_EQ((storm::modelchecker::region::RegionCheckResult::ALLSAT), allSatRegion.getCheckResult());
    modelchecker.checkRegion(exBothRegion);
    EXPECT_EQ((storm::modelchecker::region::RegionCheckResult::EXISTSBOTH), exBothRegion.getCheckResult());
    modelchecker.checkRegion(allVioRegion);
    EXPECT_EQ((storm::modelchecker::region::RegionCheckResult::ALLVIOLATED), allVioRegion.getCheckResult());

    storm::settings::mutableRegionSettings().resetModes();
}

TEST(SparseMdpRegionModelCheckerTest, coin_Prob) {
    
    std::string const& programFile = STORM_CPP_BASE_PATH "/examples/pmdp/consensus/coin2_2.nm";
    std::string const& formulaAsString = "P>0.25 [F \"finish_with_1\" ]";
    std::string const& constantsAsString = ""; //e.g. pL=0.9,TOACK=0.5
    
    //Build model, formula, region model checker
    boost::optional<storm::prism::Program> program=storm::parser::PrismParser::parse(programFile).simplify().simplify();
    program->checkValidity();
    storm::parser::FormulaParser formulaParser(program.get().getManager().getSharedPointer());
    std::vector<std::shared_ptr<storm::logic::Formula>> formulas = formulaParser.parseFromString(formulaAsString);
    typename storm::builder::ExplicitPrismModelBuilder<storm::RationalFunction>::Options options=storm::builder::ExplicitPrismModelBuilder<storm::RationalFunction>::Options(*formulas[0]);
    options.addConstantDefinitionsFromString(program.get(), constantsAsString); 
    options.preserveFormula(*formulas[0]);
    std::shared_ptr<storm::models::sparse::Model<storm::RationalFunction>> model = storm::builder::ExplicitPrismModelBuilder<storm::RationalFunction>().translateProgram(program.get(), options)->as<storm::models::sparse::Model<storm::RationalFunction>>();
    ASSERT_EQ(storm::models::ModelType::Mdp, model->getType());
    std::shared_ptr<storm::models::sparse::Mdp<storm::RationalFunction>> mdp = model->template as<storm::models::sparse::Mdp<storm::RationalFunction>>();
    storm::modelchecker::region::SparseMdpRegionModelChecker<storm::models::sparse::Mdp<storm::RationalFunction>, double> modelchecker(*mdp);
    ASSERT_TRUE(modelchecker.canHandle(*formulas[0]));
    modelchecker.specifyFormula(formulas[0]);
    
    //start testing
    auto allSatRegion=storm::modelchecker::region::ParameterRegion<storm::RationalFunction>::parseRegion("0.3<=p<=0.45,0.2<=q<=0.54");
    auto exBothRegion=storm::modelchecker::region::ParameterRegion<storm::RationalFunction>::parseRegion("0.4<=p<=0.65,0.5<=q<=0.7");
    auto allVioRegion=storm::modelchecker::region::ParameterRegion<storm::RationalFunction>::parseRegion("0.4<=p<=0.7,0.55<=q<=0.6");

    EXPECT_NEAR(0.9512773402, modelchecker.getReachabilityValue(allSatRegion.getLowerBounds()), storm::settings::generalSettings().getPrecision());
    EXPECT_NEAR(0.26787251126, modelchecker.getReachabilityValue(allSatRegion.getUpperBounds()),  storm::settings::generalSettings().getPrecision());
    EXPECT_NEAR(0.41879628383, modelchecker.getReachabilityValue(exBothRegion.getLowerBounds()), storm::settings::generalSettings().getPrecision());
    EXPECT_NEAR(0.01535089684, modelchecker.getReachabilityValue(exBothRegion.getUpperBounds()),  storm::settings::generalSettings().getPrecision());
    EXPECT_NEAR(0.24952612245, modelchecker.getReachabilityValue(allVioRegion.getLowerBounds()), storm::settings::generalSettings().getPrecision());
    EXPECT_NEAR(0.01711494956, modelchecker.getReachabilityValue(allVioRegion.getUpperBounds()),  storm::settings::generalSettings().getPrecision());
   
    //test approximative method
    storm::settings::mutableRegionSettings().modifyModes(storm::settings::modules::RegionSettings::ApproxMode::TESTFIRST, storm::settings::modules::RegionSettings::SampleMode::INSTANTIATE, storm::settings::modules::RegionSettings::SmtMode::OFF);
    ASSERT_TRUE(storm::settings::regionSettings().doApprox());
    ASSERT_TRUE(storm::settings::regionSettings().doSample());
    ASSERT_FALSE(storm::settings::regionSettings().doSmt());
    modelchecker.checkRegion(allSatRegion);
    EXPECT_EQ((storm::modelchecker::region::RegionCheckResult::ALLSAT), allSatRegion.getCheckResult());
    modelchecker.checkRegion(exBothRegion);
    EXPECT_EQ((storm::modelchecker::region::RegionCheckResult::EXISTSBOTH), exBothRegion.getCheckResult());
    modelchecker.checkRegion(allVioRegion);
    EXPECT_EQ((storm::modelchecker::region::RegionCheckResult::ALLVIOLATED), allVioRegion.getCheckResult());

    storm::settings::mutableRegionSettings().resetModes();
}

#endif
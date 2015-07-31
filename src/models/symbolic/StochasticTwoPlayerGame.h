#ifndef STORM_MODELS_SYMBOLIC_STOCHASTICTWOPLAYERGAME_H_
#define STORM_MODELS_SYMBOLIC_STOCHASTICTWOPLAYERGAME_H_

#include "src/models/symbolic/NondeterministicModel.h"
#include "src/utility/OsDetection.h"

namespace storm {
    namespace models {
        namespace symbolic {
            
            /*!
             * This class represents a discrete-time stochastic two-player game.
             */
            template<storm::dd::DdType Type>
            class StochasticTwoPlayerGame : public NondeterministicModel<Type> {
            public:
                StochasticTwoPlayerGame(StochasticTwoPlayerGame<Type> const& other) = default;
                StochasticTwoPlayerGame& operator=(StochasticTwoPlayerGame<Type> const& other) = default;
                
#ifndef WINDOWS
                StochasticTwoPlayerGame(StochasticTwoPlayerGame<Type>&& other) = default;
                StochasticTwoPlayerGame& operator=(StochasticTwoPlayerGame<Type>&& other) = default;
#endif
                
                /*!
                 * Constructs a model from the given data.
                 *
                 * @param manager The manager responsible for the decision diagrams.
                 * @param reachableStates A DD representing the reachable states.
                 * @param initialStates A DD representing the initial states of the model.
                 * @param transitionMatrix The matrix representing the transitions in the model.
                 * @param rowVariables The set of row meta variables used in the DDs.
                 * @param rowExpressionAdapter An object that can be used to translate expressions in terms of the row
                 * meta variables.
                 * @param columVariables The set of column meta variables used in the DDs.
                 * @param columnExpressionAdapter An object that can be used to translate expressions in terms of the
                 * column meta variables.
                 * @param rowColumnMetaVariablePairs All pairs of row/column meta variables.
                 * @param player1Variables The meta variables used to encode the nondeterministic choices of player 1.
                 * @param player2Variables The meta variables used to encode the nondeterministic choices of player 2.
                 * @param allNondeterminismVariables The meta variables used to encode the nondeterminism in the model.
                 * @param labelToExpressionMap A mapping from label names to their defining expressions.
                 * @param optionalStateRewardVector The reward values associated with the states.
                 * @param optionalTransitionRewardMatrix The reward values associated with the transitions of the model.
                 */
                StochasticTwoPlayerGame(std::shared_ptr<storm::dd::DdManager<Type>> manager,
                                        storm::dd::Bdd<Type> reachableStates,
                                        storm::dd::Bdd<Type> initialStates,
                                        storm::dd::Add<Type> transitionMatrix,
                                        std::set<storm::expressions::Variable> const& rowVariables,
                                        std::shared_ptr<storm::adapters::AddExpressionAdapter<Type>> rowExpressionAdapter,
                                        std::set<storm::expressions::Variable> const& columnVariables,
                                        std::shared_ptr<storm::adapters::AddExpressionAdapter<Type>> columnExpressionAdapter,
                                        std::vector<std::pair<storm::expressions::Variable, storm::expressions::Variable>> const& rowColumnMetaVariablePairs,
                                        std::set<storm::expressions::Variable> const& player1Variables,
                                        std::set<storm::expressions::Variable> const& player2Variables,
                                        std::set<storm::expressions::Variable> const& allNondeterminismVariables,
                                        std::map<std::string, storm::expressions::Expression> labelToExpressionMap = std::map<std::string, storm::expressions::Expression>(),
                                        boost::optional<storm::dd::Add<Type>> const& optionalStateRewardVector = boost::optional<storm::dd::Dd<Type>>(),
                                        boost::optional<storm::dd::Add<Type>> const& optionalTransitionRewardMatrix = boost::optional<storm::dd::Dd<Type>>());

                /*!
                 * Retrieeves the set of meta variables used to encode the nondeterministic choices of player 1.
                 *
                 * @return The set of meta variables used to encode the nondeterministic choices of player 1.
                 */
                std::set<storm::expressions::Variable> const& getPlayer1Variables() const;
                
                /*!
                 * Retrieeves the set of meta variables used to encode the nondeterministic choices of player 2.
                 *
                 * @return The set of meta variables used to encode the nondeterministic choices of player 2.
                 */
                std::set<storm::expressions::Variable> const& getPlayer2Variables() const;
                
            private:
                // The meta variables used to encode the nondeterministic choices of player 1.
                std::set<storm::expressions::Variable> player1Variables;
                
                // The meta variables used to encode the nondeterministic choices of player 2.
                std::set<storm::expressions::Variable> player2Variables;
            };
            
        } // namespace symbolic
    } // namespace models
} // namespace storm

#endif /* STORM_MODELS_SYMBOLIC_STOCHASTICTWOPLAYERGAME_H_ */

#ifndef STORM_MODELS_SPARSE_STANDARDREWARDMODEL_H_
#define STORM_MODELS_SPARSE_STANDARDREWARDMODEL_H_

#include <vector>
#include <boost/optional.hpp>

#include "src/storage/SparseMatrix.h"
#include "src/utility/OsDetection.h"

namespace storm {
    namespace models {
        namespace sparse {
            template <typename ValueType>
            class StandardRewardModel {
            public:
                /*!
                 * Constructs a reward model by copying the given data.
                 *
                 * @param optionalStateRewardVector The reward values associated with the states.
                 * @param optionalStateActionRewardVector The reward values associated with state-action pairs.
                 * @param optionalTransitionRewardMatrix The reward values associated with the transitions of the model.
                 */
                StandardRewardModel(boost::optional<std::vector<ValueType>> const& optionalStateRewardVector = boost::optional<std::vector<ValueType>>(),
                                    boost::optional<std::vector<ValueType>> const& optionalStateActionRewardVector = boost::optional<std::vector<ValueType>>(),
                                    boost::optional<storm::storage::SparseMatrix<ValueType>> const& optionalTransitionRewardMatrix = boost::optional<storm::storage::SparseMatrix<ValueType>>());
                
                /*!
                 * Constructs a reward model by moving the given data.
                 *
                 * @param optionalStateRewardVector The reward values associated with the states.
                 * @param optionalStateActionRewardVector The reward values associated with state-action pairs.
                 * @param optionalTransitionRewardMatrix The reward values associated with the transitions of the model.
                 */
                StandardRewardModel(boost::optional<std::vector<ValueType>>&& optionalStateRewardVector = boost::optional<std::vector<ValueType>>(),
                            boost::optional<std::vector<ValueType>>&& optionalStateActionRewardVector = boost::optional<std::vector<ValueType>>(),
                            boost::optional<storm::storage::SparseMatrix<ValueType>>&& optionalTransitionRewardMatrix = boost::optional<storm::storage::SparseMatrix<ValueType>>());
                
                StandardRewardModel(StandardRewardModel<ValueType> const& dtmc) = default;
                StandardRewardModel& operator=(StandardRewardModel<ValueType> const& dtmc) = default;
                
#ifndef WINDOWS
                StandardRewardModel(StandardRewardModel<ValueType>&& dtmc) = default;
                StandardRewardModel& operator=(StandardRewardModel<ValueType>&& dtmc) = default;
#endif
                
                /*!
                 * Retrieves whether the reward model has state rewards.
                 *
                 * @return True iff the reward model has state rewards.
                 */
                bool hasStateRewards() const;

                /*!
                 * Retrieves the state rewards of the reward model. Note that it is illegal to call this function if the
                 * reward model does not have state rewards.
                 *
                 * @return The state reward vector.
                 */
                std::vector<ValueType> const& getStateRewardVector() const;
                
                /*!
                 * Retrieves an optional value that contains the state reward vector if there is one.
                 *
                 * @return The state reward vector if there is one.
                 */
                boost::optional<std::vector<ValueType>> const& getOptionalStateRewardVector() const;
                
                /*!
                 * Retrieves whether the reward model has state-action rewards.
                 *
                 * @return True iff the reward model has state-action rewards.
                 */
                bool hasStateActionRewards() const;

                /*!
                 * Retrieves the state-action rewards of the reward model. Note that it is illegal to call this function
                 * if the reward model does not have state-action rewards.
                 *
                 * @return The state-action reward vector.
                 */
                std::vector<ValueType> const& getStateActionRewardVector() const;
                
                /*!
                 * Retrieves an optional value that contains the state-action reward vector if there is one.
                 *
                 * @return The state-action reward vector if there is one.
                 */
                boost::optional<std::vector<ValueType>> const& getOptionalStateActionRewardVector() const;
                
                /*!
                 * Retrieves whether the reward model has transition rewards.
                 *
                 * @return True iff the reward model has transition rewards.
                 */
                bool hasTransitionRewards() const;
                
                /*!
                 * Retrieves the transition rewards of the reward model. Note that it is illegal to call this function
                 * if the reward model does not have transition rewards.
                 *
                 * @return The transition reward matrix.
                 */
                storm::storage::SparseMatrix<ValueType> const& getTransitionRewardMatrix() const;
                
                /*!
                 * Retrieves an optional value that contains the transition reward matrix if there is one.
                 *
                 * @return The transition reward matrix if there is one.
                 */
                boost::optional<storm::storage::SparseMatrix<ValueType>> const& getOptionalTransitionRewardMatrix() const;
                
                /*!
                 * Creates a new reward model by restricting the actions of the action-based rewards.
                 *
                 * @param enabledActions A bit vector representing the enabled actions.
                 * @return The restricted reward model.
                 */
                StandardRewardModel<ValueType> restrictActions(storm::storage::BitVector const& enabledActions) const;
                
                /*!
                 * Converts the transition-based rewards to state-action rewards by taking the average of each row. Note
                 * that this preserves expected rewards, but not all reward-based properties. Also note that it is only
                 * legal to do this transformation if the reward model has transition rewards.
                 *
                 * @param transitionMatrix The transition matrix that is used to weight the rewards in the reward matrix.
                 */
                template<typename MatrixValueType>
                void convertTransitionRewardsToStateActionRewards(storm::storage::SparseMatrix<MatrixValueType> const& transitionMatrix);
                
                /*!
                 * Retrieves whether the reward model is empty, i.e. contains no state-, state-action- or transition-based
                 * rewards.
                 *
                 * @return True iff the reward model is empty.
                 */
                bool empty() const;
                
                /*!
                 * Retrieves (an approximation of) the size of the model in bytes.
                 *
                 * @return The size of the internal representation of the model measured in bytes.
                 */
                std::size_t getSizeInBytes() const;
                
            private:
                // An (optional) vector representing the state rewards.
                boost::optional<std::vector<ValueType>> optionalStateRewardVector;
                
                // An (optional) vector representing the state-action rewards.
                boost::optional<std::vector<ValueType>> optionalStateActionRewardVector;
                
                // An (optional) matrix representing the transition rewards.
                boost::optional<storm::storage::SparseMatrix<ValueType>> optionalTransitionRewardMatrix;
            };
        }
    }
}

#endif /* STORM_MODELS_SPARSE_STANDARDREWARDMODEL_H_ */
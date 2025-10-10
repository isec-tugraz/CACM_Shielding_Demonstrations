#include "SparseNondeterministicGameInfiniteHorizonHelper.h"

#include "storm/modelchecker/helper/infinitehorizon/internal/LraViHelper.h"

#include "storm/storage/SparseMatrix.h"
#include "storm/storage/MaximalEndComponentDecomposition.h"
#include "storm/storage/GameMaximalEndComponentDecomposition.h"
#include "storm/storage/Scheduler.h"

#include "storm/solver/MinMaxLinearEquationSolver.h"
#include "storm/solver/Multiplier.h"

#include "storm/utility/solver.h"
#include "storm/utility/vector.h"

#include "storm/environment/solver/LongRunAverageSolverEnvironment.h"
#include "storm/environment/solver/MinMaxSolverEnvironment.h"

#include "storm/exceptions/UnmetRequirementException.h"
#include "storm/exceptions/InternalException.h"

namespace storm {
    namespace modelchecker {
        namespace helper {

            template <typename ValueType>
            SparseNondeterministicGameInfiniteHorizonHelper<ValueType>::SparseNondeterministicGameInfiniteHorizonHelper(storm::storage::SparseMatrix<ValueType> const& transitionMatrix, storm::storage::BitVector statesOfCoalition) : SparseInfiniteHorizonHelper<ValueType, true>(transitionMatrix), statesOfCoalition(statesOfCoalition) {
                // Intentionally left empty.
            }

            template <typename ValueType>
            std::vector<uint64_t> const& SparseNondeterministicGameInfiniteHorizonHelper<ValueType>::getProducedOptimalChoices() const {
                STORM_LOG_ASSERT(this->isProduceSchedulerSet(), "Trying to get the produced optimal choices although no scheduler was requested.");
                STORM_LOG_ASSERT(this->_producedOptimalChoices.is_initialized(), "Trying to get the produced optimal choices but none were available. Was there a computation call before?");
                return this->_producedOptimalChoices.get();
            }

            template <typename ValueType>
            std::vector<uint64_t>& SparseNondeterministicGameInfiniteHorizonHelper<ValueType>::getProducedOptimalChoices() {
                STORM_LOG_ASSERT(this->isProduceSchedulerSet(), "Trying to get the produced optimal choices although no scheduler was requested.");
                STORM_LOG_ASSERT(this->_producedOptimalChoices.is_initialized(), "Trying to get the produced optimal choices but none were available. Was there a computation call before?");
                return this->_producedOptimalChoices.get();
            }

            template <typename ValueType>
            storm::storage::Scheduler<ValueType> SparseNondeterministicGameInfiniteHorizonHelper<ValueType>::extractScheduler() const {
                auto const& optimalChoices = getProducedOptimalChoices();
                storm::storage::Scheduler<ValueType> scheduler(optimalChoices.size());
                for (uint64_t state = 0; state < optimalChoices.size(); ++state) {
                        scheduler.setChoice(optimalChoices[state], state);
                }
                return scheduler;
            }

            template <typename ValueType>
            std::vector<ValueType> SparseNondeterministicGameInfiniteHorizonHelper<ValueType>::getChoiceValues() const {
                STORM_LOG_ASSERT(this->isProduceChoiceValuesSet(), "Trying to get the computed choice values although this was not requested.");
                STORM_LOG_ASSERT(this->_choiceValues.is_initialized(), "Trying to get the computed choice values but none were available. Was there a computation call before?");
                return this->_choiceValues.get();
            }

            template <typename ValueType>
            void SparseNondeterministicGameInfiniteHorizonHelper<ValueType>::createDecomposition() {
                // TODO This needs to be changed to return the whole model as one component as long as there is no overwritten version of MaximalEndComponentDecomposition for SMGs.
                if (this->_longRunComponentDecomposition == nullptr) {
                    // The decomposition has not been provided or computed, yet.
                    if (this->_backwardTransitions == nullptr) {
                        this->_computedBackwardTransitions = std::make_unique<storm::storage::SparseMatrix<ValueType>>(this->_transitionMatrix.transpose(true));
                        this->_backwardTransitions = this->_computedBackwardTransitions.get();
                    }
                    this->_computedLongRunComponentDecomposition = std::make_unique<storm::storage::GameMaximalEndComponentDecomposition<ValueType>>(this->_transitionMatrix, *this->_backwardTransitions);

                    this->_longRunComponentDecomposition = this->_computedLongRunComponentDecomposition.get();
                }
            }

            template <typename ValueType>
            std::vector<ValueType> SparseNondeterministicGameInfiniteHorizonHelper<ValueType>::computeLongRunAverageValues(Environment const& env, ValueGetter const& stateValuesGetter,  ValueGetter const& actionValuesGetter) {
                auto underlyingSolverEnvironment = env;
                std::vector<ValueType> componentLraValues;
                createDecomposition();
                componentLraValues.reserve(this->_longRunComponentDecomposition->size());
                for (auto const& c : *(this->_longRunComponentDecomposition)) {
                    componentLraValues.push_back(computeLraForComponent(underlyingSolverEnvironment, stateValuesGetter, actionValuesGetter, c));
                }
                return componentLraValues;
            }

            template <typename ValueType>
            ValueType SparseNondeterministicGameInfiniteHorizonHelper<ValueType>::computeLraForComponent(Environment const& env, ValueGetter const& stateRewardsGetter, ValueGetter const& actionRewardsGetter, storm::storage::MaximalEndComponent const& component) {
                // Allocate memory for the nondeterministic choices.
                if (this->isProduceSchedulerSet()) {
                    if (!this->_producedOptimalChoices.is_initialized()) {
                        this->_producedOptimalChoices.emplace();
                    }
                    this->_producedOptimalChoices->resize(this->_transitionMatrix.getRowGroupCount());
                }
                // Allocate memory for the choice values.
                if (this->isProduceChoiceValuesSet()) {
                    if (!this->_choiceValues.is_initialized()) {
                        this->_choiceValues.emplace();
                    }
                    this->_choiceValues->resize(this->_transitionMatrix.getRowCount());
                }

                storm::solver::LraMethod method = env.solver().lra().getNondetLraMethod();
                if (method == storm::solver::LraMethod::LinearProgramming) {
                    STORM_LOG_THROW(false, storm::exceptions::InvalidSettingsException, "Unsupported technique.");
                } else if (method == storm::solver::LraMethod::ValueIteration) {
                    return computeLraVi(env, stateRewardsGetter, actionRewardsGetter, component);
                } else {
                    STORM_LOG_THROW(false, storm::exceptions::InvalidSettingsException, "Unsupported technique.");
                }
            }

            template <typename ValueType>
            ValueType SparseNondeterministicGameInfiniteHorizonHelper<ValueType>::computeLraVi(Environment const& env, ValueGetter const& stateRewardsGetter, ValueGetter const& actionRewardsGetter, storm::storage::MaximalEndComponent const& mec) {

                // Collect some parameters of the computation
                ValueType aperiodicFactor = storm::utility::convertNumber<ValueType>(env.solver().lra().getAperiodicFactor());
                std::vector<uint64_t>* optimalChoices = nullptr;
                if (this->isProduceSchedulerSet()) {
                    optimalChoices = &this->_producedOptimalChoices.get();
                }
                std::vector<ValueType>* choiceValues = nullptr;
                if (this->isProduceChoiceValuesSet()) {
                    choiceValues = &this->_choiceValues.get();
                }

                // Now create a helper and perform the algorithm
                if (this->isContinuousTime()) {
                    STORM_LOG_THROW(false, storm::exceptions::InternalException, "We cannot handle continuous time games.");
                } else {
                    storm::modelchecker::helper::internal::LraViHelper<ValueType, storm::storage::MaximalEndComponent, storm::modelchecker::helper::internal::LraViTransitionsType::GameNondetTsNoIs> viHelper(mec, this->_transitionMatrix, aperiodicFactor, nullptr, nullptr, &statesOfCoalition);
                    return viHelper.performValueIteration(env, stateRewardsGetter, actionRewardsGetter, nullptr, &this->getOptimizationDirection(), optimalChoices, choiceValues);
                }
            }

            template <typename ValueType>
            std::vector<ValueType> SparseNondeterministicGameInfiniteHorizonHelper<ValueType>::buildAndSolveSsp(Environment const& env, std::vector<ValueType> const& componentLraValues) {
                STORM_LOG_THROW(false, storm::exceptions::InternalException, "We do not create compositions for LRA for SMGs, solving a stochastic shortest path problem is not available.");
            }


            template class SparseNondeterministicGameInfiniteHorizonHelper<double>;
            template class SparseNondeterministicGameInfiniteHorizonHelper<storm::RationalNumber>;

        }
    }
}

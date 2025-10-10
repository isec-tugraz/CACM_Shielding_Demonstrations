#include "ShieldHandling.h"

namespace tempest {
    namespace shields {
        template<typename ValueType, typename IndexType>
        std::unique_ptr<tempest::shields::AbstractShield<ValueType, IndexType>> createShield(std::shared_ptr<storm::models::sparse::Model<ValueType>> model, std::vector<ValueType> const& choiceValues, std::shared_ptr<storm::logic::ShieldExpression const> const& shieldingExpression, storm::OptimizationDirection optimizationDirection, storm::storage::BitVector relevantStates, boost::optional<storm::storage::BitVector> coalitionStates) {
            if(coalitionStates.is_initialized()) coalitionStates.get().complement();
            if(shieldingExpression->isPreSafetyShield()) {
                PreShield<ValueType, IndexType> shield(model->getTransitionMatrix().getRowGroupIndices(), choiceValues, shieldingExpression, optimizationDirection, relevantStates, coalitionStates);
                return std::make_unique<tempest::shields::PreShield<ValueType, IndexType>>(shield);
            } else if(shieldingExpression->isPostSafetyShield()) {
                PostShield<ValueType, IndexType> shield(model->getTransitionMatrix().getRowGroupIndices(), choiceValues, shieldingExpression, optimizationDirection, relevantStates, coalitionStates);
                return std::make_unique<tempest::shields::PostShield<ValueType, IndexType>>(shield);
            } else {
                STORM_LOG_THROW(false, storm::exceptions::InvalidArgumentException, "Unknown Shielding Type: " + shieldingExpression->typeToString());
            }
        }   

        template<typename ValueType, typename IndexType>
        std::unique_ptr<tempest::shields::AbstractShield<ValueType, IndexType>> createQuantitativeShield(std::shared_ptr<storm::models::sparse::Model<ValueType>> model, std::vector<ValueType> const& choiceValues, std::shared_ptr<storm::logic::ShieldExpression const> const& shieldingExpression, storm::OptimizationDirection optimizationDirection, storm::storage::BitVector relevantStates, boost::optional<storm::storage::BitVector> coalitionStates) {
            if(coalitionStates.is_initialized()) coalitionStates.get().complement(); // TODO CHECK THIS!!!
            if(shieldingExpression->isOptimalPreShield()) {
                PreShield<ValueType, IndexType> shield(model->getTransitionMatrix().getRowGroupIndices(), choiceValues, shieldingExpression, optimizationDirection, relevantStates, coalitionStates);
                return std::make_unique<tempest::shields::PreShield<ValueType, IndexType>>(shield);
            } else if(shieldingExpression->isOptimalPostShield()) {
                 PostShield<ValueType, IndexType> shield(model->getTransitionMatrix().getRowGroupIndices(), choiceValues, shieldingExpression, optimizationDirection, relevantStates, coalitionStates);
                return std::make_unique<tempest::shields::PostShield<ValueType, IndexType>>(shield);

            } else {
                STORM_LOG_THROW(false, storm::exceptions::InvalidArgumentException, "Unknown Shielding Type: " + shieldingExpression->typeToString());
            }
        }

        // Explicitly instantiate appropriate
        template std::unique_ptr<tempest::shields::AbstractShield<double, typename storm::storage::SparseMatrix<double>::index_type>> createShield<double, typename storm::storage::SparseMatrix<double>::index_type>(std::shared_ptr<storm::models::sparse::Model<double>> model, std::vector<double> const& choiceValues, std::shared_ptr<storm::logic::ShieldExpression const> const& shieldingExpression, storm::OptimizationDirection optimizationDirection, storm::storage::BitVector relevantStates, boost::optional<storm::storage::BitVector> coalitionStates);
        template std::unique_ptr<tempest::shields::AbstractShield<double, typename storm::storage::SparseMatrix<double>::index_type>> createQuantitativeShield<double, typename storm::storage::SparseMatrix<double>::index_type>(std::shared_ptr<storm::models::sparse::Model<double>> model, std::vector<double> const& choiceValues, std::shared_ptr<storm::logic::ShieldExpression const> const& shieldingExpression, storm::OptimizationDirection optimizationDirection, storm::storage::BitVector relevantStates, boost::optional<storm::storage::BitVector> coalitionStates);
#ifdef STORM_HAVE_CARL
        template std::unique_ptr<tempest::shields::AbstractShield<storm::RationalNumber, typename storm::storage::SparseMatrix<storm::RationalNumber>::index_type>> createShield<storm::RationalNumber, typename storm::storage::SparseMatrix<storm::RationalNumber>::index_type>(std::shared_ptr<storm::models::sparse::Model<storm::RationalNumber>> model, std::vector<storm::RationalNumber> const& choiceValues, std::shared_ptr<storm::logic::ShieldExpression const> const& shieldingExpression, storm::OptimizationDirection optimizationDirection, storm::storage::BitVector relevantStates, boost::optional<storm::storage::BitVector> coalitionStates); 
        template std::unique_ptr<tempest::shields::AbstractShield<storm::RationalNumber, typename storm::storage::SparseMatrix<storm::RationalNumber>::index_type>> createQuantitativeShield<storm::RationalNumber, typename storm::storage::SparseMatrix<storm::RationalNumber>::index_type>(std::shared_ptr<storm::models::sparse::Model<storm::RationalNumber>> model, std::vector<storm::RationalNumber> const& choiceValues, std::shared_ptr<storm::logic::ShieldExpression const> const& shieldingExpression, storm::OptimizationDirection optimizationDirection, storm::storage::BitVector relevantStates, boost::optional<storm::storage::BitVector> coalitionStates);
#endif
    }
}

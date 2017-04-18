#ifndef STORM_SOLVER_TOPOLOGICALVALUEITERATIONMINMAXLINEAREQUATIONSOLVER_H_
#define STORM_SOLVER_TOPOLOGICALVALUEITERATIONMINMAXLINEAREQUATIONSOLVER_H_

#include "storm/solver/MinMaxLinearEquationSolver.h"
#include "storm/storage/StronglyConnectedComponentDecomposition.h"
#include "storm/storage/SparseMatrix.h"
#include "storm/exceptions/NotImplementedException.h"
#include "storm/exceptions/NotSupportedException.h"

#include <utility>
#include <vector>

#include "storm-config.h"
#ifdef STORM_HAVE_CUDA
#include "cudaForStorm.h"
#endif

namespace storm {
    namespace solver {
        
        /*!
         * A class that uses SCC Decompositions to solve a min/max linear equation system.
         */
        template<class ValueType>
        class TopologicalMinMaxLinearEquationSolver : public MinMaxLinearEquationSolver<ValueType> {
        public:
            /*!
             * Constructs a min-max linear equation solver with parameters being set according to the settings
             * object.
             *
             * @param A The matrix defining the coefficients of the linear equation system.
             */
            TopologicalMinMaxLinearEquationSolver(storm::storage::SparseMatrix<ValueType> const& A, double precision = 1e-6, uint_fast64_t maximalNumberOfIterations = 20000, bool relative = true);
                        
            virtual bool solveEquations(OptimizationDirection dir, std::vector<ValueType>& x, std::vector<ValueType> const& b) const override;
            
            virtual void repeatedMultiply(OptimizationDirection dir, std::vector<ValueType>& x, std::vector<ValueType>* b, uint_fast64_t n) const override;

            virtual ValueType getPrecision() const override;
            virtual bool getRelative() const override;

        private:
            storm::storage::SparseMatrix<ValueType> const& A;
            double precision;
            uint_fast64_t maximalNumberOfIterations;
            bool relative;
            
            bool enableCuda;
            /*!
             * Given a topological sort of a SCC Decomposition, this will calculate the optimal grouping of SCCs with respect to the size of the GPU memory.
             */
            std::vector<std::pair<bool, storm::storage::StateBlock>> getOptimalGroupingFromTopologicalSccDecomposition(storm::storage::StronglyConnectedComponentDecomposition<ValueType> const& sccDecomposition, std::vector<uint_fast64_t> const& topologicalSort, storm::storage::SparseMatrix<ValueType> const& matrix) const;
        };
        
        template <typename IndexType, typename ValueType>
        bool __basicValueIteration_mvReduce_minimize(uint_fast64_t const, double const, bool const, std::vector<uint_fast64_t> const&, std::vector<storm::storage::MatrixEntry<IndexType, ValueType>> const&, std::vector<ValueType>& x, std::vector<ValueType> const&, std::vector<uint_fast64_t> const&, size_t&) {
            //
            STORM_LOG_THROW(false, storm::exceptions::NotImplementedException, "Unsupported template arguments.");
        }
        template <>
        inline bool __basicValueIteration_mvReduce_minimize<uint_fast64_t, double>(uint_fast64_t const maxIterationCount, double const precision, bool const relativePrecisionCheck, std::vector<uint_fast64_t> const& matrixRowIndices, std::vector<storm::storage::MatrixEntry<uint_fast64_t, double>> const& columnIndicesAndValues, std::vector<double>& x, std::vector<double> const& b, std::vector<uint_fast64_t> const& nondeterministicChoiceIndices, size_t& iterationCount) {

            (void)maxIterationCount;
            (void)precision;
            (void)relativePrecisionCheck;
            (void)matrixRowIndices;
            (void)columnIndicesAndValues;
            (void)x;
            (void)b;
            (void)nondeterministicChoiceIndices;
            (void)iterationCount;

#ifdef STORM_HAVE_CUDA
            return basicValueIteration_mvReduce_uint64_double_minimize(maxIterationCount, precision, relativePrecisionCheck, matrixRowIndices, columnIndicesAndValues, x, b, nondeterministicChoiceIndices, iterationCount);
#else
            STORM_LOG_THROW(false, storm::exceptions::NotSupportedException, "Storm is compiled without CUDA support.");
#endif
        }
        template <>
        inline bool __basicValueIteration_mvReduce_minimize<uint_fast64_t, float>(uint_fast64_t const maxIterationCount, double const precision, bool const relativePrecisionCheck, std::vector<uint_fast64_t> const& matrixRowIndices, std::vector<storm::storage::MatrixEntry<uint_fast64_t, float>> const& columnIndicesAndValues, std::vector<float>& x, std::vector<float> const& b, std::vector<uint_fast64_t> const& nondeterministicChoiceIndices, size_t& iterationCount) {

            (void)maxIterationCount;
            (void)precision;
            (void)relativePrecisionCheck;
            (void)matrixRowIndices;
            (void)columnIndicesAndValues;
            (void)x;
            (void)b;
            (void)nondeterministicChoiceIndices;
            (void)iterationCount;

#ifdef STORM_HAVE_CUDA
            return basicValueIteration_mvReduce_uint64_float_minimize(maxIterationCount, precision, relativePrecisionCheck, matrixRowIndices, columnIndicesAndValues, x, b, nondeterministicChoiceIndices, iterationCount);
#else
            STORM_LOG_THROW(false, storm::exceptions::NotSupportedException, "Storm is compiled without CUDA support.");
#endif
        }
        
        template <typename IndexType, typename ValueType>
        bool __basicValueIteration_mvReduce_maximize(uint_fast64_t const, double const, bool const, std::vector<uint_fast64_t> const&, std::vector<storm::storage::MatrixEntry<IndexType, ValueType>> const&, std::vector<ValueType>&, std::vector<ValueType> const&, std::vector<uint_fast64_t> const&, size_t&) {
            STORM_LOG_THROW(false, storm::exceptions::NotImplementedException, "Unsupported template arguments.");
        }
        template <>
        inline bool __basicValueIteration_mvReduce_maximize<uint_fast64_t, double>(uint_fast64_t const maxIterationCount, double const precision, bool const relativePrecisionCheck, std::vector<uint_fast64_t> const& matrixRowIndices, std::vector<storm::storage::MatrixEntry<uint_fast64_t, double>> const& columnIndicesAndValues, std::vector<double>& x, std::vector<double> const& b, std::vector<uint_fast64_t> const& nondeterministicChoiceIndices, size_t& iterationCount) {

            (void)maxIterationCount;
            (void)precision;
            (void)relativePrecisionCheck;
            (void)matrixRowIndices;
            (void)columnIndicesAndValues;
            (void)x;
            (void)b;
            (void)nondeterministicChoiceIndices;
            (void)iterationCount;

#ifdef STORM_HAVE_CUDA
            return basicValueIteration_mvReduce_uint64_double_maximize(maxIterationCount, precision, relativePrecisionCheck, matrixRowIndices, columnIndicesAndValues, x, b, nondeterministicChoiceIndices, iterationCount);
#else
            STORM_LOG_THROW(false, storm::exceptions::NotSupportedException, "Storm is compiled without CUDA support.");
#endif
        }
        template <>
        inline bool __basicValueIteration_mvReduce_maximize<uint_fast64_t, float>(uint_fast64_t const maxIterationCount, double const precision, bool const relativePrecisionCheck, std::vector<uint_fast64_t> const& matrixRowIndices, std::vector<storm::storage::MatrixEntry<uint_fast64_t, float>> const& columnIndicesAndValues, std::vector<float>& x, std::vector<float> const& b, std::vector<uint_fast64_t> const& nondeterministicChoiceIndices, size_t& iterationCount) {

            (void)maxIterationCount;
            (void)precision;
            (void)relativePrecisionCheck;
            (void)matrixRowIndices;
            (void)columnIndicesAndValues;
            (void)x;
            (void)b;
            (void)nondeterministicChoiceIndices;
            (void)iterationCount;

#ifdef STORM_HAVE_CUDA
            return basicValueIteration_mvReduce_uint64_float_maximize(maxIterationCount, precision, relativePrecisionCheck, matrixRowIndices, columnIndicesAndValues, x, b, nondeterministicChoiceIndices, iterationCount);
#else
            STORM_LOG_THROW(false, storm::exceptions::NotSupportedException, "Storm is compiled without CUDA support.");
#endif
        }
        
        template<typename ValueType>
        class TopologicalMinMaxLinearEquationSolverFactory : public MinMaxLinearEquationSolverFactory<ValueType> {
        public:
            TopologicalMinMaxLinearEquationSolverFactory(bool trackScheduler = false);
            
            virtual std::unique_ptr<MinMaxLinearEquationSolver<ValueType>> create(storm::storage::SparseMatrix<ValueType> const& matrix) const override;
            virtual std::unique_ptr<MinMaxLinearEquationSolver<ValueType>> create(storm::storage::SparseMatrix<ValueType>&& matrix) const override;
        };

    } // namespace solver
} // namespace storm

#endif /* STORM_SOLVER_TOPOLOGICALVALUEITERATIONMINMAXLINEAREQUATIONSOLVER_H_ */

#include "basicValueIteration.h"
#define CUSP_USE_TEXTURE_MEMORY

#include <iostream>
#include <chrono>

#include <cuda_runtime.h>
#include "cusparse_v2.h"

#include "utility.h"

#include "cuspExtension.h"
#include <thrust/transform.h>
#include <thrust/device_ptr.h>
#include <thrust/functional.h>


#define CUDA_CHECK_ALL_ERRORS() do { \
	cudaError_t errSync  = cudaGetLastError(); \
	cudaError_t errAsync = cudaDeviceSynchronize(); \
	if (errSync != cudaSuccess) { \
		std::cout << "(DLL) Sync kernel error: " << cudaGetErrorString(errSync) << " (Code: " << errSync << ")" << std::endl; \
	} \
	if (errAsync != cudaSuccess) { \
		std::cout << "(DLL) Async kernel error: " << cudaGetErrorString(errAsync) << " (Code: " << errAsync << ")" << std::endl; \
	} } while(false)


template<typename T, bool Relative>
struct equalModuloPrecision : public thrust::binary_function<T,T,T>
{
__host__ __device__ T operator()(const T &x, const T &y) const
{
    if (Relative) {
		const T result = (x - y) / y;
		return ((result >= 0) ? (result) : (-result));
    } else {
        const T result = (x - y);
		return ((result >= 0) ? (result) : (-result));
    }
}
};

template<typename IndexType, typename ValueType>
void exploadVector(std::vector<std::pair<IndexType, ValueType>> const& inputVector, std::vector<IndexType>& indexVector, std::vector<ValueType>& valueVector) {
	indexVector.reserve(inputVector.size());
	valueVector.reserve(inputVector.size());
	for (size_t i = 0; i < inputVector.size(); ++i) {
		indexVector.push_back(inputVector.at(i).first);
		valueVector.push_back(inputVector.at(i).second);
	}
}

template <bool Minimize, bool Relative, typename IndexType, typename ValueType>
void basicValueIteration_mvReduce(uint_fast64_t const maxIterationCount, ValueType const precision, std::vector<IndexType> const& matrixRowIndices, std::vector<std::pair<IndexType, ValueType>> const& columnIndicesAndValues, std::vector<ValueType>& x, std::vector<ValueType> const& b, std::vector<IndexType> const& nondeterministicChoiceIndices) {
	
	std::vector<IndexType> matrixColumnIndices;
	std::vector<ValueType> matrixValues;
	exploadVector<IndexType, ValueType>(columnIndicesAndValues, matrixColumnIndices, matrixValues);
	
	IndexType* device_matrixRowIndices = nullptr;
	IndexType* device_matrixColIndices = nullptr;
	ValueType* device_matrixValues = nullptr;
	ValueType* device_x = nullptr;
	ValueType* device_xSwap = nullptr;
	ValueType* device_b = nullptr;
	ValueType* device_multiplyResult = nullptr;
	IndexType* device_nondeterministicChoiceIndices = nullptr;

	std::cout.sync_with_stdio(true);
	std::cout << "(DLL) Device has " << getTotalCudaMemory() << " Bytes of Memory with " << getFreeCudaMemory() << "Bytes free (" << (static_cast<double>(getFreeCudaMemory()) / static_cast<double>(getTotalCudaMemory()))*100 << "%)." << std::endl; 
	size_t memSize = sizeof(IndexType) * matrixRowIndices.size() + sizeof(IndexType) * columnIndicesAndValues.size() * 2 + sizeof(ValueType) * x.size() + sizeof(ValueType) * x.size() + sizeof(ValueType) * b.size() + sizeof(ValueType) * b.size() + sizeof(IndexType) * nondeterministicChoiceIndices.size();
	std::cout << "(DLL) We will allocate " << memSize << " Bytes." << std::endl;

	const IndexType matrixRowCount = matrixRowIndices.size() - 1;
	const IndexType matrixColCount = nondeterministicChoiceIndices.size() - 1;
	const IndexType matrixNnzCount = columnIndicesAndValues.size();

	cudaError_t cudaMallocResult;

	bool converged = false;
	uint_fast64_t iterationCount = 0;

	CUDA_CHECK_ALL_ERRORS();
	cudaMallocResult = cudaMalloc(reinterpret_cast<void**>(&device_matrixRowIndices), sizeof(IndexType) * (matrixRowCount + 1));
	if (cudaMallocResult != cudaSuccess) {
		std::cout << "Could not allocate memory for Matrix Row Indices, Error Code " << cudaMallocResult << "." << std::endl;
		goto cleanup;
	}

	CUDA_CHECK_ALL_ERRORS();
	cudaMallocResult = cudaMalloc(reinterpret_cast<void**>(&device_matrixColIndices), sizeof(IndexType) * matrixNnzCount);
	if (cudaMallocResult != cudaSuccess) {
		std::cout << "Could not allocate memory for Matrix Column Indices, Error Code " << cudaMallocResult << "." << std::endl;
		goto cleanup;
	}

	CUDA_CHECK_ALL_ERRORS();
	cudaMallocResult = cudaMalloc(reinterpret_cast<void**>(&device_matrixValues), sizeof(ValueType) * matrixNnzCount);
	if (cudaMallocResult != cudaSuccess) {
		std::cout << "Could not allocate memory for Matrix Values, Error Code " << cudaMallocResult << "." << std::endl;
		goto cleanup;
	}

	CUDA_CHECK_ALL_ERRORS();
	cudaMallocResult = cudaMalloc(reinterpret_cast<void**>(&device_x), sizeof(ValueType) * matrixColCount);
	if (cudaMallocResult != cudaSuccess) {
		std::cout << "Could not allocate memory for Vector x, Error Code " << cudaMallocResult << "." << std::endl;
		goto cleanup;
	}

	CUDA_CHECK_ALL_ERRORS();
	cudaMallocResult = cudaMalloc(reinterpret_cast<void**>(&device_xSwap), sizeof(ValueType) * matrixColCount);
	if (cudaMallocResult != cudaSuccess) {
		std::cout << "Could not allocate memory for Vector x swap, Error Code " << cudaMallocResult << "." << std::endl;
		goto cleanup;
	}

	CUDA_CHECK_ALL_ERRORS();
	cudaMallocResult = cudaMalloc(reinterpret_cast<void**>(&device_b), sizeof(ValueType) * matrixRowCount);
	if (cudaMallocResult != cudaSuccess) {
		std::cout << "Could not allocate memory for Vector b, Error Code " << cudaMallocResult << "." << std::endl;
		goto cleanup;
	}

	CUDA_CHECK_ALL_ERRORS();
	cudaMallocResult = cudaMalloc(reinterpret_cast<void**>(&device_multiplyResult), sizeof(ValueType) * matrixRowCount);
	if (cudaMallocResult != cudaSuccess) {
		std::cout << "Could not allocate memory for Vector multiplyResult, Error Code " << cudaMallocResult << "." << std::endl;
		goto cleanup;
	}

	CUDA_CHECK_ALL_ERRORS();
	cudaMallocResult = cudaMalloc(reinterpret_cast<void**>(&device_nondeterministicChoiceIndices), sizeof(IndexType) * (matrixRowCount + 1));
	if (cudaMallocResult != cudaSuccess) {
		std::cout << "Could not allocate memory for Nondeterministic Choice Indices, Error Code " << cudaMallocResult << "." << std::endl;
		goto cleanup;
	}

	// Memory allocated, copy data to device
	cudaError_t cudaCopyResult;

	CUDA_CHECK_ALL_ERRORS();
	cudaCopyResult = cudaMemcpy(device_matrixRowIndices, matrixRowIndices.data(), sizeof(IndexType) * (matrixRowCount + 1), cudaMemcpyHostToDevice);
	if (cudaCopyResult != cudaSuccess) {
		std::cout << "Could not copy data for Matrix Row Indices, Error Code " << cudaCopyResult << std::endl;
		goto cleanup;
	}

	CUDA_CHECK_ALL_ERRORS();
	cudaCopyResult = cudaMemcpy(device_matrixColIndices, matrixColumnIndices.data(), (sizeof(IndexType) * matrixNnzCount), cudaMemcpyHostToDevice);
	if (cudaCopyResult != cudaSuccess) {
		std::cout << "Could not copy data for Matrix Column Indices, Error Code " << cudaCopyResult << std::endl;
		goto cleanup;
	}

	CUDA_CHECK_ALL_ERRORS();
	cudaCopyResult = cudaMemcpy(device_matrixValues, matrixValues.data(), (sizeof(ValueType) * matrixNnzCount), cudaMemcpyHostToDevice);
	if (cudaCopyResult != cudaSuccess) {
		std::cout << "Could not copy data for Matrix Values, Error Code " << cudaCopyResult << std::endl;
		goto cleanup;
	}

	CUDA_CHECK_ALL_ERRORS();
	cudaCopyResult = cudaMemcpy(device_x, x.data(), sizeof(ValueType) * matrixColCount, cudaMemcpyHostToDevice);
	if (cudaCopyResult != cudaSuccess) {
		std::cout << "Could not copy data for Vector x, Error Code " << cudaCopyResult << std::endl;
		goto cleanup;
	}

	// Preset the xSwap to zeros...
	CUDA_CHECK_ALL_ERRORS();
	cudaCopyResult = cudaMemset(device_xSwap, 0, sizeof(ValueType) * matrixColCount);
	if (cudaCopyResult != cudaSuccess) {
		std::cout << "Could not zero the Swap Vector x, Error Code " << cudaCopyResult << std::endl;
		goto cleanup;
	}

	CUDA_CHECK_ALL_ERRORS();
	cudaCopyResult = cudaMemcpy(device_b, b.data(), sizeof(ValueType) * matrixRowCount, cudaMemcpyHostToDevice);
	if (cudaCopyResult != cudaSuccess) {
		std::cout << "Could not copy data for Vector b, Error Code " << cudaCopyResult << std::endl;
		goto cleanup;
	}

	// Preset the multiplyResult to zeros...
	CUDA_CHECK_ALL_ERRORS();
	cudaCopyResult = cudaMemset(device_multiplyResult, 0, sizeof(ValueType) * matrixRowCount);
	if (cudaCopyResult != cudaSuccess) {
		std::cout << "Could not zero the multiply Result, Error Code " << cudaCopyResult << std::endl;
		goto cleanup;
	}

	CUDA_CHECK_ALL_ERRORS();
	cudaCopyResult = cudaMemcpy(device_nondeterministicChoiceIndices, nondeterministicChoiceIndices.data(), sizeof(IndexType) * (matrixRowCount + 1), cudaMemcpyHostToDevice);
	if (cudaCopyResult != cudaSuccess) {
		std::cout << "Could not copy data for Vector b, Error Code " << cudaCopyResult << std::endl;
		goto cleanup;
	}

	// Data is on device, start Kernel
	while (!converged && iterationCount < maxIterationCount)
	{ // In a sub-area since transfer of control via label evades initialization
		cusp::detail::device::storm_cuda_opt_spmv_csr_vector<IndexType, ValueType>(matrixRowCount, matrixNnzCount, device_matrixRowIndices, device_matrixColIndices, device_matrixValues, device_x, device_multiplyResult);
		CUDA_CHECK_ALL_ERRORS();

		thrust::device_ptr<ValueType> devicePtrThrust_b(device_b);
		thrust::device_ptr<ValueType> devicePtrThrust_multiplyResult(device_multiplyResult);

		// Transform: Add multiplyResult + b inplace to multiplyResult
		thrust::transform(devicePtrThrust_multiplyResult, devicePtrThrust_multiplyResult + matrixRowCount, devicePtrThrust_b, devicePtrThrust_multiplyResult, thrust::plus<ValueType>());
		CUDA_CHECK_ALL_ERRORS();

		// Reduce: Reduce multiplyResult to a new x vector
		cusp::detail::device::storm_cuda_opt_vector_reduce<Minimize, IndexType, ValueType>(matrixColCount, matrixRowCount, device_nondeterministicChoiceIndices, device_xSwap, device_multiplyResult);
		CUDA_CHECK_ALL_ERRORS();

		// Check for convergence
		// Transform: x = abs(x - xSwap)/ xSwap
		thrust::device_ptr<ValueType> devicePtrThrust_x(device_x);
		thrust::device_ptr<ValueType> devicePtrThrust_x_end(device_x + matrixColCount);
		thrust::device_ptr<ValueType> devicePtrThrust_xSwap(device_xSwap);
		thrust::transform(devicePtrThrust_x, devicePtrThrust_x_end, devicePtrThrust_xSwap, devicePtrThrust_x, equalModuloPrecision<ValueType, Relative>());
		CUDA_CHECK_ALL_ERRORS();

		// Reduce: get Max over x and check for res < Precision
		ValueType maxX = thrust::reduce(devicePtrThrust_x, devicePtrThrust_x_end, -std::numeric_limits<ValueType>::max(), thrust::maximum<ValueType>());
		CUDA_CHECK_ALL_ERRORS();
		converged = (maxX < precision);
		++iterationCount;

		// If there are empty rows in the matrix we need to clear multiplyResult
		thrust::fill(devicePtrThrust_multiplyResult, devicePtrThrust_multiplyResult + matrixRowCount, 0);

		// Swap pointers, device_x always contains the most current result
		std::swap(device_x, device_xSwap);
	}
	std::cout << "(DLL) Executed " << iterationCount << " of max. " << maxIterationCount << " Iterations." << std::endl;

	// Get x back from the device
	cudaCopyResult = cudaMemcpy(x.data(), device_x, sizeof(ValueType) * matrixColCount, cudaMemcpyDeviceToHost);
	if (cudaCopyResult != cudaSuccess) {
		std::cout << "Could not copy back data for result vector x, Error Code " << cudaCopyResult << std::endl;
		goto cleanup;
	}

	// All code related to freeing memory and clearing up the device
cleanup:
	if (device_matrixRowIndices != nullptr) {
		cudaError_t cudaFreeResult = cudaFree(device_matrixRowIndices);
		if (cudaFreeResult != cudaSuccess) {
			std::cout << "Could not free Memory of Matrix Row Indices, Error Code " << cudaFreeResult << "." << std::endl;
		}
		device_matrixRowIndices = nullptr;
	}
	if (device_matrixColIndices != nullptr) {
		cudaError_t cudaFreeResult = cudaFree(device_matrixColIndices);
		if (cudaFreeResult != cudaSuccess) {
			std::cout << "Could not free Memory of Matrix Column Indices, Error Code " << cudaFreeResult << "." << std::endl;
		}
		device_matrixColIndices = nullptr;
	}
	if (device_matrixValues != nullptr) {
		cudaError_t cudaFreeResult = cudaFree(device_matrixValues);
		if (cudaFreeResult != cudaSuccess) {
			std::cout << "Could not free Memory of Matrix Values, Error Code " << cudaFreeResult << "." << std::endl;
		}
		device_matrixValues = nullptr;
	}
	if (device_x != nullptr) {
		cudaError_t cudaFreeResult = cudaFree(device_x);
		if (cudaFreeResult != cudaSuccess) {
			std::cout << "Could not free Memory of Vector x, Error Code " << cudaFreeResult << "." << std::endl;
		}
		device_x = nullptr;
	}
	if (device_xSwap != nullptr) {
		cudaError_t cudaFreeResult = cudaFree(device_xSwap);
		if (cudaFreeResult != cudaSuccess) {
			std::cout << "Could not free Memory of Vector x swap, Error Code " << cudaFreeResult << "." << std::endl;
		}
		device_xSwap = nullptr;
	}
	if (device_b != nullptr) {
		cudaError_t cudaFreeResult = cudaFree(device_b);
		if (cudaFreeResult != cudaSuccess) {
			std::cout << "Could not free Memory of Vector b, Error Code " << cudaFreeResult << "." << std::endl;
		}
		device_b = nullptr;
	}
	if (device_multiplyResult != nullptr) {
		cudaError_t cudaFreeResult = cudaFree(device_multiplyResult);
		if (cudaFreeResult != cudaSuccess) {
			std::cout << "Could not free Memory of Vector multiplyResult, Error Code " << cudaFreeResult << "." << std::endl;
		}
		device_multiplyResult = nullptr;
	}
	if (device_nondeterministicChoiceIndices != nullptr) {
		cudaError_t cudaFreeResult = cudaFree(device_nondeterministicChoiceIndices);
		if (cudaFreeResult != cudaSuccess) {
			std::cout << "Could not free Memory of Nondeterministic Choice Indices, Error Code " << cudaFreeResult << "." << std::endl;
		}
		device_nondeterministicChoiceIndices = nullptr;
	}
}

template <typename IndexType, typename ValueType>
void basicValueIteration_spmv(uint_fast64_t const matrixColCount, std::vector<IndexType> const& matrixRowIndices, std::vector<std::pair<IndexType, ValueType>> const& columnIndicesAndValues, std::vector<ValueType> const& x, std::vector<ValueType>& b) {
	std::vector<IndexType> matrixColumnIndices;
	std::vector<ValueType> matrixValues;
	exploadVector<IndexType, ValueType>(columnIndicesAndValues, matrixColumnIndices, matrixValues);
	
	IndexType* device_matrixRowIndices = nullptr;
	IndexType* device_matrixColIndices = nullptr;
	ValueType* device_matrixValues = nullptr;
	ValueType* device_x = nullptr;
	ValueType* device_multiplyResult = nullptr;

	std::cout.sync_with_stdio(true);
	std::cout << "(DLL) Device has " << getTotalCudaMemory() << " Bytes of Memory with " << getFreeCudaMemory() << "Bytes free (" << (static_cast<double>(getFreeCudaMemory()) / static_cast<double>(getTotalCudaMemory()))*100 << "%)." << std::endl; 
	size_t memSize = sizeof(IndexType) * matrixRowIndices.size() + sizeof(IndexType) * columnIndicesAndValues.size() * 2 + sizeof(ValueType) * x.size() + sizeof(ValueType) * b.size();
	std::cout << "(DLL) We will allocate " << memSize << " Bytes." << std::endl;

	const IndexType matrixRowCount = matrixRowIndices.size() - 1;
	const IndexType matrixNnzCount = columnIndicesAndValues.size();

	cudaError_t cudaMallocResult;

	CUDA_CHECK_ALL_ERRORS();
	cudaMallocResult = cudaMalloc(reinterpret_cast<void**>(&device_matrixRowIndices), sizeof(IndexType) * (matrixRowCount + 1));
	if (cudaMallocResult != cudaSuccess) {
		std::cout << "Could not allocate memory for Matrix Row Indices, Error Code " << cudaMallocResult << "." << std::endl;
		goto cleanup;
	}

	CUDA_CHECK_ALL_ERRORS();
	cudaMallocResult = cudaMalloc(reinterpret_cast<void**>(&device_matrixColIndices), sizeof(IndexType) * matrixNnzCount);
	if (cudaMallocResult != cudaSuccess) {
		std::cout << "Could not allocate memory for Matrix Column Indices, Error Code " << cudaMallocResult << "." << std::endl;
		goto cleanup;
	}

	CUDA_CHECK_ALL_ERRORS();
	cudaMallocResult = cudaMalloc(reinterpret_cast<void**>(&device_matrixValues), sizeof(ValueType) * matrixNnzCount);
	if (cudaMallocResult != cudaSuccess) {
		std::cout << "Could not allocate memory for Matrix Values, Error Code " << cudaMallocResult << "." << std::endl;
		goto cleanup;
	}

	CUDA_CHECK_ALL_ERRORS();
	cudaMallocResult = cudaMalloc(reinterpret_cast<void**>(&device_x), sizeof(ValueType) * matrixColCount);
	if (cudaMallocResult != cudaSuccess) {
		std::cout << "Could not allocate memory for Vector x, Error Code " << cudaMallocResult << "." << std::endl;
		goto cleanup;
	}

	CUDA_CHECK_ALL_ERRORS();
	cudaMallocResult = cudaMalloc(reinterpret_cast<void**>(&device_multiplyResult), sizeof(ValueType) * matrixRowCount);
	if (cudaMallocResult != cudaSuccess) {
		std::cout << "Could not allocate memory for Vector multiplyResult, Error Code " << cudaMallocResult << "." << std::endl;
		goto cleanup;
	}

	// Memory allocated, copy data to device
	cudaError_t cudaCopyResult;

	CUDA_CHECK_ALL_ERRORS();
	cudaCopyResult = cudaMemcpy(device_matrixRowIndices, matrixRowIndices.data(), sizeof(IndexType) * (matrixRowCount + 1), cudaMemcpyHostToDevice);
	if (cudaCopyResult != cudaSuccess) {
		std::cout << "Could not copy data for Matrix Row Indices, Error Code " << cudaCopyResult << std::endl;
		goto cleanup;
	}

	CUDA_CHECK_ALL_ERRORS();
	cudaCopyResult = cudaMemcpy(device_matrixColIndices, matrixColumnIndices.data(), (sizeof(IndexType) * matrixNnzCount), cudaMemcpyHostToDevice);
	if (cudaCopyResult != cudaSuccess) {
		std::cout << "Could not copy data for Matrix Column Indices, Error Code " << cudaCopyResult << std::endl;
		goto cleanup;
	}

	CUDA_CHECK_ALL_ERRORS();
	cudaCopyResult = cudaMemcpy(device_matrixValues, matrixValues.data(), (sizeof(ValueType) * matrixNnzCount), cudaMemcpyHostToDevice);
	if (cudaCopyResult != cudaSuccess) {
		std::cout << "Could not copy data for Matrix Values, Error Code " << cudaCopyResult << std::endl;
		goto cleanup;
	}

	CUDA_CHECK_ALL_ERRORS();
	cudaCopyResult = cudaMemcpy(device_x, x.data(), sizeof(ValueType) * matrixColCount, cudaMemcpyHostToDevice);
	if (cudaCopyResult != cudaSuccess) {
		std::cout << "Could not copy data for Vector x, Error Code " << cudaCopyResult << std::endl;
		goto cleanup;
	}

	// Preset the multiplyResult to zeros...
	CUDA_CHECK_ALL_ERRORS();
	cudaCopyResult = cudaMemset(device_multiplyResult, 0, sizeof(ValueType) * matrixRowCount);
	if (cudaCopyResult != cudaSuccess) {
		std::cout << "Could not zero the multiply Result, Error Code " << cudaCopyResult << std::endl;
		goto cleanup;
	}

	cusp::detail::device::storm_cuda_opt_spmv_csr_vector<IndexType, ValueType>(matrixRowCount, matrixNnzCount, device_matrixRowIndices, device_matrixColIndices, device_matrixValues, device_x, device_multiplyResult);
	CUDA_CHECK_ALL_ERRORS();

	// Get result back from the device
	cudaCopyResult = cudaMemcpy(b.data(), device_multiplyResult, sizeof(ValueType) * matrixRowCount, cudaMemcpyDeviceToHost);
	if (cudaCopyResult != cudaSuccess) {
		std::cout << "Could not copy back data for result vector, Error Code " << cudaCopyResult << std::endl;
		goto cleanup;
	}

	// All code related to freeing memory and clearing up the device
cleanup:
	if (device_matrixRowIndices != nullptr) {
		cudaError_t cudaFreeResult = cudaFree(device_matrixRowIndices);
		if (cudaFreeResult != cudaSuccess) {
			std::cout << "Could not free Memory of Matrix Row Indices, Error Code " << cudaFreeResult << "." << std::endl;
		}
		device_matrixRowIndices = nullptr;
	}
	if (device_matrixColIndices != nullptr) {
		cudaError_t cudaFreeResult = cudaFree(device_matrixColIndices);
		if (cudaFreeResult != cudaSuccess) {
			std::cout << "Could not free Memory of Matrix Column Indices, Error Code " << cudaFreeResult << "." << std::endl;
		}
		device_matrixColIndices = nullptr;
	}
	if (device_matrixValues != nullptr) {
		cudaError_t cudaFreeResult = cudaFree(device_matrixValues);
		if (cudaFreeResult != cudaSuccess) {
			std::cout << "Could not free Memory of Matrix Values, Error Code " << cudaFreeResult << "." << std::endl;
		}
		device_matrixValues = nullptr;
	}
	if (device_x != nullptr) {
		cudaError_t cudaFreeResult = cudaFree(device_x);
		if (cudaFreeResult != cudaSuccess) {
			std::cout << "Could not free Memory of Vector x, Error Code " << cudaFreeResult << "." << std::endl;
		}
		device_x = nullptr;
	}
	if (device_multiplyResult != nullptr) {
		cudaError_t cudaFreeResult = cudaFree(device_multiplyResult);
		if (cudaFreeResult != cudaSuccess) {
			std::cout << "Could not free Memory of Vector multiplyResult, Error Code " << cudaFreeResult << "." << std::endl;
		}
		device_multiplyResult = nullptr;
	}
}

template <typename ValueType>
void basicValueIteration_addVectorsInplace(std::vector<ValueType>& a, std::vector<ValueType> const& b) {
	ValueType* device_a = nullptr;
	ValueType* device_b = nullptr;

	const size_t vectorSize = std::max(a.size(), b.size());

	cudaError_t cudaMallocResult;

	CUDA_CHECK_ALL_ERRORS();
	cudaMallocResult = cudaMalloc(reinterpret_cast<void**>(&device_a), sizeof(ValueType) * vectorSize);
	if (cudaMallocResult != cudaSuccess) {
		std::cout << "Could not allocate memory for Vector a, Error Code " << cudaMallocResult << "." << std::endl;
		goto cleanup;
	}

	CUDA_CHECK_ALL_ERRORS();
	cudaMallocResult = cudaMalloc(reinterpret_cast<void**>(&device_b), sizeof(ValueType) * vectorSize);
	if (cudaMallocResult != cudaSuccess) {
		std::cout << "Could not allocate memory for Vector b, Error Code " << cudaMallocResult << "." << std::endl;
		goto cleanup;
	}

	// Memory allocated, copy data to device
	cudaError_t cudaCopyResult;

	CUDA_CHECK_ALL_ERRORS();
	cudaCopyResult = cudaMemcpy(device_a, a.data(), sizeof(ValueType) * vectorSize, cudaMemcpyHostToDevice);
	if (cudaCopyResult != cudaSuccess) {
		std::cout << "Could not copy data for Vector a, Error Code " << cudaCopyResult << std::endl;
		goto cleanup;
	}

	CUDA_CHECK_ALL_ERRORS();
	cudaCopyResult = cudaMemcpy(device_b, b.data(), sizeof(ValueType) * vectorSize, cudaMemcpyHostToDevice);
	if (cudaCopyResult != cudaSuccess) {
		std::cout << "Could not copy data for Vector b, Error Code " << cudaCopyResult << std::endl;
		goto cleanup;
	}
	
	do {
		// Transform: Add multiplyResult + b inplace to multiplyResult
		thrust::device_ptr<ValueType> devicePtrThrust_a(device_a);
		thrust::device_ptr<ValueType> devicePtrThrust_b(device_b);
		thrust::transform(devicePtrThrust_a, devicePtrThrust_a + vectorSize, devicePtrThrust_b, devicePtrThrust_a, thrust::plus<ValueType>());
		CUDA_CHECK_ALL_ERRORS();
	} while (false);

	// Get result back from the device
	cudaCopyResult = cudaMemcpy(a.data(), device_a, sizeof(ValueType) * vectorSize, cudaMemcpyDeviceToHost);
	if (cudaCopyResult != cudaSuccess) {
		std::cout << "Could not copy back data for result vector, Error Code " << cudaCopyResult << std::endl;
		goto cleanup;
	}

	// All code related to freeing memory and clearing up the device
cleanup:
	if (device_a != nullptr) {
		cudaError_t cudaFreeResult = cudaFree(device_a);
		if (cudaFreeResult != cudaSuccess) {
			std::cout << "Could not free Memory of Vector a, Error Code " << cudaFreeResult << "." << std::endl;
		}
		device_a = nullptr;
	}
	if (device_b != nullptr) {
		cudaError_t cudaFreeResult = cudaFree(device_b);
		if (cudaFreeResult != cudaSuccess) {
			std::cout << "Could not free Memory of Vector b, Error Code " << cudaFreeResult << "." << std::endl;
		}
		device_b = nullptr;
	}
}

template <typename IndexType, typename ValueType, bool Minimize>
void basicValueIteration_reduceGroupedVector(std::vector<ValueType> const& groupedVector, std::vector<IndexType> const& grouping, std::vector<ValueType>& targetVector) {
	ValueType* device_groupedVector = nullptr;
	IndexType* device_grouping = nullptr;
	ValueType* device_target = nullptr;

	const size_t groupedSize = groupedVector.size();
	const size_t groupingSize = grouping.size();
	const size_t targetSize = targetVector.size();

	cudaError_t cudaMallocResult;

	CUDA_CHECK_ALL_ERRORS();
	cudaMallocResult = cudaMalloc(reinterpret_cast<void**>(&device_groupedVector), sizeof(ValueType) * groupedSize);
	if (cudaMallocResult != cudaSuccess) {
		std::cout << "Could not allocate memory for Vector groupedVector, Error Code " << cudaMallocResult << "." << std::endl;
		goto cleanup;
	}

	CUDA_CHECK_ALL_ERRORS();
	cudaMallocResult = cudaMalloc(reinterpret_cast<void**>(&device_grouping), sizeof(IndexType) * groupingSize);
	if (cudaMallocResult != cudaSuccess) {
		std::cout << "Could not allocate memory for Vector grouping, Error Code " << cudaMallocResult << "." << std::endl;
		goto cleanup;
	}

	CUDA_CHECK_ALL_ERRORS();
	cudaMallocResult = cudaMalloc(reinterpret_cast<void**>(&device_target), sizeof(ValueType) * targetSize);
	if (cudaMallocResult != cudaSuccess) {
		std::cout << "Could not allocate memory for Vector targetVector, Error Code " << cudaMallocResult << "." << std::endl;
		goto cleanup;
	}

	// Memory allocated, copy data to device
	cudaError_t cudaCopyResult;

	CUDA_CHECK_ALL_ERRORS();
	cudaCopyResult = cudaMemcpy(device_groupedVector, groupedVector.data(), sizeof(ValueType) * groupedSize, cudaMemcpyHostToDevice);
	if (cudaCopyResult != cudaSuccess) {
		std::cout << "Could not copy data for Vector groupedVector, Error Code " << cudaCopyResult << std::endl;
		goto cleanup;
	}

	CUDA_CHECK_ALL_ERRORS();
	cudaCopyResult = cudaMemcpy(device_grouping, grouping.data(), sizeof(IndexType) * groupingSize, cudaMemcpyHostToDevice);
	if (cudaCopyResult != cudaSuccess) {
		std::cout << "Could not copy data for Vector grouping, Error Code " << cudaCopyResult << std::endl;
		goto cleanup;
	}
	
	do {
		// Reduce: Reduce multiplyResult to a new x vector
		cusp::detail::device::storm_cuda_opt_vector_reduce<Minimize, IndexType, ValueType>(groupingSize - 1, groupedSize, device_grouping, device_target, device_groupedVector);
		CUDA_CHECK_ALL_ERRORS();
	} while (false);

	// Get result back from the device
	cudaCopyResult = cudaMemcpy(targetVector.data(), device_target, sizeof(ValueType) * targetSize, cudaMemcpyDeviceToHost);
	if (cudaCopyResult != cudaSuccess) {
		std::cout << "Could not copy back data for result vector, Error Code " << cudaCopyResult << std::endl;
		goto cleanup;
	}

	// All code related to freeing memory and clearing up the device
cleanup:
	if (device_groupedVector != nullptr) {
		cudaError_t cudaFreeResult = cudaFree(device_groupedVector);
		if (cudaFreeResult != cudaSuccess) {
			std::cout << "Could not free Memory of Vector groupedVector, Error Code " << cudaFreeResult << "." << std::endl;
		}
		device_groupedVector = nullptr;
	}
	if (device_grouping != nullptr) {
		cudaError_t cudaFreeResult = cudaFree(device_grouping);
		if (cudaFreeResult != cudaSuccess) {
			std::cout << "Could not free Memory of Vector grouping, Error Code " << cudaFreeResult << "." << std::endl;
		}
		device_grouping = nullptr;
	}
	if (device_target != nullptr) {
		cudaError_t cudaFreeResult = cudaFree(device_target);
		if (cudaFreeResult != cudaSuccess) {
			std::cout << "Could not free Memory of Vector target, Error Code " << cudaFreeResult << "." << std::endl;
		}
		device_target = nullptr;
	}
}

template <typename ValueType, bool Relative>
void basicValueIteration_equalModuloPrecision(std::vector<ValueType> const& x, std::vector<ValueType> const& y, ValueType& maxElement) {
	ValueType* device_x = nullptr;
	ValueType* device_y = nullptr;

	const size_t vectorSize = x.size();

	cudaError_t cudaMallocResult;

	CUDA_CHECK_ALL_ERRORS();
	cudaMallocResult = cudaMalloc(reinterpret_cast<void**>(&device_x), sizeof(ValueType) * vectorSize);
	if (cudaMallocResult != cudaSuccess) {
		std::cout << "Could not allocate memory for Vector x, Error Code " << cudaMallocResult << "." << std::endl;
		goto cleanup;
	}

	CUDA_CHECK_ALL_ERRORS();
	cudaMallocResult = cudaMalloc(reinterpret_cast<void**>(&device_y), sizeof(ValueType) * vectorSize);
	if (cudaMallocResult != cudaSuccess) {
		std::cout << "Could not allocate memory for Vector y, Error Code " << cudaMallocResult << "." << std::endl;
		goto cleanup;
	}

	// Memory allocated, copy data to device
	cudaError_t cudaCopyResult;

	CUDA_CHECK_ALL_ERRORS();
	cudaCopyResult = cudaMemcpy(device_x, x.data(), sizeof(ValueType) * vectorSize, cudaMemcpyHostToDevice);
	if (cudaCopyResult != cudaSuccess) {
		std::cout << "Could not copy data for Vector x, Error Code " << cudaCopyResult << std::endl;
		goto cleanup;
	}

	CUDA_CHECK_ALL_ERRORS();
	cudaCopyResult = cudaMemcpy(device_y, y.data(), sizeof(ValueType) * vectorSize, cudaMemcpyHostToDevice);
	if (cudaCopyResult != cudaSuccess) {
		std::cout << "Could not copy data for Vector y, Error Code " << cudaCopyResult << std::endl;
		goto cleanup;
	}
	
	do {
		// Transform: x = abs(x - xSwap)/ xSwap
		thrust::device_ptr<ValueType> devicePtrThrust_x(device_x);
		thrust::device_ptr<ValueType> devicePtrThrust_y(device_y);
		thrust::transform(devicePtrThrust_x, devicePtrThrust_x + vectorSize, devicePtrThrust_y, devicePtrThrust_x, equalModuloPrecision<ValueType, Relative>());
		CUDA_CHECK_ALL_ERRORS();

		// Reduce: get Max over x and check for res < Precision
		maxElement = thrust::reduce(devicePtrThrust_x, devicePtrThrust_x + vectorSize, -std::numeric_limits<ValueType>::max(), thrust::maximum<ValueType>());
		CUDA_CHECK_ALL_ERRORS();
	} while (false);

	// All code related to freeing memory and clearing up the device
cleanup:
	if (device_x != nullptr) {
		cudaError_t cudaFreeResult = cudaFree(device_x);
		if (cudaFreeResult != cudaSuccess) {
			std::cout << "Could not free Memory of Vector x, Error Code " << cudaFreeResult << "." << std::endl;
		}
		device_x = nullptr;
	}
	if (device_y != nullptr) {
		cudaError_t cudaFreeResult = cudaFree(device_y);
		if (cudaFreeResult != cudaSuccess) {
			std::cout << "Could not free Memory of Vector y, Error Code " << cudaFreeResult << "." << std::endl;
		}
		device_y = nullptr;
	}
}

/*
 * Declare and implement all exported functions for these Kernels here
 *
 */

void basicValueIteration_spmv_uint64_double(uint_fast64_t const matrixColCount, std::vector<uint_fast64_t> const& matrixRowIndices, std::vector<std::pair<uint_fast64_t, double>> const& columnIndicesAndValues, std::vector<double> const& x, std::vector<double>& b) {
	basicValueIteration_spmv(matrixColCount, matrixRowIndices, columnIndicesAndValues, x, b);
}

void basicValueIteration_addVectorsInplace_double(std::vector<double>& a, std::vector<double> const& b) {
	basicValueIteration_addVectorsInplace<double>(a, b);
}

void basicValueIteration_reduceGroupedVector_uint64_double_minimize(std::vector<double> const& groupedVector, std::vector<uint_fast64_t> const& grouping, std::vector<double>& targetVector) {
	basicValueIteration_reduceGroupedVector<uint_fast64_t, double, true>(groupedVector, grouping, targetVector);
}

void basicValueIteration_reduceGroupedVector_uint64_double_maximize(std::vector<double> const& groupedVector, std::vector<uint_fast64_t> const& grouping, std::vector<double>& targetVector) {
	basicValueIteration_reduceGroupedVector<uint_fast64_t, double, false>(groupedVector, grouping, targetVector);
}

void basicValueIteration_equalModuloPrecision_double_Relative(std::vector<double> const& x, std::vector<double> const& y, double& maxElement) {
	basicValueIteration_equalModuloPrecision<double, true>(x, y, maxElement);
}

void basicValueIteration_equalModuloPrecision_double_NonRelative(std::vector<double> const& x, std::vector<double> const& y, double& maxElement) {
	basicValueIteration_equalModuloPrecision<double, false>(x, y, maxElement);
}

void basicValueIteration_mvReduce_uint64_double_minimize(uint_fast64_t const maxIterationCount, double const precision, bool const relativePrecisionCheck, std::vector<uint_fast64_t> const& matrixRowIndices, std::vector<std::pair<uint_fast64_t, double>> const& columnIndicesAndValues, std::vector<double>& x, std::vector<double> const& b, std::vector<uint_fast64_t> const& nondeterministicChoiceIndices) {
	if (relativePrecisionCheck) {
		basicValueIteration_mvReduce<true, true, uint_fast64_t, double>(maxIterationCount, precision, matrixRowIndices, columnIndicesAndValues, x, b, nondeterministicChoiceIndices);
	} else {
		basicValueIteration_mvReduce<true, false, uint_fast64_t, double>(maxIterationCount, precision, matrixRowIndices, columnIndicesAndValues, x, b, nondeterministicChoiceIndices);
	}
}

void basicValueIteration_mvReduce_uint64_double_maximize(uint_fast64_t const maxIterationCount, double const precision, bool const relativePrecisionCheck, std::vector<uint_fast64_t> const& matrixRowIndices, std::vector<std::pair<uint_fast64_t, double>> const& columnIndicesAndValues, std::vector<double>& x, std::vector<double> const& b, std::vector<uint_fast64_t> const& nondeterministicChoiceIndices) {
	if (relativePrecisionCheck) {
		basicValueIteration_mvReduce<false, true, uint_fast64_t, double>(maxIterationCount, precision, matrixRowIndices, columnIndicesAndValues, x, b, nondeterministicChoiceIndices);
	} else {
		basicValueIteration_mvReduce<false, false, uint_fast64_t, double>(maxIterationCount, precision, matrixRowIndices, columnIndicesAndValues, x, b, nondeterministicChoiceIndices);
	}
}
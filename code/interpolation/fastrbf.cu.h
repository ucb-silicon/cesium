#ifndef __SLIB_INTERPOLATION_FAST_RBF_CU_H__
#define __SLIB_INTERPOLATION_FAST_RBF_CU_H__

#include <Eigen/Dense>

#define CUDA_MALLOC(var, size)						\
  {									\
    const cudaError_t __var##_err__ = cudaMalloc((void **)&var, size);	\
    if (__var##_err__ != cudaSuccess) {					\
      LOG(ERROR) << "Failed to allocate " #var " (error code " << cudaGetErrorString(__var##_err__) << ")"; \
      exit(EXIT_FAILURE);						\
    }									\
  }

#define CUDA_UPLOAD(device_var, host_var, size)				\
  {									\
    const cudaError_t __device_var##_err__ = cudaMemcpy(device_var, host_var, size, cudaMemcpyHostToDevice); \
    if (__device_var##_err__ != cudaSuccess) {				\
      LOG(ERROR) << "Failed to copy " #device_var " to device "		\
	"(error code" << cudaGetErrorString(__device_var##_err__) << ")"; \
      exit(EXIT_FAILURE);						\
    }									\
  }

#define CUDA_DOWNLOAD(host_var, device_var, size)			\
  {									\
    const cudaError_t __device_var##_err__ = cudaMemcpy(host_var, device_var, size, cudaMemcpyDeviceToHost); \
    if (__device_var##_err__ != cudaSuccess) {				\
      LOG(ERROR) << "Failed to copy " #device_var " from device "	\
	"(error code" << cudaGetErrorString(__device_var##_err__) << ")"; \
      exit(EXIT_FAILURE);						\
    }									\
  }

#define CUDA_FREE(var)							\
  {									\
    const cudaError_t __var##_err__ =  cudaFree(var);			\
    if (__var##_err__ != cudaSuccess) {					\
      LOG(ERROR) << "Failed to free " #var " (error code %s)!" << cudaGetErrorString(err); \
      exit(EXIT_FAILURE);						\
    }									\
  }

namespace slib {
  namespace interpolation {
    Eigen::VectorXf CUDAInterpolatePoints(const Eigen::MatrixXf& _points, const Eigen::VectorXf& _w,
					  const float& _alpha, const float& _epsilon2,
					  const Eigen::MatrixXf& _samples);
  }  // namepsace interpolation
}  // namespace slib

#endif  // __SLIB_INTERPOLATION_FAST_RBF_CU_H__

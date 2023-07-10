/*
 * Copyright (c) 2023, NVIDIA CORPORATION.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <cuproj/assert.cuh>

#include <cuda_runtime_api.h>

#include <stdexcept>
#include <string>

namespace cuproj {

/**
 * @addtogroup exception
 * @{
 */

/**---------------------------------------------------------------------------*
 * @brief Exception thrown when logical precondition is violated.
 *
 * This exception should not be thrown directly and is instead thrown by the
 * CUPROJ_EXPECTS macro.
 *
 *---------------------------------------------------------------------------**/
struct logic_error : public std::logic_error {
  logic_error(char const* const message) : std::logic_error(message) {}
  logic_error(std::string const& message) : std::logic_error(message) {}
};

/**
 * @brief Exception thrown when a CUDA error is encountered.
 */
struct cuda_error : public std::runtime_error {
  cuda_error(std::string const& message) : std::runtime_error(message) {}
};

/**
 * @} // end of doxygen group
 */

}  // namespace cuproj

#define CUPROJ_STRINGIFY_DETAIL(x) #x
#define CUPROJ_STRINGIFY(x)        CUPROJ_STRINGIFY_DETAIL(x)

/**---------------------------------------------------------------------------*
 * @brief Macro for checking (pre-)conditions that throws an exception when
 * a condition is violated.
 *
 * Example usage:
 *
 * @code
 * CUPROJ_EXPECTS(lhs->dtype == rhs->dtype, "Column type mismatch");
 * @endcode
 *
 * @param[in] cond Expression that evaluates to true or false
 * @param[in] reason String literal description of the reason that cond is
 * expected to be true
 * @throw cuproj::logic_error if the condition evaluates to false.
 *---------------------------------------------------------------------------**/
#define CUPROJ_EXPECTS(cond, reason)                                       \
  (!!(cond)) ? static_cast<void>(0)                                        \
             : throw cuspatial::logic_error("cuProj failure at: " __FILE__ \
                                            ":" CUPROJ_STRINGIFY(__LINE__) ": " reason)

/**---------------------------------------------------------------------------*
 * @brief Macro for checking (pre-)conditions that throws an exception when
 * a condition is violated.
 *
 * Example usage:
 *
 * @code
 * CUPROJ_HOST_DEVICE_EXPECTS(lhs->dtype == rhs->dtype, "Column type
 *mismatch");
 * @endcode
 *
 * @param[in] cond Expression that evaluates to true or false
 * @param[in] reason String literal description of the reason that cond is
 * expected to be true
 *
 * (if on host)
 * @throw cuproj::logic_error if the condition evaluates to false.
 * (if on device)
 * program terminates and assertion error message is printed to stderr.
 *---------------------------------------------------------------------------**/
#ifndef __CUDA_ARCH__
#define CUPROJ_HOST_DEVICE_EXPECTS(cond, reason) CUPROJ_EXPECTS(cond, reason)
#else
#define CUPROJ_HOST_DEVICE_EXPECTS(cond, reason) cuproj_assert(cond&& reason)
#endif

/**---------------------------------------------------------------------------*
 * @brief Indicates that an erroneous code path has been taken.
 *
 * In host code, throws a `cuproj::logic_error`.
 *
 *
 * Example usage:
 * ```
 * CUPROJ_FAIL("Non-arithmetic operation is not supported");
 * ```
 *
 * @param[in] reason String literal description of the reason
 *---------------------------------------------------------------------------**/
#define CUPROJ_FAIL(reason)                                                      \
  throw cuproj::logic_error("cuProj failure at: " __FILE__ ":" CUPROJ_STRINGIFY( \
    __LINE__) ":"                                                                \
              " " reason)

namespace cuproj {
namespace detail {

inline void throw_cuda_error(cudaError_t error, const char* file, unsigned int line)
{
  throw cuproj::cuda_error(std::string{"CUDA error encountered at: " + std::string{file} + ":" +
                                       std::to_string(line) + ": " + std::to_string(error) + " " +
                                       cudaGetErrorName(error) + " " + cudaGetErrorString(error)});
}

}  // namespace detail
}  // namespace cuproj

/**
 * @brief Error checking macro for CUDA runtime API functions.
 *
 * Invokes a CUDA runtime API function call, if the call does not return
 * cudaSuccess, invokes cudaGetLastError() to clear the error and throws an
 * exception detailing the CUDA error that occurred
 */
#define CUPROJ_CUDA_TRY(call)                                       \
  do {                                                              \
    cudaError_t const status = (call);                              \
    if (cudaSuccess != status) {                                    \
      cudaGetLastError();                                           \
      cuproj::detail::throw_cuda_error(status, __FILE__, __LINE__); \
    }                                                               \
  } while (0);

/**
 * @brief Debug macro to check for CUDA errors
 *
 * In a non-release build, this macro will synchronize the specified stream
 * before error checking. In both release and non-release builds, this macro
 * checks for any pending CUDA errors from previous calls. If an error is
 * reported, an exception is thrown detailing the CUDA error that occurred.
 *
 * The intent of this macro is to provide a mechanism for synchronous and
 * deterministic execution for debugging asynchronous CUDA execution. It should
 * be used after any asynchronous CUDA call, e.g., cudaMemcpyAsync, or an
 * asynchronous kernel launch.
 */
#ifndef NDEBUG
#define CUPROJ_CHECK_CUDA(stream)                   \
  do {                                              \
    CUPROJ_CUDA_TRY(cudaStreamSynchronize(stream)); \
    CUPROJ_CUDA_TRY(cudaPeekAtLastError());         \
  } while (0);
#else
#define CUPROJ_CHECK_CUDA(stream) CUPROJ_CUDA_TRY(cudaPeekAtLastError());
#endif

#define CL_TARGET_OPENCL_VERSION 120
#include <CL/cl.h>

#include "OpenCLCollisionDetector.h"

#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace ms::systems {
    namespace {
        void checkOpenCLError(const cl_int error, const char* operation) {
            if (error != CL_SUCCESS) {
                throw std::runtime_error(
                    std::string(operation) + " failed with OpenCL error " +
                    std::to_string(error));
            }
        }

        std::string loadKernelSource() {
            constexpr auto kernelPath = "assets/shaders/detect_collision.cl";
            std::ifstream file(kernelPath, std::ios::binary);
            if (!file) {
                throw std::runtime_error(
                    "Unable to open OpenCL SAT kernel: " + std::string(kernelPath));
            }
            std::string source(
                std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{});
            if (source.empty()) {
                throw std::runtime_error("OpenCL SAT kernel is empty");
            }
            return source;
        }
    }

    class OpenCLCollisionDetector::Impl {
        cl_context context{};
        cl_command_queue queue{};
        cl_program program{};
        cl_kernel satKernel{};

        void release() {
            if (satKernel) clReleaseKernel(std::exchange(satKernel, nullptr));
            if (program) clReleaseProgram(std::exchange(program, nullptr));
            if (queue) clReleaseCommandQueue(std::exchange(queue, nullptr));
            if (context) clReleaseContext(std::exchange(context, nullptr));
        }

    public:
        Impl() {
            try {
                cl_uint platformCount = 0;
                checkOpenCLError(
                    clGetPlatformIDs(0, nullptr, &platformCount), "clGetPlatformIDs");
                if (platformCount == 0) {
                    return;
                }
                std::vector<cl_platform_id> platforms(platformCount);
                checkOpenCLError(
                    clGetPlatformIDs(platformCount, platforms.data(), nullptr),
                    "clGetPlatformIDs");

                cl_platform_id selectedPlatform{};
                cl_device_id selectedDevice{};
                for (const auto platform : platforms) {
                    cl_uint deviceCount = 0;
                    const auto result =
                        clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 0, nullptr, &deviceCount);
                    if (result == CL_DEVICE_NOT_FOUND || deviceCount == 0) {
                        continue;
                    }
                    checkOpenCLError(result, "clGetDeviceIDs");
                    std::vector<cl_device_id> devices(deviceCount);
                    checkOpenCLError(
                        clGetDeviceIDs(
                            platform, CL_DEVICE_TYPE_GPU, deviceCount, devices.data(), nullptr),
                        "clGetDeviceIDs");
                    selectedPlatform = platform;
                    selectedDevice = devices.front();
                    break;
                }
                if (!selectedDevice) {
                    std::clog << "No OpenCL GPU found; using CPU SAT collision detection\n";
                    return;
                }

                cl_int error = CL_SUCCESS;
                const cl_context_properties properties[] = {
                    CL_CONTEXT_PLATFORM,
                    reinterpret_cast<cl_context_properties>(selectedPlatform),
                    0
                };
                context = clCreateContext(
                    properties, 1, &selectedDevice, nullptr, nullptr, &error);
                checkOpenCLError(error, "clCreateContext");
                queue = clCreateCommandQueue(context, selectedDevice, 0, &error);
                checkOpenCLError(error, "clCreateCommandQueue");

                const auto source = loadKernelSource();
                const auto* sourceData = source.c_str();
                const auto sourceLength = source.size();
                program = clCreateProgramWithSource(
                    context, 1, &sourceData, &sourceLength, &error);
                checkOpenCLError(error, "clCreateProgramWithSource");
                error = clBuildProgram(
                    program, 1, &selectedDevice, nullptr, nullptr, nullptr);
                if (error != CL_SUCCESS) {
                    size_t logSize = 0;
                    clGetProgramBuildInfo(
                        program, selectedDevice, CL_PROGRAM_BUILD_LOG, 0, nullptr, &logSize);
                    std::string buildLog(logSize, '\0');
                    if (logSize > 0) {
                        clGetProgramBuildInfo(
                            program, selectedDevice, CL_PROGRAM_BUILD_LOG,
                            logSize, buildLog.data(), nullptr);
                    }
                    throw std::runtime_error("OpenCL SAT kernel build failed: " + buildLog);
                }
                satKernel = clCreateKernel(program, "detect_sat_triangle_pairs", &error);
                checkOpenCLError(error, "clCreateKernel(SAT)");
                size_t nameSize = 0;
                clGetDeviceInfo(selectedDevice, CL_DEVICE_NAME, 0, nullptr, &nameSize);
                std::string name(nameSize, '\0');
                clGetDeviceInfo(
                    selectedDevice, CL_DEVICE_NAME, nameSize, name.data(), nullptr);
                if (!name.empty() && name.back() == '\0') {
                    name.pop_back();
                }
                std::clog << "OpenCL SAT collision detection using " << name << '\n';
            } catch (const std::exception& error) {
                release();
                std::cerr << "Unable to initialize OpenCL SAT collision detection; "
                             "using CPU SAT collision detection: "
                          << error.what() << '\n';
            }
        }

        ~Impl() {
            release();
        }

        [[nodiscard]] std::optional<std::vector<SatContact>> detect(
            const std::vector<SatTrianglePair>& trianglePairs) {
            if (!satKernel) {
                return std::nullopt;
            }
            if (trianglePairs.empty()) {
                return std::vector<SatContact>{};
            }
            if (trianglePairs.size() > std::numeric_limits<cl_uint>::max()) {
                throw std::runtime_error("Too many triangle pairs for the OpenCL SAT kernel");
            }

            std::vector<std::array<float, 12>> vertices;
            vertices.reserve(trianglePairs.size());
            for (const auto& pair : trianglePairs) {
                std::array<float, 12> packed{};
                for (std::size_t i = 0; i < 3; ++i) {
                    packed[i * 2] = pair.first[i].x;
                    packed[i * 2 + 1] = pair.first[i].y;
                    packed[6 + i * 2] = pair.second[i].x;
                    packed[6 + i * 2 + 1] = pair.second[i].y;
                }
                vertices.push_back(packed);
            }

            cl_int error = CL_SUCCESS;
            const auto vertexBuffer = clCreateBuffer(
                context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                vertices.size() * sizeof(vertices.front()), vertices.data(), &error);
            checkOpenCLError(error, "clCreateBuffer(vertices)");
            const auto contactBuffer = clCreateBuffer(
                context, CL_MEM_WRITE_ONLY,
                trianglePairs.size() * sizeof(std::array<float, 4>), nullptr, &error);
            if (error != CL_SUCCESS) {
                clReleaseMemObject(vertexBuffer);
                checkOpenCLError(error, "clCreateBuffer(contacts)");
            }

            try {
                const auto pairCount = static_cast<cl_uint>(trianglePairs.size());
                checkOpenCLError(
                    clSetKernelArg(satKernel, 0, sizeof(cl_mem), &vertexBuffer),
                    "clSetKernelArg(vertices)");
                checkOpenCLError(
                    clSetKernelArg(satKernel, 1, sizeof(cl_mem), &contactBuffer),
                    "clSetKernelArg(contacts)");
                checkOpenCLError(
                    clSetKernelArg(satKernel, 2, sizeof(pairCount), &pairCount),
                    "clSetKernelArg(pair count)");

                const size_t globalSize = trianglePairs.size();
                checkOpenCLError(
                    clEnqueueNDRangeKernel(
                        queue, satKernel, 1, nullptr, &globalSize, nullptr, 0, nullptr, nullptr),
                    "clEnqueueNDRangeKernel");
                std::vector<std::array<float, 4>> results(trianglePairs.size());
                checkOpenCLError(
                    clEnqueueReadBuffer(
                        queue, contactBuffer, CL_TRUE, 0,
                        results.size() * sizeof(results.front()),
                        results.data(), 0, nullptr, nullptr),
                    "clEnqueueReadBuffer");

                std::vector<SatContact> contacts;
                contacts.reserve(results.size());
                for (const auto& result : results) {
                    contacts.push_back({
                        .intersects = result[3] != 0.f,
                        .normal = {result[0], result[1]},
                        .penetration = result[2]
                    });
                }
                clReleaseMemObject(contactBuffer);
                clReleaseMemObject(vertexBuffer);
                return contacts;
            } catch (...) {
                clReleaseMemObject(contactBuffer);
                clReleaseMemObject(vertexBuffer);
                throw;
            }
        }

        void disable() {
            release();
        }
    };

    OpenCLCollisionDetector::OpenCLCollisionDetector()
        : impl(std::make_unique<Impl>()) {}

    OpenCLCollisionDetector::~OpenCLCollisionDetector() = default;

    std::optional<std::vector<SatContact>> OpenCLCollisionDetector::detect(
        const std::vector<SatTrianglePair>& trianglePairs) {
        try {
            return impl->detect(trianglePairs);
        } catch (const std::exception& error) {
            impl->disable();
            std::cerr << "OpenCL SAT collision detection failed; "
                         "using CPU SAT collision detection: "
                      << error.what() << '\n';
            return std::nullopt;
        }
    }
}

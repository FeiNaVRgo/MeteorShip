#define CL_TARGET_OPENCL_VERSION 120
#include <CL/cl.h>

#include "OpenCLCollisionDetector.h"

#include <algorithm>
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
        cl_kernel triangulationKernel{};

        void release() {
            if (triangulationKernel) {
                clReleaseKernel(std::exchange(triangulationKernel, nullptr));
            }
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
                triangulationKernel =
                    clCreateKernel(program, "triangulate_polygons", &error);
                checkOpenCLError(error, "clCreateKernel(triangulation)");

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

        [[nodiscard]] std::optional<std::vector<std::vector<SatTriangle>>> triangulate(
            const std::vector<std::vector<sf::Vector2f>>& polygons) {
            if (!triangulationKernel) {
                return std::nullopt;
            }
            std::vector<std::vector<SatTriangle>> result(polygons.size());
            if (polygons.empty()) {
                return result;
            }
            if (polygons.size() > std::numeric_limits<cl_uint>::max()) {
                throw std::runtime_error("Too many polygons for the OpenCL triangulation kernel");
            }

            std::vector<float> vertices;
            std::vector<cl_uint> polygonOffsets;
            std::vector<cl_uint> polygonCounts;
            std::vector<cl_uint> triangleOffsets;
            std::size_t triangleCapacity = 0;
            polygonOffsets.reserve(polygons.size());
            polygonCounts.reserve(polygons.size());
            triangleOffsets.reserve(polygons.size());
            for (const auto& polygon : polygons) {
                const auto vertexCount = vertices.size() / 2;
                const auto maxIndexCount =
                    static_cast<std::size_t>(std::numeric_limits<cl_uint>::max());
                if (vertexCount > maxIndexCount ||
                    polygon.size() > maxIndexCount - vertexCount) {
                    throw std::runtime_error("Polygon exceeds OpenCL triangulation index limits");
                }
                polygonOffsets.push_back(static_cast<cl_uint>(vertexCount));
                polygonCounts.push_back(static_cast<cl_uint>(polygon.size()));
                triangleOffsets.push_back(static_cast<cl_uint>(triangleCapacity));
                if (polygon.size() >= 3) {
                    triangleCapacity += polygon.size() - 2;
                    if (triangleCapacity > std::numeric_limits<cl_uint>::max() / 3) {
                        throw std::runtime_error("Triangulation output exceeds OpenCL index limits");
                    }
                }
                for (const auto point : polygon) {
                    vertices.push_back(point.x);
                    vertices.push_back(point.y);
                }
            }

            if (triangleCapacity == 0) {
                return result;
            }

            std::vector<cl_uint> triangleCounts(polygons.size());
            std::vector<cl_uint> statuses(polygons.size());
            std::vector<cl_uint> outputIndices(triangleCapacity * 3);
            std::vector<cl_uint> scratch(vertices.size() / 2);

            cl_mem vertexBuffer{};
            cl_mem polygonOffsetBuffer{};
            cl_mem polygonCountBuffer{};
            cl_mem triangleOffsetBuffer{};
            cl_mem triangleBuffer{};
            cl_mem triangleCountBuffer{};
            cl_mem scratchBuffer{};
            cl_mem statusBuffer{};
            const auto createBuffer = [this](
                const cl_mem_flags flags, const size_t size, const void* data) {
                cl_int error = CL_SUCCESS;
                const auto buffer = clCreateBuffer(
                    context, flags | (data ? CL_MEM_COPY_HOST_PTR : 0),
                    size, const_cast<void*>(data), &error);
                checkOpenCLError(error, "clCreateBuffer(triangulation)");
                return buffer;
            };
            try {
                vertexBuffer = createBuffer(
                    CL_MEM_READ_ONLY, vertices.size() * sizeof(float), vertices.data());
                polygonOffsetBuffer = createBuffer(
                    CL_MEM_READ_ONLY, polygonOffsets.size() * sizeof(cl_uint),
                    polygonOffsets.data());
                polygonCountBuffer = createBuffer(
                    CL_MEM_READ_ONLY, polygonCounts.size() * sizeof(cl_uint),
                    polygonCounts.data());
                triangleOffsetBuffer = createBuffer(
                    CL_MEM_READ_ONLY, triangleOffsets.size() * sizeof(cl_uint),
                    triangleOffsets.data());
                triangleBuffer = createBuffer(
                    CL_MEM_WRITE_ONLY, outputIndices.size() * sizeof(cl_uint), nullptr);
                triangleCountBuffer = createBuffer(
                    CL_MEM_WRITE_ONLY, triangleCounts.size() * sizeof(cl_uint), nullptr);
                scratchBuffer = createBuffer(
                    CL_MEM_READ_WRITE, scratch.size() * sizeof(cl_uint), nullptr);
                statusBuffer = createBuffer(
                    CL_MEM_WRITE_ONLY, statuses.size() * sizeof(cl_uint), nullptr);

                const auto polygonCount = static_cast<cl_uint>(polygons.size());
                const std::array<cl_mem, 8> arguments{
                    vertexBuffer, polygonOffsetBuffer, polygonCountBuffer,
                    triangleOffsetBuffer, triangleBuffer, triangleCountBuffer,
                    scratchBuffer, statusBuffer
                };
                for (cl_uint i = 0; i < arguments.size(); ++i) {
                    checkOpenCLError(
                        clSetKernelArg(
                            triangulationKernel, i, sizeof(cl_mem), &arguments[i]),
                        "clSetKernelArg(triangulation buffer)");
                }
                checkOpenCLError(
                    clSetKernelArg(
                        triangulationKernel, 8, sizeof(polygonCount), &polygonCount),
                    "clSetKernelArg(polygon count)");
                const size_t globalSize = polygons.size();
                checkOpenCLError(
                    clEnqueueNDRangeKernel(
                        queue, triangulationKernel, 1, nullptr, &globalSize,
                        nullptr, 0, nullptr, nullptr),
                    "clEnqueueNDRangeKernel(triangulation)");
                checkOpenCLError(
                    clEnqueueReadBuffer(
                        queue, triangleBuffer, CL_TRUE, 0,
                        outputIndices.size() * sizeof(cl_uint),
                        outputIndices.data(), 0, nullptr, nullptr),
                    "clEnqueueReadBuffer(triangles)");
                checkOpenCLError(
                    clEnqueueReadBuffer(
                        queue, triangleCountBuffer, CL_TRUE, 0,
                        triangleCounts.size() * sizeof(cl_uint),
                        triangleCounts.data(), 0, nullptr, nullptr),
                    "clEnqueueReadBuffer(triangle counts)");
                checkOpenCLError(
                    clEnqueueReadBuffer(
                        queue, statusBuffer, CL_TRUE, 0,
                        statuses.size() * sizeof(cl_uint),
                        statuses.data(), 0, nullptr, nullptr),
                    "clEnqueueReadBuffer(triangulation statuses)");

                for (std::size_t polygonIndex = 0;
                     polygonIndex < polygons.size(); ++polygonIndex) {
                    if (statuses[polygonIndex] == 0) {
                        continue;
                    }
                    const auto outputOffset = triangleOffsets[polygonIndex] * 3;
                    const auto triangleCount = triangleCounts[polygonIndex];
                    const auto maxTriangleCount = polygonCounts[polygonIndex] >= 3
                                                      ? polygonCounts[polygonIndex] - 2
                                                      : 0;
                    if (triangleCount > maxTriangleCount) {
                        throw std::runtime_error("OpenCL triangulation returned an invalid count");
                    }
                    auto& polygonTriangles = result[polygonIndex];
                    polygonTriangles.reserve(triangleCount);
                    for (cl_uint triangleIndex = 0;
                         triangleIndex < triangleCount; ++triangleIndex) {
                        SatTriangle triangle{};
                        for (cl_uint vertexIndex = 0; vertexIndex < 3; ++vertexIndex) {
                            const auto sourceIndex = outputIndices[
                                outputOffset + triangleIndex * 3 + vertexIndex];
                            if (sourceIndex >= vertices.size() / 2) {
                                throw std::runtime_error(
                                    "OpenCL triangulation returned an invalid vertex index");
                            }
                            triangle[vertexIndex] = {
                                vertices[sourceIndex * 2], vertices[sourceIndex * 2 + 1]
                            };
                        }
                        polygonTriangles.push_back(triangle);
                    }
                }
                releaseBuffers({
                    vertexBuffer, polygonOffsetBuffer, polygonCountBuffer,
                    triangleOffsetBuffer, triangleBuffer, triangleCountBuffer,
                    scratchBuffer, statusBuffer
                });
                return result;
            } catch (...) {
                releaseBuffers({
                    vertexBuffer, polygonOffsetBuffer, polygonCountBuffer,
                    triangleOffsetBuffer, triangleBuffer, triangleCountBuffer,
                    scratchBuffer, statusBuffer
                });
                throw;
            }
        }

        static void releaseBuffers(const std::array<cl_mem, 8>& buffers) {
            for (const auto buffer : buffers) {
                if (buffer) {
                    clReleaseMemObject(buffer);
                }
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

    std::optional<std::vector<std::vector<SatTriangle>>>
    OpenCLCollisionDetector::triangulate(
        const std::vector<std::vector<sf::Vector2f>>& polygons) {
        try {
            return impl->triangulate(polygons);
        } catch (const std::exception& error) {
            impl->disable();
            std::cerr << "OpenCL polygon triangulation failed; "
                         "using CPU polygon triangulation: "
                      << error.what() << '\n';
            return std::nullopt;
        }
    }
}

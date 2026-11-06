#define CL_TARGET_OPENCL_VERSION 120
#include <CL/cl.h>

#include "CollisionSystem.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "Components/Rigidbody.h"

namespace ms::systems {
    namespace {
        struct Aabb {
            float left;
            float top;
            float right;
            float bottom;
        };

        static_assert(sizeof(Aabb) == sizeof(float) * 4);

        std::string loadCollisionKernelSource() {
            constexpr auto kernelPath = "assets/shaders/detect_collision.cl";
            std::ifstream file(kernelPath, std::ios::binary);
            if (!file) {
                throw std::runtime_error("Unable to open OpenCL kernel: " +
                                         std::string(kernelPath));
            }

            std::string source(std::istreambuf_iterator<char>{file},
                               std::istreambuf_iterator<char>{});
            if (source.empty()) {
                throw std::runtime_error("OpenCL kernel is empty: " + std::string(kernelPath));
            }
            return source;
        }

        void checkOpenCLError(cl_int error, const char *operation) {
            if (error != CL_SUCCESS) {
                throw std::runtime_error(std::string(operation) + " failed with OpenCL error " +
                                         std::to_string(error));
            }
        }

        bool intersects(const Aabb &a, const Aabb &b) {
            return a.left < b.right && a.right > b.left &&
                   a.top < b.bottom && a.bottom > b.top;
        }

        Aabb toAabb(const sf::FloatRect &bounds) {
            return {
                bounds.position.x,
                bounds.position.y,
                bounds.position.x + bounds.size.x,
                bounds.position.y + bounds.size.y
            };
        }
    }

    class CollisionSystem::OpenCLBackend {
        cl_context context{};
        cl_command_queue queue{};
        cl_program program{};
        cl_kernel kernel{};
        cl_mem boxesBuffer{};
        cl_mem overlapsBuffer{};
        size_t bufferCapacity{};
        std::string deviceName;

        void release() {
            if (overlapsBuffer) {
                clReleaseMemObject(overlapsBuffer);
                overlapsBuffer = nullptr;
            }
            if (boxesBuffer) {
                clReleaseMemObject(boxesBuffer);
                boxesBuffer = nullptr;
            }
            if (kernel) {
                clReleaseKernel(kernel);
                kernel = nullptr;
            }
            if (program) {
                clReleaseProgram(program);
                program = nullptr;
            }
            if (queue) {
                clReleaseCommandQueue(queue);
                queue = nullptr;
            }
            if (context) {
                clReleaseContext(context);
                context = nullptr;
            }
        }

    public:
        OpenCLBackend() {
            try {
                cl_uint platformCount = 0;
                checkOpenCLError(clGetPlatformIDs(0, nullptr, &platformCount), "clGetPlatformIDs");
                if (platformCount == 0) {
                    throw std::runtime_error("No OpenCL platform is installed");
                }

                std::vector<cl_platform_id> platforms(platformCount);
                checkOpenCLError(clGetPlatformIDs(platformCount, platforms.data(), nullptr),
                                 "clGetPlatformIDs");

                cl_device_id device = nullptr;
                cl_platform_id selectedPlatform = nullptr;
                cl_device_id fallbackDevice = nullptr;
                cl_platform_id fallbackPlatform = nullptr;
                for (const auto platform : platforms) {
                    cl_uint deviceCount = 0;
                    const cl_int result = clGetDeviceIDs(
                        platform, CL_DEVICE_TYPE_GPU, 0, nullptr, &deviceCount);
                    if (result == CL_DEVICE_NOT_FOUND || deviceCount == 0) {
                        continue;
                    }
                    checkOpenCLError(result, "clGetDeviceIDs");

                    std::vector<cl_device_id> devices(deviceCount);
                    checkOpenCLError(clGetDeviceIDs(
                                         platform, CL_DEVICE_TYPE_GPU, deviceCount, devices.data(), nullptr),
                                     "clGetDeviceIDs");

                    for (const auto candidate: devices) {
                        size_t vendorSize = 0;
                        checkOpenCLError(clGetDeviceInfo(
                                             candidate, CL_DEVICE_VENDOR, 0, nullptr, &vendorSize),
                                         "clGetDeviceInfo");
                        std::string vendor(vendorSize, '\0');
                        checkOpenCLError(clGetDeviceInfo(
                                             candidate, CL_DEVICE_VENDOR, vendorSize, vendor.data(), nullptr),
                                         "clGetDeviceInfo");
                        if (vendor.find("NVIDIA") != std::string::npos) {
                            device = candidate;
                            selectedPlatform = platform;
                            break;
                        }
                        if (fallbackDevice == nullptr) {
                            fallbackDevice = candidate;
                            fallbackPlatform = platform;
                        }
                    }
                    if (device != nullptr) {
                        break;
                    }
                }
                if (device == nullptr) {
                    device = fallbackDevice;
                    selectedPlatform = fallbackPlatform;
                }
                if (device == nullptr) {
                    throw std::runtime_error("No OpenCL GPU device is available");
                }

                size_t nameSize = 0;
                checkOpenCLError(clGetDeviceInfo(
                                     device, CL_DEVICE_NAME, 0, nullptr, &nameSize), "clGetDeviceInfo");
                deviceName.resize(nameSize);
                checkOpenCLError(clGetDeviceInfo(
                                     device, CL_DEVICE_NAME, nameSize, deviceName.data(), nullptr),
                                 "clGetDeviceInfo");
                if (!deviceName.empty() && deviceName.back() == '\0') {
                    deviceName.pop_back();
                }

                cl_int error = CL_SUCCESS;
                const cl_context_properties properties[] = {
                    CL_CONTEXT_PLATFORM,
                    reinterpret_cast<cl_context_properties>(selectedPlatform),
                    0
                };
                context = clCreateContext(properties, 1, &device, nullptr, nullptr, &error);
                checkOpenCLError(error, "clCreateContext");
                queue = clCreateCommandQueue(context, device, 0, &error);
                checkOpenCLError(error, "clCreateCommandQueue");
                const std::string source = loadCollisionKernelSource();
                const char *sourceData = source.c_str();
                const size_t sourceLength = source.size();
                program = clCreateProgramWithSource(
                    context, 1, &sourceData, &sourceLength, &error);
                checkOpenCLError(error, "clCreateProgramWithSource");
                error = clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr);
                if (error != CL_SUCCESS) {
                    size_t logSize = 0;
                    clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &logSize);
                    std::string buildLog(logSize, '\0');
                    if (logSize > 0) {
                        clGetProgramBuildInfo(
                            program, device, CL_PROGRAM_BUILD_LOG, logSize, buildLog.data(), nullptr);
                    }
                    throw std::runtime_error("OpenCL collision kernel build failed: " + buildLog);
                }
                kernel = clCreateKernel(program, "detect_collisions", &error);
                checkOpenCLError(error, "clCreateKernel");
            } catch (...) {
                release();
                throw;
            }
        }

        ~OpenCLBackend() { release(); }

        [[nodiscard]] const std::string &getDeviceName() const { return deviceName; }

        std::vector<cl_uint> detect(const std::vector<Aabb> &boxes, size_t pairCount) {
            if (pairCount == 0) {
                return {};
            }
            if (pairCount > std::numeric_limits<cl_uint>::max()) {
                throw std::runtime_error("Too many collision pairs for the OpenCL kernel");
            }

            if (pairCount > bufferCapacity) {
                const size_t newCapacity = std::max<size_t>(pairCount, bufferCapacity * 2);
                if (boxesBuffer) clReleaseMemObject(boxesBuffer);
                if (overlapsBuffer) clReleaseMemObject(overlapsBuffer);

                cl_int error = CL_SUCCESS;
                boxesBuffer = clCreateBuffer(
                    context, CL_MEM_READ_ONLY, newCapacity * 2 * sizeof(Aabb), nullptr, &error);
                checkOpenCLError(error, "clCreateBuffer(boxes)");
                overlapsBuffer = clCreateBuffer(
                    context, CL_MEM_WRITE_ONLY, newCapacity * sizeof(cl_uint), nullptr, &error);
                checkOpenCLError(error, "clCreateBuffer(overlaps)");
                bufferCapacity = newCapacity;
            }

            checkOpenCLError(clEnqueueWriteBuffer(
                                 queue, boxesBuffer, CL_TRUE, 0, pairCount * 2 * sizeof(Aabb),
                                 boxes.data(), 0, nullptr, nullptr), "clEnqueueWriteBuffer");

            const cl_uint count = static_cast<cl_uint>(pairCount);
            checkOpenCLError(clSetKernelArg(kernel, 0, sizeof(boxesBuffer), &boxesBuffer),
                             "clSetKernelArg(boxes)");
            checkOpenCLError(clSetKernelArg(kernel, 1, sizeof(overlapsBuffer), &overlapsBuffer),
                             "clSetKernelArg(overlaps)");
            checkOpenCLError(clSetKernelArg(kernel, 2, sizeof(count), &count),
                             "clSetKernelArg(pair count)");

            const size_t globalSize = pairCount;
            checkOpenCLError(clEnqueueNDRangeKernel(
                                 queue, kernel, 1, nullptr, &globalSize, nullptr, 0, nullptr, nullptr),
                             "clEnqueueNDRangeKernel");

            std::vector<cl_uint> overlaps(pairCount);
            checkOpenCLError(clEnqueueReadBuffer(
                                 queue, overlapsBuffer, CL_TRUE, 0, pairCount * sizeof(cl_uint),
                                 overlaps.data(), 0, nullptr, nullptr), "clEnqueueReadBuffer");
            return overlaps;
        }
    };

    CollisionSystem::CollisionSystem(Quadtree &quadtree, EventDispatcher &eventDispatcher)
        : quadtree(quadtree), eventDispatcher(eventDispatcher) {
        try {
            gpuBackend = std::make_unique<OpenCLBackend>();
            std::clog << "OpenCL collision detection using " << gpuBackend->getDeviceName() << '\n';
        } catch (const std::exception &error) {
            gpuFailureReported = true;
            std::cerr << "OpenCL collision detection unavailable; using CPU fallback: "
                    << error.what() << '\n';
        }
    }

    CollisionSystem::~CollisionSystem() = default;

    void CollisionSystem::update(float) {
        std::unordered_map<ecs::Entity, Aabb> entityBounds;
        ecs->view<components::Rigidbody>().each(
            [&entityBounds](ecs::Entity entity, const components::Rigidbody &rigidbody) {
                entityBounds.emplace(entity, toAabb(rigidbody.bounds));
            });

        std::vector<std::pair<ecs::Entity, ecs::Entity> > candidatePairs;
        for (const auto &[entity, bounds]: entityBounds) {
            const sf::FloatRect queryBounds(
                {bounds.left, bounds.top}, {bounds.right - bounds.left, bounds.bottom - bounds.top});
            for (const auto candidate: quadtree.queryPotential(queryBounds)) {
                if (candidate == entity || !entityBounds.contains(candidate)) {
                    continue;
                }
                candidatePairs.emplace_back(
                    std::min(entity, candidate), std::max(entity, candidate));
            }
        }
        std::ranges::sort(candidatePairs);
        candidatePairs.erase(std::unique(candidatePairs.begin(), candidatePairs.end()),
                             candidatePairs.end());

        std::vector<Aabb> boxes;
        boxes.reserve(candidatePairs.size() * 2);
        for (const auto &[first, second]: candidatePairs) {
            boxes.push_back(entityBounds.at(first));
            boxes.push_back(entityBounds.at(second));
        }

        std::vector<cl_uint> overlaps;
        if (gpuBackend) {
            try {
                overlaps = gpuBackend->detect(boxes, candidatePairs.size());
            } catch (const std::exception &error) {
                if (!gpuFailureReported) {
                    std::cerr << "OpenCL collision detection failed; switching to CPU fallback: "
                            << error.what() << '\n';
                    gpuFailureReported = true;
                }
                gpuBackend.reset();
            }
        }

        collisions.clear();
        for (size_t i = 0; i < candidatePairs.size(); ++i) {
            const bool overlapsOnGpu = gpuBackend != nullptr;
            const bool collision = overlapsOnGpu ? overlaps[i] != 0 : intersects(boxes[i * 2], boxes[i * 2 + 1]);
            if (collision) {
                collisions.push_back(candidatePairs[i]);
                eventDispatcher.push(Event(Event::CollisionDetected{
                    candidatePairs[i].first, candidatePairs[i].second
                }));
            }
        }
    }
}

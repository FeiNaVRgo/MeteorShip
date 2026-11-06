#define CL_TARGET_OPENCL_VERSION 120
#include <CL/cl.h>

#include "OpenCLCollisionDetector.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <utility>

namespace ms::systems {
    namespace {
        void checkOpenCLError(cl_int error, const char *operation) {
            if (error != CL_SUCCESS) {
                throw std::runtime_error(std::string(operation) + " failed with OpenCL error " +
                                         std::to_string(error));
            }
        }

        std::string getDeviceString(cl_device_id device, cl_device_info parameter) {
            size_t size = 0;
            checkOpenCLError(clGetDeviceInfo(device, parameter, 0, nullptr, &size),
                             "clGetDeviceInfo");
            std::string value(size, '\0');
            checkOpenCLError(clGetDeviceInfo(device, parameter, size, value.data(), nullptr),
                             "clGetDeviceInfo");
            if (!value.empty() && value.back() == '\0') {
                value.pop_back();
            }
            return value;
        }

        template<typename T>
        T getDeviceValue(cl_device_id device, cl_device_info parameter) {
            T value{};
            checkOpenCLError(clGetDeviceInfo(device, parameter, sizeof(value), &value, nullptr),
                             "clGetDeviceInfo");
            return value;
        }

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
    }

    class OpenCLCollisionDetector::Impl {
    public:
        struct Device {
            cl_platform_id platform{};
            cl_device_id device{};
            std::string name;
            std::string vendor;
            cl_ulong globalMemory{};
            cl_uint computeUnits{};
            cl_uint clockFrequency{};
            cl_bool unifiedMemory{};
        };

        class Backend {
            cl_context context{};
            cl_command_queue queue{};
            cl_program program{};
            cl_kernel kernel{};
            cl_mem boxesBuffer{};
            cl_mem overlapsBuffer{};
            size_t bufferCapacity{};

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
            explicit Backend(const Device &device) {
                try {
                    cl_int error = CL_SUCCESS;
                    const cl_context_properties properties[] = {
                        CL_CONTEXT_PLATFORM,
                        reinterpret_cast<cl_context_properties>(device.platform),
                        0
                    };
                    context = clCreateContext(properties, 1, &device.device, nullptr, nullptr, &error);
                    checkOpenCLError(error, "clCreateContext");
                    queue = clCreateCommandQueue(context, device.device, 0, &error);
                    checkOpenCLError(error, "clCreateCommandQueue");

                    const std::string source = loadCollisionKernelSource();
                    const char *sourceData = source.c_str();
                    const size_t sourceLength = source.size();
                    program = clCreateProgramWithSource(
                        context, 1, &sourceData, &sourceLength, &error);
                    checkOpenCLError(error, "clCreateProgramWithSource");
                    error = clBuildProgram(program, 1, &device.device, nullptr, nullptr, nullptr);
                    if (error != CL_SUCCESS) {
                        size_t logSize = 0;
                        clGetProgramBuildInfo(
                            program, device.device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &logSize);
                        std::string buildLog(logSize, '\0');
                        if (logSize > 0) {
                            clGetProgramBuildInfo(
                                program, device.device, CL_PROGRAM_BUILD_LOG,
                                logSize, buildLog.data(), nullptr);
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

            ~Backend() { release(); }

            std::vector<std::uint8_t> detect(
                const std::vector<std::array<float, 4> > &boxes, size_t pairCount) {
                if (pairCount == 0) {
                    return {};
                }
                if (pairCount > std::numeric_limits<cl_uint>::max()) {
                    throw std::runtime_error("Too many collision pairs for the OpenCL kernel");
                }

                if (pairCount > bufferCapacity) {
                    const size_t newCapacity = std::max<size_t>(pairCount, bufferCapacity * 2);
                    if (boxesBuffer) {
                        clReleaseMemObject(boxesBuffer);
                        boxesBuffer = nullptr;
                    }
                    if (overlapsBuffer) {
                        clReleaseMemObject(overlapsBuffer);
                        overlapsBuffer = nullptr;
                    }

                    cl_int error = CL_SUCCESS;
                    boxesBuffer = clCreateBuffer(
                        context, CL_MEM_READ_ONLY, newCapacity * 2 * sizeof(boxes[0]),
                        nullptr, &error);
                    checkOpenCLError(error, "clCreateBuffer(boxes)");
                    overlapsBuffer = clCreateBuffer(
                        context, CL_MEM_WRITE_ONLY, newCapacity * sizeof(cl_uint), nullptr, &error);
                    checkOpenCLError(error, "clCreateBuffer(overlaps)");
                    bufferCapacity = newCapacity;
                }

                checkOpenCLError(clEnqueueWriteBuffer(
                                     queue, boxesBuffer, CL_TRUE, 0, pairCount * 2 * sizeof(boxes[0]),
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

                std::vector<cl_uint> results(pairCount);
                checkOpenCLError(clEnqueueReadBuffer(
                                     queue, overlapsBuffer, CL_TRUE, 0, pairCount * sizeof(cl_uint),
                                     results.data(), 0, nullptr, nullptr), "clEnqueueReadBuffer");

                std::vector<std::uint8_t> overlaps;
                overlaps.reserve(pairCount);
                for (const auto result: results) {
                    overlaps.push_back(static_cast<std::uint8_t>(result));
                }
                return overlaps;
            }
        };

        std::vector<Device> devices;
        std::unique_ptr<Backend> backend;
        int failedOption{-1};

        Impl() {
            try {
                cl_uint platformCount = 0;
                checkOpenCLError(clGetPlatformIDs(0, nullptr, &platformCount), "clGetPlatformIDs");
                if (platformCount == 0) {
                    return;
                }
                std::vector<cl_platform_id> platforms(platformCount);
                checkOpenCLError(clGetPlatformIDs(
                                     platformCount, platforms.data(), nullptr), "clGetPlatformIDs");

                for (const auto platform: platforms) {
                    cl_uint deviceCount = 0;
                    const cl_int result = clGetDeviceIDs(
                        platform, CL_DEVICE_TYPE_GPU, 0, nullptr, &deviceCount);
                    if (result == CL_DEVICE_NOT_FOUND || deviceCount == 0) {
                        continue;
                    }
                    checkOpenCLError(result, "clGetDeviceIDs");

                    std::vector<cl_device_id> platformDevices(deviceCount);
                    checkOpenCLError(clGetDeviceIDs(
                                         platform, CL_DEVICE_TYPE_GPU, deviceCount,
                                         platformDevices.data(), nullptr), "clGetDeviceIDs");
                    for (const auto device: platformDevices) {
                        devices.push_back({
                            .platform = platform,
                            .device = device,
                            .name = getDeviceString(device, CL_DEVICE_NAME),
                            .vendor = getDeviceString(device, CL_DEVICE_VENDOR),
                            .globalMemory = getDeviceValue<cl_ulong>(
                                device, CL_DEVICE_GLOBAL_MEM_SIZE),
                            .computeUnits = getDeviceValue<cl_uint>(
                                device, CL_DEVICE_MAX_COMPUTE_UNITS),
                            .clockFrequency = getDeviceValue<cl_uint>(
                                device, CL_DEVICE_MAX_CLOCK_FREQUENCY),
                            .unifiedMemory = getDeviceValue<cl_bool>(
                                device, CL_DEVICE_HOST_UNIFIED_MEMORY)
                        });
                    }
                }
            } catch (const std::exception &error) {
                devices.clear();
                std::cerr << "OpenCL GPU discovery failed; using CPU collision detection: "
                        << error.what() << '\n';
            }
        }

        [[nodiscard]] size_t getBestDevice() const {
            size_t best = 0;
            for (size_t i = 1; i < devices.size(); ++i) {
                const auto &candidate = devices[i];
                const auto &current = devices[best];
                if (candidate.unifiedMemory != current.unifiedMemory) {
                    if (!candidate.unifiedMemory) {
                        best = i;
                    }
                    continue;
                }
                if (candidate.globalMemory != current.globalMemory) {
                    if (candidate.globalMemory > current.globalMemory) {
                        best = i;
                    }
                    continue;
                }
                if (candidate.computeUnits != current.computeUnits) {
                    if (candidate.computeUnits > current.computeUnits) {
                        best = i;
                    }
                    continue;
                }
                if (candidate.clockFrequency > current.clockFrequency) {
                    best = i;
                }
            }
            return best;
        }

        std::optional<std::vector<std::uint8_t> > detect(
            int option, const std::vector<std::array<float, 4> > &boxes, size_t pairCount) {
            if (option == 1 || devices.empty() || failedOption == option) {
                return std::nullopt;
            }

            const size_t deviceIndex = option == 0
                                           ? getBestDevice()
                                           : static_cast<size_t>(option - 2);
            if (deviceIndex >= devices.size()) {
                return std::nullopt;
            }

            try {
                if (!backend) {
                    backend = std::make_unique<Backend>(devices[deviceIndex]);
                    std::clog << "OpenCL collision detection using "
                            << devices[deviceIndex].vendor << " "
                            << devices[deviceIndex].name << '\n';
                }
                return backend->detect(boxes, pairCount);
            } catch (const std::exception &error) {
                backend.reset();
                failedOption = option;
                std::cerr << "OpenCL collision detection failed; using CPU fallback: "
                        << error.what() << '\n';
                return std::nullopt;
            }
        }
    };

    OpenCLCollisionDetector::OpenCLCollisionDetector()
        : impl(std::make_unique<Impl>()),
          options{"Auto (best GPU)", "CPU"} {
        for (const auto &device: impl->devices) {
            options.push_back(device.vendor + " - " + device.name);
        }
    }

    OpenCLCollisionDetector::~OpenCLCollisionDetector() = default;

    void OpenCLCollisionDetector::selectOption(const int option) {
        if (option < 0 || static_cast<size_t>(option) >= options.size()) {
            throw std::out_of_range("OpenCL device option is out of range");
        }
        if (selectedOption == option) {
            return;
        }
        selectedOption = option;
        impl->backend.reset();
        impl->failedOption = -1;
    }

    std::optional<std::vector<std::uint8_t> > OpenCLCollisionDetector::detect(
        const std::vector<std::array<float, 4> > &boxes, const std::size_t pairCount) {
        return impl->detect(selectedOption, boxes, pairCount);
    }
}

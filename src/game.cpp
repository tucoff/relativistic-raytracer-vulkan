#pragma region Includes

#include "game.h"
#include "utils.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <chrono>
#include <array>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <optional>
#include <set>
#include <cstdint>
#include <limits>
#include <algorithm>
#include <thread>
#include <iomanip>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#include <filesystem> 
namespace fs = std::filesystem;

#pragma endregion

#pragma region Constants and Global Variables

const char* APP_NAME = "RayTracing Vulkan Application";
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 800;

const int MAX_FRAMES_IN_FLIGHT = 4;

// Resolution presets
struct Resolution {
    uint32_t width;
    uint32_t height;
    const char* name;
};

const std::vector<Resolution> RESOLUTION_PRESETS = {
    { 256, 144, "144p" },
    { 853, 480, "480p" },
    { 1280, 720, "720p" },
    { 1920, 1080, "1080p" }
};

const float CAMERA_SPEED = 40;
const float MOUSE_SENSITIVITY = 0.05f;

bool relativisticViewEnabled = true;
bool traceMode = false;

const std::vector<const char*> validationLayers =
{
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions =
{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

#pragma endregion

#pragma region Structs

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete()
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities = {};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct CameraUBO {
    alignas(16) glm::vec3 origin;
    alignas(16) glm::vec3 lower_left_corner;
    alignas(16) glm::vec3 horizontal;
    alignas(16) glm::vec3 vertical;
	alignas(4) bool relativistic_view_enabled;
    alignas(4) bool method_euler;
    alignas(4) float step_size;
    alignas(4) int max_steps;
    alignas(4) int metric;
    alignas(4) float spin_speed;
    alignas(4) int current_scene;
    alignas(4) float gravity_multiplier; 
};

#pragma endregion

class Game;

struct CameraPreset {
    std::string name;
    glm::vec3 position;
    float pitch;
    float yaw;
};

struct StepSizePreset {
    std::string name;
    float value;
};

struct GravityPreset {
    std::string name;
    float value;
};

struct SpinSpeedPreset {
    std::string name;
    float value;
};

class BenchmarkAutomator {
public:
    enum class Phase {
        Baseline,
        Full
    };

    Phase currentPhase = Phase::Baseline;
    
    int resIndex = 0;
    int metricIndex = 0;
    int integratorIndex = 0;
    int sceneIndex = 1;
     
    int camIndex = 0;
    int stepIndex = 0;
    int gravIndex = 0;
    int spinIndex = 0;
    int baselineSceneIndex = 1;

    float timer = 0.0f;
    float warmUpTimer = 0.0f;
    int frameCount = 0;
    bool isFinished = false;
    bool baselineFinished = false;
    bool configApplied = false;
    
    const float WARM_UP_DURATION = 1.0f;
     
    const float benchmarkDurationPerConfig = 3.0f;
     
    std::vector<StepSizePreset> stepPresets = {
        {"SmallStep", 1.0f}, {"MediumStep", 52.0f}, {"BigStep", 260.0f}
    };
    std::vector<GravityPreset> gravityPresets = {
        {"NormalG", 1.989e31f}, {"StrongG", 7.989e31f}, {"MuchStrongerG", 1.989e32f}
    };
    std::vector<GravityPreset> customScene6Gravities = {
        {"Grav_1.989e31", 1.989e+31f}, {"Grav_1.989e32", 1.989e+32f},
        {"Grav_1.989e33", 1.989e+33f}, {"Grav_1.989e34", 1.989e+34f}
    };
    std::vector<SpinSpeedPreset> spinPresets = {
        {"NaturalSpinSpeed", 0.5f}, {"UnaturalSpinSpeed", 10.0f},
        {"StrongSpinSpeed", 50.0f}, {"FunnySpinSpeed", 200.0f}
    };
     
    std::vector<CameraPreset> defaultCameras = {
        {"Front",     glm::vec3(0.0f, 0.0f, 666.0f),     0.0f,  -90.0f}, 
        {"Periferic", glm::vec3(-140.0f, 0.0f, -360.0f), 0.0f,   24.0f}, 
        {"Above",     glm::vec3(0.0f, 180.0f, -150.0f),  -90.0f,  -90.0f}, 
        {"Diagonal",  glm::vec3(100.0f, 70.0f, 400.0f),-15.0f,  -105.0f}, 
        {"Tangent",   glm::vec3(0.0f, 0.0f, -180.0f),    0.0f,    0.0f}, 
        {"LookAway",  glm::vec3(0.0f, 0.0f, -200.0f),    0.0f,  -90.0f} 
    };

    std::vector<CameraPreset> scene6Cameras = {
        {"CamPos1", glm::vec3(83.0f, -4.0f, 30.0f),       0.0f,  -150.0f},
        {"CamPos2", glm::vec3(100.0f, 0.0f, 235.0f),      0.0f,  -90.0f},
        {"CamPos3", glm::vec3(300.0f, 0.0f, 800.0f),      0.0f,  -26.0f}, 
        {"CamPos4", glm::vec3(150.0f, -200.0f, -100.0f), 60.0f,  110.0f}, 
        {"CamPos5", glm::vec3(125.0f, -2.0f, -30.0f),    -1.0f,   65.0f}, 
        {"CamPos6", glm::vec3(100.0f, 2.2f, 45.0f),       0.3f,    0.0f} 
    };

    void update(float deltaTime, Game* game);
    void applyConfig(Game* game);

private:
    void advance(Game* game);
    void advanceBaseline(Game* game);
    void saveBenchmark(float fps, Game* game);
    void saveBaselineScreenshot(float fps, Game* game);
    void saveToCSV(float fps, const std::string& imagePath);
    void saveBaselineToCSV(float fps, const std::string& imagePath);
};

class Game
{   

public:
    glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 666.0f);
    glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    float yaw = -90.0f;
    float pitch = 0.0f;

    void run()
    {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
#pragma region Private Member Variables

    GLFWwindow* window = nullptr;

    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkPhysicalDeviceFeatures supportedPhysicalDeviceFeatures = {};
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    std::vector<VkImage> swapChainImages = {};
    VkFormat swapChainImageFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D swapChainExtent = {};
    std::vector<VkImageView> swapChainImageViews = {};
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkImage textureImage = VK_NULL_HANDLE;
    VkDeviceMemory textureImageMemory = VK_NULL_HANDLE;
    VkImageView textureImageView = VK_NULL_HANDLE;
    VkSampler textureSampler = VK_NULL_HANDLE; 
    
    VkImage milkyWayImage = VK_NULL_HANDLE;
    VkDeviceMemory milkyWayImageMemory = VK_NULL_HANDLE;
    VkImageView milkyWayImageView = VK_NULL_HANDLE;
    
    std::array<VkImage, 10> planetImages = {};
    std::array<VkDeviceMemory, 10> planetImageMemories = {};
    std::array<VkImageView, 10> planetImageViews = {};
    
    VkImage storageImage = VK_NULL_HANDLE;
    VkDeviceMemory storageImageMemory = VK_NULL_HANDLE;
    VkImageView storageImageView = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE; 
    std::vector<VkDescriptorSet> descriptorSets;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    size_t currentFrame = 0;
    bool framebufferResized = false;
    uint32_t mipLevels = 1; 
    std::vector<VkBuffer> cameraUniformBuffers;
    std::vector<VkDeviceMemory> cameraUniformBuffersMemory;
    void* cameraBuffersMapped[MAX_FRAMES_IN_FLIGHT]; 
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    float cameraSpeed = 2.5f;
    bool firstMouse = true;
    float hX = WIDTH / 2.0f;
    float hY = HEIGHT / 2.0f;
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;
    bool mouseControlEnabled = false;
    bool ignoreNextMouseMove = false;
    public:bool method = true;
    float stepSize = 10.0f;
    int maxSteps = 1000;
    public:int metric = 0; // 0 - Newton || 1 - Schwarzschild || 2 - Kerr
    float spinSpeed = 0.5f;
    int currentScene = 0;
    bool isInputActive = false;
    bool showFPS = false;
    float fpsUpdateTimer = 0.0f;
    float currentFPS = 0.0f;
    int frameCount = 0;
    uint32_t currentResolutionIndex = 0;
    uint32_t currentWindowWidth = WIDTH;
    uint32_t currentWindowHeight = HEIGHT;
    float gravityMultiplier = 1.0f;  // Used for Scene 6 Solar System

#pragma endregion

#pragma region InitWindow()
    void initWindow()
    {
        #if defined(_WIN32) || defined(_WIN64)
        #elif defined(__linux__)
            glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
        #endif

        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WIDTH, HEIGHT, APP_NAME, nullptr, nullptr);

        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
        glfwSetCursorPosCallback(window, mouseCallback);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        (void)width;   // Suppress unused parameter warning
        (void)height;  // Suppress unused parameter warning
        auto app = reinterpret_cast<Game*>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }

    static void mouseCallback(GLFWwindow* window, double xpos, double ypos) 
    {
        auto app = reinterpret_cast<Game*>(glfwGetWindowUserPointer(window));
        app->processMouseMovement(static_cast<float>(xpos), static_cast<float>(ypos));
    }

#pragma endregion

#pragma region InitVulkan()

    void initVulkan()
    {
        createInstance();
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createCommandPool();
        createUniformBuffers();
        createTextureImage();
        createMilkyWayTexture();
        createPlanetTextures();
        createTextureImageView();
        createTextureSampler();
        createStorageImage();
        createDescriptorSetLayout();
        createDescriptorPool();
        createDescriptorSets();
        createPipeline();
        createCommandBuffers();
        createSyncObjects();
    }

    #pragma region Other Helper Functions
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice candidateDevice)
    {
        QueueFamilyIndices indices;
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(candidateDevice, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(candidateDevice, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies)
        {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                indices.graphicsFamily = i;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(candidateDevice, i, surface, &presentSupport);

            if (presentSupport)
            {
                indices.presentFamily = i;
            }

            if (indices.isComplete())
            {
                break;
            }

            i++;
        }

        return indices;
    }

    VkFormat findDepthFormat()
    {
        return findSupportedFormat(
            { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }

    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
    {
        for (VkFormat format : candidates)
        {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
            {
                return format;
            }
            else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
            {
                return format;
            }
        }

        throw std::runtime_error("failed to find supported format!");
    }

    void createImage(uint32_t texWidth, uint32_t texHeight, uint32_t levels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = format;
        imageInfo.extent.width = texWidth;
        imageInfo.extent.height = texHeight;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = levels;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = numSamples;
        imageInfo.tiling = tiling;
        imageInfo.usage = usage;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create texture image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryTypeIndex(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate texture image memory!");
        }

        vkBindImageMemory(device, image, imageMemory, 0);
    }

    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t levels)
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        viewInfo.subresourceRange.levelCount = levels;

        VkImageView imageView;
        if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create image view!");
        }

        return imageView;
    }

    uint32_t findMemoryTypeIndex(uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
    {
        VkCommandBuffer commandBuffer = beginSingleTimeCommand();
        
        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = size;

        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        endSingleTimeCommand(commandBuffer);
    }

    #pragma endregion

    #pragma region CreateInstance()
    void createInstance()
    {
        if (enableValidationLayers && !checkValidationLayerSupport())
        {
            throw std::runtime_error("validation layers requested, but not available!");
        }

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = APP_NAME;
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "74 Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_4;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        auto extensions = getRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
        }
        else
        {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create instance!");
        }
    }

    bool checkValidationLayerSupport()
    {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : validationLayers)
        {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers)
            {
                if (strcmp(layerName, layerProperties.layerName) == 0)
                {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound)
            {
                return false;
            }
        }

        return true;
    }

    std::vector<const char*> getRequiredExtensions()
    {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (enableValidationLayers)
        {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }
    
    #pragma endregion

    #pragma region SetupDebugMessenger()
    void setupDebugMessenger()
    {
        if (!enableValidationLayers) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);

        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    }

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
    {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
         
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
    {
        (void)messageSeverity;  
        (void)messageType;      
        (void)pUserData;        
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }

    #pragma endregion

    #pragma region CreateSurface()
    void createSurface()
    {
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create window surface!");
        }
    }

    #pragma endregion

    #pragma region PickPhysicalDevice()
    void pickPhysicalDevice()
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        if (deviceCount == 0)
        {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        for (const auto &device : devices) {
            VkPhysicalDeviceFeatures currentDeviceFeatures;
            vkGetPhysicalDeviceFeatures(device, &currentDeviceFeatures);

            if (isDeviceSuitable(device)) {
                physicalDevice = device;
                vkGetPhysicalDeviceFeatures(device, &supportedPhysicalDeviceFeatures);
                break;
            }
        }

        if (physicalDevice == VK_NULL_HANDLE)
        {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
    }

    bool isDeviceSuitable(VkPhysicalDevice device)
    {
        QueueFamilyIndices indices = findQueueFamilies(device);
        bool extensionsSupported = checkDeviceExtensionSupport(device);

        bool swapChainAdequate = false;
        if (extensionsSupported)
        {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        return indices.isComplete() && extensionsSupported && swapChainAdequate && deviceFeatures.samplerAnisotropy;
    }

    bool checkDeviceExtensionSupport(VkPhysicalDevice device)
    {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
        if (extensionCount == 0) return false;
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
        for (const auto& extension : availableExtensions)
        {
            requiredExtensions.erase(extension.extensionName);
        }
        return requiredExtensions.empty();
    }

    #pragma endregion

    #pragma region CreateLogicalDevice()
    void createLogicalDevice()
    {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        } 

        VkPhysicalDeviceFeatures enabledFeatures = {};
        enabledFeatures.samplerAnisotropy = VK_TRUE;

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();

        createInfo.pEnabledFeatures = &enabledFeatures;
         
        createInfo.pNext = nullptr;

        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());;
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        if (enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else
        {
            createInfo.enabledLayerCount = 0;
        }

        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create logical device!");
        }

        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
    }

    #pragma endregion

    #pragma region CreateSwapChain()
    void createSwapChain()
    {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
        {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

        if (indices.graphicsFamily != indices.presentFamily)
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = chooseSwapCompositeAlpha(swapChainSupport.capabilities);
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create swap chain!");
        }

        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
         
        imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);
    }

    VkCompositeAlphaFlagBitsKHR chooseSwapCompositeAlpha(const VkSurfaceCapabilitiesKHR& capabilities) 
    {
        if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR) 
        {
            return VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
        }

        if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR) 
        {
            return VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
        }
        
        if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) 
        {
            return VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        }

        throw std::runtime_error("failed to find a supported composite alpha!");
    }

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device)
    {
        SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

        if (formatCount != 0)
        {
            details.formats.resize(formatCount);

            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount;

        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        if (presentModeCount != 0)
        {
            details.presentModes.resize(presentModeCount);

            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
    {
        for (const auto& availableFormat : availableFormats)
        {
            if (availableFormat.format == VK_FORMAT_R8G8B8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
    { 
        for (const auto& availablePresentMode : availablePresentModes)
        {
            if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
            {
                return availablePresentMode;
            }
        } 
        for (const auto& availablePresentMode : availablePresentModes)
        {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return availablePresentMode;
            }
        }
        return VK_PRESENT_MODE_FIFO_KHR; 
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
    {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            return capabilities.currentExtent;
        }
        else
        {
            int width, height;

            glfwGetFramebufferSize(window, &width, &height);

            VkExtent2D actualExtent =
            {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

    #pragma endregion

    #pragma region CreateImageViews()
    void createImageViews()
    {
        swapChainImageViews.resize(swapChainImages.size());
        for (size_t i = 0; i < swapChainImages.size(); i++)
        {
            swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        }
    }

    #pragma endregion

    #pragma region CreateCommandPool()
    void createCommandPool()
    {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create command pool!");
        }
    }
    
    #pragma endregion

    #pragma region CreateTextureImage()
    void createTextureImage()
    { 
        std::vector<std::string> skyboxFaces = {
            "textures/Left.png",    // +X
            "textures/Right.png",   // -X
            "textures/Up.png",      // +Y
            "textures/Down.png",    // -Y
            "textures/Front.png",   // +Z
            "textures/Back.png"     // -Z
        };

        int texWidth, texHeight, texChannels;
        stbi_uc* dummy = stbi_load(skyboxFaces[0].c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        
        if (!dummy)
        {
            throw std::runtime_error("failed to load cubemap faces!");
        }
        stbi_image_free(dummy);

        VkDeviceSize layerSize = texWidth * texHeight * 4;
        VkDeviceSize imageSize = layerSize * 6;

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);

        for (int i = 0; i < 6; i++)
        {
            stbi_uc* pixels = stbi_load(skyboxFaces[i].c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
            if (!pixels)
            {
                throw std::runtime_error("failed to load cubemap face: " + skyboxFaces[i]);
            }
            memcpy(static_cast<stbi_uc*>(data) + (layerSize * i), pixels, static_cast<size_t>(layerSize));
            stbi_image_free(pixels);
        }
        vkUnmapMemory(device, stagingBufferMemory);

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        imageInfo.extent.width = texWidth;
        imageInfo.extent.height = texHeight;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 6;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

        if (vkCreateImage(device, &imageInfo, nullptr, &textureImage) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create cubemap image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, textureImage, &memRequirements);
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryTypeIndex(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        
        if (vkAllocateMemory(device, &allocInfo, nullptr, &textureImageMemory) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate cubemap memory!");
        }
        vkBindImageMemory(device, textureImage, textureImageMemory, 0);

        transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, 6);
        copyBufferToImageCubemap(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
        transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, 6);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
        mipLevels = 1;
    }

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& vertexBuffer, VkDeviceMemory& vertexBufferMemory)
    {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &bufferInfo, nullptr, &vertexBuffer) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create vertex buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, vertexBuffer, &memRequirements);
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryTypeIndex(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &vertexBufferMemory) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate vertex buffer memory!");
        }

        vkBindBufferMemory(device, vertexBuffer, vertexBufferMemory, 0);
    }

    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, uint32_t layerCount = 1)
    {
        (void)format;  
        VkCommandBuffer commandBuffer = beginSingleTimeCommand();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.levelCount = mipLevels;

        if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        
            if (hasStencilComponent(format))
            {
                barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        }
        else
        {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        }

        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = mipLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = layerCount;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_GENERAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        }
        else
        {
            throw std::invalid_argument("unsupported layout transition!");
        }

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        endSingleTimeCommand(commandBuffer);
    }

    bool hasStencilComponent(VkFormat format)
    {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }

    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
    {
        VkCommandBuffer commandBuffer = beginSingleTimeCommand();

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = { width, height, 1 };

        vkCmdCopyBufferToImage(
            commandBuffer,
            buffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
        );

        endSingleTimeCommand(commandBuffer);
    }

    void copyBufferToImageCubemap(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
    {
        VkCommandBuffer commandBuffer = beginSingleTimeCommand();
        std::vector<VkBufferImageCopy> bufferCopyRegions;

        for (uint32_t face = 0; face < 6; face++)
        {
            VkBufferImageCopy region{};
            region.bufferOffset = face * width * height * 4;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = face;
            region.imageSubresource.layerCount = 1;
            region.imageOffset = { 0, 0, 0 };
            region.imageExtent = { width, height, 1 };
            bufferCopyRegions.push_back(region);
        }

        vkCmdCopyBufferToImage(
            commandBuffer,
            buffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            static_cast<uint32_t>(bufferCopyRegions.size()),
            bufferCopyRegions.data()
        );

        endSingleTimeCommand(commandBuffer);
    }

    void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t numMipLevels)
    {
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);

        if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
        {
            throw std::runtime_error("texture image format does not support linear blitting!");
        }

        VkCommandBuffer commandBuffer = beginSingleTimeCommand();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        int32_t mipWidth = texWidth;
        int32_t mipHeight = texHeight;

        for (uint32_t i = 1; i < numMipLevels; i++)
        {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);

            VkImageBlit blit{};
            blit.srcOffsets[0] = {0, 0, 0};
            blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = {0, 0, 0};
            blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(commandBuffer,
                           image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1, &blit,
                           VK_FILTER_LINEAR);

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            
            vkCmdPipelineBarrier(commandBuffer,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);

            if (mipWidth > 1) mipWidth /= 2;
            if (mipHeight > 1) mipHeight /= 2;
        }

        barrier.subresourceRange.baseMipLevel = numMipLevels - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier);

        endSingleTimeCommand(commandBuffer);
    }

    VkCommandBuffer beginSingleTimeCommand()
    {
        VkCommandBuffer commandBuffer;

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate command buffer!");
        }

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to begin command buffer!");
        }

        return commandBuffer;
    }

    void endSingleTimeCommand(VkCommandBuffer commandBuffer)
    {
        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to end command buffer!");
        }

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to submit command buffer!");
        }

        vkQueueWaitIdle(graphicsQueue);
    }

    #pragma endregion

    #pragma region CreateMilkyWayTexture()
    void createMilkyWayTexture()
    {
        std::vector<std::string> milkyWayFaces = {
            "textures/_px.png", "textures/_nx.png",
            "textures/_py.png", "textures/_ny.png",
            "textures/_pz.png", "textures/_nz.png"
        };

        int texWidth, texHeight, texChannels;
        stbi_uc* dummy = stbi_load(milkyWayFaces[0].c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        
        if (!dummy) {
            std::cerr << "Warning: Milky Way texture not found, skipping..." << std::endl;
            return;
        }
        stbi_image_free(dummy);

        VkDeviceSize layerSize = texWidth * texHeight * 4;
        VkDeviceSize imageSize = layerSize * 6;

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);

        for (int i = 0; i < 6; i++) {
            stbi_uc* pixels = stbi_load(milkyWayFaces[i].c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
            if (!pixels) {
                std::cerr << "Warning: failed to load milky way face: " << milkyWayFaces[i] << std::endl;
                vkUnmapMemory(device, stagingBufferMemory);
                vkDestroyBuffer(device, stagingBuffer, nullptr);
                vkFreeMemory(device, stagingBufferMemory, nullptr);
                return;
            }
            memcpy(static_cast<stbi_uc*>(data) + (layerSize * i), pixels, static_cast<size_t>(layerSize));
            stbi_image_free(pixels);
        }
        vkUnmapMemory(device, stagingBufferMemory);

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        imageInfo.extent.width = texWidth;
        imageInfo.extent.height = texHeight;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 6;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

        if (vkCreateImage(device, &imageInfo, nullptr, &milkyWayImage) != VK_SUCCESS) {
            throw std::runtime_error("failed to create milky way cubemap image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, milkyWayImage, &memRequirements);
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryTypeIndex(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        
        if (vkAllocateMemory(device, &allocInfo, nullptr, &milkyWayImageMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate milky way cubemap memory!");
        }
        vkBindImageMemory(device, milkyWayImage, milkyWayImageMemory, 0);

        transitionImageLayout(milkyWayImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, 6);
        copyBufferToImageCubemap(stagingBuffer, milkyWayImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
        transitionImageLayout(milkyWayImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, 6);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }
    #pragma endregion

    #pragma region CreatePlanetTextures()
    void createPlanetTextures()
    {
        std::array<std::string, 10> planetNames = {
            "2k_sun", "2k_mercury", "2k_venus", "2k_earth", "2k_moon",
            "2k_mars", "2k_jupiter", "2k_saturn", "2k_uranus", "2k_neptune"
        };

        stbi_set_flip_vertically_on_load(true);

        for (int planetIdx = 0; planetIdx < 10; planetIdx++) {
            std::string texturePath = "textures/" + planetNames[planetIdx] + ".jpg";
            
            int texWidth, texHeight, texChannels;
            stbi_uc* pixels = stbi_load(texturePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
            
            if (!pixels) {
                std::cerr << "Warning: failed to load planet texture: " << texturePath << std::endl;
                texWidth = 1;
                texHeight = 1;
                pixels = new stbi_uc[4]{255, 255, 255, 255};
            }

            VkDeviceSize imageSize = texWidth * texHeight * 4;
            VkBuffer stagingBuffer;
            VkDeviceMemory stagingBufferMemory;
            createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

            void* data;
            vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
            memcpy(data, pixels, static_cast<size_t>(imageSize));
            vkUnmapMemory(device, stagingBufferMemory);

            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
            imageInfo.extent.width = texWidth;
            imageInfo.extent.height = texHeight;
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            if (vkCreateImage(device, &imageInfo, nullptr, &planetImages[planetIdx]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create planet image!");
            }

            VkMemoryRequirements memRequirements;
            vkGetImageMemoryRequirements(device, planetImages[planetIdx], &memRequirements);
            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = findMemoryTypeIndex(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            
            if (vkAllocateMemory(device, &allocInfo, nullptr, &planetImageMemories[planetIdx]) != VK_SUCCESS) {
                throw std::runtime_error("failed to allocate planet image memory!");
            }
            vkBindImageMemory(device, planetImages[planetIdx], planetImageMemories[planetIdx], 0);

            transitionImageLayout(planetImages[planetIdx], VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, 1);
            copyBufferToImage(stagingBuffer, planetImages[planetIdx], static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
            transitionImageLayout(planetImages[planetIdx], VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, 1);

            vkDestroyBuffer(device, stagingBuffer, nullptr);
            vkFreeMemory(device, stagingBufferMemory, nullptr);
            stbi_image_free(pixels);
        }
    }
    #pragma endregion

    #pragma region CreateTextureImageView()
    void createTextureImageView()
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = textureImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 6;

        if (vkCreateImageView(device, &viewInfo, nullptr, &textureImageView) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create cubemap image view!");
        }

        if (milkyWayImage != VK_NULL_HANDLE)
        {
            VkImageViewCreateInfo milkyWayViewInfo{};
            milkyWayViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            milkyWayViewInfo.image = milkyWayImage;
            milkyWayViewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
            milkyWayViewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
            milkyWayViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            milkyWayViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            milkyWayViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            milkyWayViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            milkyWayViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            milkyWayViewInfo.subresourceRange.baseMipLevel = 0;
            milkyWayViewInfo.subresourceRange.levelCount = 1;
            milkyWayViewInfo.subresourceRange.baseArrayLayer = 0;
            milkyWayViewInfo.subresourceRange.layerCount = 6;

            if (vkCreateImageView(device, &milkyWayViewInfo, nullptr, &milkyWayImageView) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create milky way image view!");
            }
        }

        for (int i = 0; i < 10; i++)
        {
            if (planetImages[i] != VK_NULL_HANDLE)
            {
                VkImageViewCreateInfo planetViewInfo{};
                planetViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                planetViewInfo.image = planetImages[i];
                planetViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                planetViewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
                planetViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
                planetViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
                planetViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
                planetViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
                planetViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                planetViewInfo.subresourceRange.baseMipLevel = 0;
                planetViewInfo.subresourceRange.levelCount = 1;
                planetViewInfo.subresourceRange.baseArrayLayer = 0;
                planetViewInfo.subresourceRange.layerCount = 1;

                if (vkCreateImageView(device, &planetViewInfo, nullptr, &planetImageViews[i]) != VK_SUCCESS)
                {
                    throw std::runtime_error("failed to create planet image view!");
                }
            }
        }
    }

    #pragma endregion

    #pragma region CreateTextureSampler()
    void createTextureSampler()
    {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = 16;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = VK_LOD_CLAMP_NONE;

        if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create texture sampler!");
        }
    }

    #pragma endregion

    #pragma region CreateStorageImage()
    void createStorageImage()
    {
        VkFormat storageFormat = VK_FORMAT_R8G8B8A8_UNORM;

        createImage(swapChainExtent.width, swapChainExtent.height, 1, VK_SAMPLE_COUNT_1_BIT, 
                   storageFormat, VK_IMAGE_TILING_OPTIMAL, 
                   VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, 
                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
                   storageImage, storageImageMemory);

        storageImageView = createImageView(storageImage, storageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

        transitionImageLayout(storageImage, storageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, 1, 1);
    }

    #pragma endregion

    #pragma region CreateDescriptorSetLayoutPoolSets()
    void createDescriptorSetLayout()
    {
        VkDescriptorSetLayoutBinding storageImageLayoutBinding{};
        storageImageLayoutBinding.binding = 0;
        storageImageLayoutBinding.descriptorCount = 1;
        storageImageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        storageImageLayoutBinding.pImmutableSamplers = nullptr;
        storageImageLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		VkDescriptorSetLayoutBinding uniformBufferLayoutBinding{};
		uniformBufferLayoutBinding.binding = 1;
		uniformBufferLayoutBinding.descriptorCount = 1;
		uniformBufferLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformBufferLayoutBinding.pImmutableSamplers = nullptr;
		uniformBufferLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutBinding skyboxLayoutBinding{};
        skyboxLayoutBinding.binding = 2;
        skyboxLayoutBinding.descriptorCount = 1;
        skyboxLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        skyboxLayoutBinding.pImmutableSamplers = nullptr;
        skyboxLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutBinding milkyWayLayoutBinding{};
        milkyWayLayoutBinding.binding = 3;
        milkyWayLayoutBinding.descriptorCount = 1;
        milkyWayLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        milkyWayLayoutBinding.pImmutableSamplers = nullptr;
        milkyWayLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutBinding planetTexsLayoutBinding{};
        planetTexsLayoutBinding.binding = 4;
        planetTexsLayoutBinding.descriptorCount = 10;
        planetTexsLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        planetTexsLayoutBinding.pImmutableSamplers = nullptr;
        planetTexsLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        std::array<VkDescriptorSetLayoutBinding, 5> bindings = {storageImageLayoutBinding, uniformBufferLayoutBinding, skyboxLayoutBinding, milkyWayLayoutBinding, planetTexsLayoutBinding};
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create RT descriptor set layout!");
        }
    }

    void createDescriptorPool()
    { 
        std::array<VkDescriptorPoolSize, 3> poolSizes{};
         
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
         
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[2].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * 12);

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
         
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    void createDescriptorSets()
    { 
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
         
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT); 
        allocInfo.pSetLayouts = layouts.data();

        descriptorSets.clear();
        descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }
         
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        { 
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            imageInfo.imageView = storageImageView;
             
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = cameraUniformBuffers[i];  
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(CameraUBO);

            VkDescriptorImageInfo skyboxInfo{};
            skyboxInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            skyboxInfo.imageView = textureImageView;
            skyboxInfo.sampler = textureSampler; 
             
            VkDescriptorImageInfo milkyWayInfo{};
            milkyWayInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            milkyWayInfo.imageView = milkyWayImageView;
            milkyWayInfo.sampler = textureSampler;
             
            std::array<VkDescriptorImageInfo, 10> planetInfos{};
            for (int j = 0; j < 10; j++) {
                planetInfos[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                planetInfos[j].imageView = planetImageViews[j];
                planetInfos[j].sampler = textureSampler;
            }
             
            std::array<VkWriteDescriptorSet, 5> descriptorWrites{};

            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = descriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pImageInfo = &imageInfo;

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = descriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pBufferInfo = &bufferInfo;

            descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[2].dstSet = descriptorSets[i];
            descriptorWrites[2].dstBinding = 2;
            descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[2].descriptorCount = 1;
            descriptorWrites[2].pImageInfo = &skyboxInfo;
             
            descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[3].dstSet = descriptorSets[i];
            descriptorWrites[3].dstBinding = 3;
            descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[3].descriptorCount = 1;
            descriptorWrites[3].pImageInfo = &milkyWayInfo;
             
            descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[4].dstSet = descriptorSets[i];
            descriptorWrites[4].dstBinding = 4;
            descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[4].descriptorCount = 10;
            descriptorWrites[4].pImageInfo = planetInfos.data();

            vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
    }

    #pragma endregion

    #pragma region CreatePipeline()
    void createPipeline()
    {
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create compute pipeline layout!");
        }
         
        auto computeShaderCode = readFile("shaders/comp.spv");
        VkShaderModule computeShaderModule = createShaderModule(computeShaderCode);

        VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
        computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT; 
        computeShaderStageInfo.module = computeShaderModule;
        computeShaderStageInfo.pName = "main";
         
        VkComputePipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.stage = computeShaderStageInfo;
         
        if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create compute pipeline!");
        }

        vkDestroyShaderModule(device, computeShaderModule, nullptr);
    }

    static std::vector<char> readFile(const std::string& filename)
    {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file: " + filename);
        }

        size_t fileSize = (size_t) file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }

    VkShaderModule createShaderModule(const std::vector<char>& code)
    {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module!");
        }

        return shaderModule;
    }
    
    #pragma endregion

    #pragma region CreateCommandBuffers()
    void createCommandBuffers() {
        commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; 
        allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

        if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }

    #pragma endregion

    #pragma region CreateUniformBuffers()
    void createUniformBuffers()
    {
        VkDeviceSize bufferSize = sizeof(CameraUBO);

        cameraUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        cameraUniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        { 
            createBuffer(bufferSize,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                cameraUniformBuffers[i],
                cameraUniformBuffersMemory[i]);
             
            vkMapMemory(device, cameraUniformBuffersMemory[i], 0, bufferSize, 0, &cameraBuffersMapped[i]);
        }
    }

	#pragma endregion

    #pragma region CreateSyncObjects()
    void createSyncObjects()
    {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT); 

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create synchronization objects for a frame!");
            }
        }
    }

    #pragma endregion

#pragma endregion

#pragma region Camera Functions

    void processMouseMovement(float xpos, float ypos) 
    {
        if (traceMode || !mouseControlEnabled) return;

        if (ignoreNextMouseMove) {
            ignoreNextMouseMove = false;
            hX = xpos;
            hY = ypos;
            return;
        }

        if (firstMouse) 
        {
            hX = xpos;
            hY = ypos;
            firstMouse = false;
        }

        float xoffset = hX - xpos;
        float yoffset = hY - ypos;
        hX = xpos;
        hY = ypos;

        xoffset *= MOUSE_SENSITIVITY;
        yoffset *= MOUSE_SENSITIVITY;

        yaw += xoffset;
        pitch += yoffset;

        if (pitch > 89.0f)
            pitch = 89.0f;
        if (pitch < -89.0f)
            pitch = -89.0f;

        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        cameraFront = glm::normalize(front);
    }

    void processInput() 
    {
        if (isInputActive) return;
        
        float speed = CAMERA_SPEED * deltaTime;

        if (traceMode) speed = 0.0f;

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            cameraPos += speed * cameraFront;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            cameraPos -= speed * cameraFront;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            cameraPos += speed * cameraUp;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            cameraPos -= speed * cameraUp;
            
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS)
        {
            std::cout << "Camera Position: " << cameraPos.x << " " << cameraPos.y << " " << cameraPos.z << std::endl;
            std::cout << "Camera Rotation (Pitch, Yaw): " << pitch << " " << yaw << std::endl;

            if (!mouseControlEnabled)
            {
                mouseControlEnabled = true;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                firstMouse = true;
            }
            else
            {
                traceMode = !traceMode;
                if (!traceMode) glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                else glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
        {
            relativisticViewEnabled = !relativisticViewEnabled;
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
        {
            showFPS = !showFPS;
            if (showFPS) {
                std::cout << "FPS Display: ON" << std::endl;
            } else {
                std::cout << "FPS Display: OFF" << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
        {
            method = !method;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
        {
            isInputActive = true;
            std::cin >> stepSize;
            isInputActive = false;
        }

        if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
        {
            isInputActive = true;
            std::cin >> maxSteps;
            isInputActive = false;
        }

        if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
        {
            isInputActive = true;
            std::cin >> metric;
            isInputActive = false;
        }

        if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS)
        {
            isInputActive = true;
            std::cin >> spinSpeed;
            isInputActive = false;
        }

        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
        {
            currentScene = 1;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
        {
            currentScene = 2;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
        {
            currentScene = 3;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS)
        {
            currentScene = 4;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS)
        {
            currentScene = 5;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS)
        {
            currentScene = 6;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // Resolution switching with F1-F5 keys
        if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS)
        {
            setResolution(0); // 144p
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS)
        {
            setResolution(1); // 480p
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if (glfwGetKey(window, GLFW_KEY_F3) == GLFW_PRESS)
        {
            setResolution(2); // 720p
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if (glfwGetKey(window, GLFW_KEY_F4) == GLFW_PRESS)
        {
            setResolution(3); // 1080p
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    public:void setResolution(uint32_t resolutionIndex)
    {
        if (resolutionIndex >= RESOLUTION_PRESETS.size()) return;

        currentResolutionIndex = resolutionIndex;
        const Resolution& resolution = RESOLUTION_PRESETS[resolutionIndex];
        currentWindowWidth = resolution.width;
        currentWindowHeight = resolution.height;

        glfwSetWindowSize(window, resolution.width, resolution.height);
        std::cout << "Resolution changed to: " << resolution.name << " (" << resolution.width << "x" << resolution.height << ")" << std::endl;

        // Reset mouse position to center after resolution change
        float centerX = resolution.width / 2.0f;
        float centerY = resolution.height / 2.0f;
        glfwSetCursorPos(window, centerX, centerY);
        hX = centerX;
        hY = centerY;
        
        // Ignore the next mouse movement callback to prevent camera jitter
        ignoreNextMouseMove = true;

        framebufferResized = true; 

        centerWindow();
    }

    void updateFPS()
    {
        frameCount++;
        fpsUpdateTimer += deltaTime;

        if (fpsUpdateTimer >= 1.0f) {
            currentFPS = frameCount / fpsUpdateTimer;
            frameCount = 0;
            fpsUpdateTimer = 0.0f;

            if (showFPS) {
                std::cout << "FPS: " << currentFPS << std::endl;
            }
        }
    }

#pragma endregion

#pragma region MainLoop()
    void mainLoop()
    {
        BenchmarkAutomator automator;

        setResolution(0); 

        automator.spinIndex = 1;
        automator.gravIndex = 1;
        automator.stepIndex = 1;
        automator.camIndex = 5;
        automator.sceneIndex = 3;
        automator.integratorIndex = 0;
        automator.metricIndex = 2;
        automator.resIndex = 2;

        automator.applyConfig(this);

        while (!glfwWindowShouldClose(window))
        {  
            float currentTime = static_cast<float>(glfwGetTime());
            deltaTime = currentTime - lastFrame;
            lastFrame = currentTime;

            glfwPollEvents();

            //automator.update(deltaTime, this); 

            processInput();
            updateFPS(); 

            drawFrame();
        }

        vkDeviceWaitIdle(device);
    }

    void saveScreenshot(const char* filename) {
        vkDeviceWaitIdle(device);
         
        fs::path filePath(filename);
        fs::create_directories(filePath.parent_path());

        VkDeviceSize imageSize = swapChainExtent.width * swapChainExtent.height * 4;
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;

        createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer, stagingBufferMemory);

        VkCommandBuffer commandBuffer = beginSingleTimeCommand();
         
        VkImageMemoryBarrier preTransferBarrier{};
        preTransferBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        preTransferBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        preTransferBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        preTransferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        preTransferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        preTransferBarrier.image = storageImage; 
        preTransferBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        preTransferBarrier.subresourceRange.baseMipLevel = 0;
        preTransferBarrier.subresourceRange.levelCount = 1;
        preTransferBarrier.subresourceRange.baseArrayLayer = 0;
        preTransferBarrier.subresourceRange.layerCount = 1;
        preTransferBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        preTransferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &preTransferBarrier);

        VkBufferImageCopy region{};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.layerCount = 1;
        region.imageExtent = { swapChainExtent.width, swapChainExtent.height, 1 };
         
        vkCmdCopyImageToBuffer(commandBuffer, storageImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            stagingBuffer, 1, &region);
         
        VkImageMemoryBarrier postTransferBarrier{};
        postTransferBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        postTransferBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        postTransferBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        postTransferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        postTransferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        postTransferBarrier.image = storageImage; // Corrigido: Usando a storageImage
        postTransferBarrier.subresourceRange = preTransferBarrier.subresourceRange;
        postTransferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        postTransferBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &postTransferBarrier);

        endSingleTimeCommand(commandBuffer);

        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);

        if (stbi_write_png(filename, swapChainExtent.width, swapChainExtent.height, 4, (unsigned char*)data, swapChainExtent.width * 4))
        {
            std::cout << "Screenshot salvo: " << filename << std::endl;
        }
        else
        {
            std::cerr << "Erro ao salvar screenshot: " << filename << std::endl;
        }

        vkUnmapMemory(device, stagingBufferMemory);
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    void centerWindow()
    {
        const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        if (!mode) return;

        int screenWidth = mode->width;
        int screenHeight = mode->height;
        
        int windowWidth, windowHeight;
        glfwGetWindowSize(window, &windowWidth, &windowHeight);
        
        int xPos = (screenWidth - windowWidth) / 2;
        int yPos = (screenHeight - windowHeight) / 2;
        
        glfwSetWindowPos(window, xPos, yPos);
    }

    void drawFrame()
    { 
        VkResult waitResult = vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        if (waitResult == VK_ERROR_DEVICE_LOST) {
            std::cerr << "[FATAL] Device Lost detectado em vkWaitForFences. Encerrando aplicação para evitar crash." << std::endl;
            glfwSetWindowShouldClose(window, true);
            return;
        }
        if (waitResult != VK_SUCCESS) {
            throw std::runtime_error("Falha ao aguardar fences!");
        }

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(
            device,
            swapChain,
            UINT64_MAX,
            imageAvailableSemaphores[currentFrame],
            VK_NULL_HANDLE,
            &imageIndex
        );

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            return;
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) 
        { 
            if (result == VK_ERROR_DEVICE_LOST) 
            {
                std::cerr << "[FATAL] Device Lost em vkAcquireNextImageKHR." << std::endl;
                glfwSetWindowShouldClose(window, true);
                return;
            }
            throw std::runtime_error("failed to acquire swap chain image!");
        }
         
        if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
            vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
        }

        imagesInFlight[imageIndex] = inFlightFences[currentFrame]; 

        vkResetFences(device, 1, &inFlightFences[currentFrame]);

		updateUniformBuffer(currentFrame);

        vkResetCommandBuffer(commandBuffers[currentFrame], 0);

        try {
            recordCommandBuffer(commandBuffers[currentFrame], imageIndex);
        }
        catch (const std::runtime_error& e) {
            std::cerr << "Erro na gravação do command buffer: " << e.what() << std::endl;
            return;
        }

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

        VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;
         
        VkResult submitResult = vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]);

        if (submitResult == VK_ERROR_DEVICE_LOST) {
            std::cerr << "[FATAL] Device Lost em vkQueueSubmit (Provavel TDR/Timeout)." << std::endl;
            glfwSetWindowShouldClose(window, true);
            return;
        }
        if (submitResult != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = { swapChain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;

        result = vkQueuePresentKHR(presentQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
            framebufferResized = false;
            recreateSwapChain();
        }
        else if (result == VK_ERROR_DEVICE_LOST) {
            std::cerr << "[FATAL] Device Lost em vkQueuePresentKHR." << std::endl;
            glfwSetWindowShouldClose(window, true);
        }
        else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO; 

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to begin recording command buffer!");
        }
             
        VkImageMemoryBarrier storageImageBarrier{};
        storageImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        storageImageBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        storageImageBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL; 
        storageImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        storageImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        storageImageBarrier.image = storageImage;
        storageImageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        storageImageBarrier.subresourceRange.baseMipLevel = 0;
        storageImageBarrier.subresourceRange.levelCount = 1;
        storageImageBarrier.subresourceRange.baseArrayLayer = 0;
        storageImageBarrier.subresourceRange.layerCount = 1;
        storageImageBarrier.srcAccessMask = 0;
        storageImageBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 
            0,
            0, nullptr,
            0, nullptr,
            1, &storageImageBarrier
        );
             
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);  

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);
             
        uint32_t groupCountX = (swapChainExtent.width + 15) / 16;
        uint32_t groupCountY = (swapChainExtent.height + 15) / 16;

        vkCmdDispatch(commandBuffer, groupCountX, groupCountY, 1);  
             
        VkImageMemoryBarrier afterComputeBarrier{};
        afterComputeBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        afterComputeBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        afterComputeBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        afterComputeBarrier.image = storageImage;
        afterComputeBarrier.subresourceRange = storageImageBarrier.subresourceRange; 
        afterComputeBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        afterComputeBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
             
        VkImageMemoryBarrier swapChainBarrier{};
        swapChainBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        swapChainBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        swapChainBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        swapChainBarrier.image = swapChainImages[imageIndex];
        swapChainBarrier.subresourceRange = storageImageBarrier.subresourceRange;
        swapChainBarrier.srcAccessMask = 0;
        swapChainBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        std::array<VkImageMemoryBarrier, 2> barriers = { afterComputeBarrier, swapChainBarrier };

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  
            VK_PIPELINE_STAGE_TRANSFER_BIT, 
            0,
            0, nullptr,
            0, nullptr,
            static_cast<uint32_t>(barriers.size()), barriers.data()
        );
             
        VkImageCopy copyRegion{};
        copyRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        copyRegion.srcOffset = { 0, 0, 0 };
        copyRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        copyRegion.dstOffset = { 0, 0, 0 };
        copyRegion.extent = { swapChainExtent.width, swapChainExtent.height, 1 };

        vkCmdCopyImage(
            commandBuffer,
            storageImage, VK_IMAGE_LAYOUT_GENERAL,
            swapChainImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &copyRegion
        );
             
        VkImageMemoryBarrier presentBarrier{};
        presentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        presentBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        presentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        presentBarrier.image = swapChainImages[imageIndex];
        presentBarrier.subresourceRange = storageImageBarrier.subresourceRange;
        presentBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        presentBarrier.dstAccessMask = 0;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &presentBarrier
        );

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) 
        {
            throw std::runtime_error("failed to record command buffer!");
        } 
    }

    void recreateSwapChain()
    {
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(device);

        cleanupSwapChain();

        createSwapChain();
        createImageViews();
        createStorageImage();
        createDescriptorSets();  
    
        imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);
    }

    void updateUniformBuffer(uint32_t currentImage)
    { 
        float aspect_ratio = swapChainExtent.width / (float)swapChainExtent.height;
        float viewport_height = 1.0f;
        float viewport_width = aspect_ratio * viewport_height;
        float focal_length = 1.0f;
          
        glm::vec3 w = glm::normalize(cameraFront); 
        glm::vec3 u = glm::normalize(glm::cross(cameraUp, w)); 
        glm::vec3 v = glm::cross(w, u);

        glm::vec3 horizontal = viewport_width * u;
        glm::vec3 vertical = viewport_height * v;
         
        glm::vec3 lower_left_corner = cameraPos - (horizontal / 2.0f) - (vertical / 2.0f) + (w * focal_length);
         
        CameraUBO ubo{};
        ubo.origin = cameraPos;
        ubo.lower_left_corner = lower_left_corner;
        ubo.horizontal = horizontal;
        ubo.vertical = vertical;
		ubo.relativistic_view_enabled = relativisticViewEnabled ? 1 : 0;
        ubo.method_euler = method;
        ubo.step_size = stepSize;
        ubo.max_steps = maxSteps;
        ubo.metric = metric;
        ubo.spin_speed = spinSpeed;
        ubo.current_scene = currentScene;
        ubo.gravity_multiplier = gravityMultiplier;
         
        memcpy(cameraBuffersMapped[currentImage], &ubo, sizeof(ubo));
    }
#pragma endregion

#pragma region Cleanup()

    void cleanupSwapChain() 
    {
        if (storageImageView != VK_NULL_HANDLE) 
        {
            vkDestroyImageView(device, storageImageView, nullptr);
            storageImageView = VK_NULL_HANDLE;
        }
        if (storageImage != VK_NULL_HANDLE) 
        {
            vkDestroyImage(device, storageImage, nullptr);
            storageImage = VK_NULL_HANDLE;
        }
        if (storageImageMemory != VK_NULL_HANDLE) 
        {
            vkFreeMemory(device, storageImageMemory, nullptr);
            storageImageMemory = VK_NULL_HANDLE;
        }

        for (auto imageView : swapChainImageViews) 
        {
            vkDestroyImageView(device, imageView, nullptr);
        }
        swapChainImageViews.clear();
        swapChainImages.clear();

        vkDestroySwapchainKHR(device, swapChain, nullptr);
        swapChain = VK_NULL_HANDLE;

        if (!descriptorSets.empty())
        {
            vkFreeDescriptorSets(device, descriptorPool, static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data());
            descriptorSets.clear();
        }
    }

    void cleanup()
    {
        if (vkDeviceWaitIdle(device) != VK_SUCCESS) {
            std::cerr << "Aviso: vkDeviceWaitIdle falhou no cleanup (esperado se houve Device Lost)." << std::endl;
        }

        cleanupSwapChain();

        if (pipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(device, pipeline, nullptr);
        }
        if (pipelineLayout != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        }
        if (descriptorPool != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        }
        if (descriptorSetLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        }

        if (textureSampler != VK_NULL_HANDLE)
        {
            vkDestroySampler(device, textureSampler, nullptr);
        }
        if (textureImageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(device, textureImageView, nullptr);
        }
        if (textureImage != VK_NULL_HANDLE)
        {
            vkDestroyImage(device, textureImage, nullptr);
        }
        if (textureImageMemory != VK_NULL_HANDLE)
        {
            vkFreeMemory(device, textureImageMemory, nullptr);
        }

        if (commandPool != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(device, commandPool, nullptr);
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(device, inFlightFences[i], nullptr);
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroyBuffer(device, cameraUniformBuffers[i], nullptr);
            vkFreeMemory(device, cameraUniformBuffersMemory[i], nullptr);
        }
         
        if (milkyWayImageView != VK_NULL_HANDLE) 
        {
            vkDestroyImageView(device, milkyWayImageView, nullptr);
        }
        if (milkyWayImage != VK_NULL_HANDLE) 
        {
            vkDestroyImage(device, milkyWayImage, nullptr);
        }
        if (milkyWayImageMemory != VK_NULL_HANDLE) 
        {
            vkFreeMemory(device, milkyWayImageMemory, nullptr);
        }
         
        for (int i = 0; i < 10; i++) 
        {
            if (planetImageViews[i] != VK_NULL_HANDLE) 
            {
                vkDestroyImageView(device, planetImageViews[i], nullptr);
            }
            if (planetImages[i] != VK_NULL_HANDLE) 
            {
                vkDestroyImage(device, planetImages[i], nullptr);
            }
            if (planetImageMemories[i] != VK_NULL_HANDLE) 
            {
                vkFreeMemory(device, planetImageMemories[i], nullptr);
            }
        }

        vkDestroyDevice(device, nullptr);

        if (enableValidationLayers)
        {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }

        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);

        glfwDestroyWindow(window);
        glfwTerminate();
    }

#pragma endregion

};  

inline void BenchmarkAutomator::update(float deltaTime, Game* game) {
    if (isFinished) return;
    
    if (currentPhase == Phase::Baseline && !baselineFinished) { 
        if (warmUpTimer < WARM_UP_DURATION) {
            if (!configApplied) {
                game->metric = 0;
                game->method = true;
                relativisticViewEnabled = false;
                game->currentScene = baselineSceneIndex;
                 
                CameraPreset camPreset = (baselineSceneIndex == 6) ? scene6Cameras[camIndex] : defaultCameras[camIndex];
                game->cameraPos = camPreset.position;
                game->pitch = camPreset.pitch;
                game->yaw = camPreset.yaw;
                
                glm::vec3 front;
                front.x = cos(glm::radians(game->yaw)) * cos(glm::radians(game->pitch));
                front.y = sin(glm::radians(game->pitch));
                front.z = sin(glm::radians(game->yaw)) * cos(glm::radians(game->pitch));
                game->cameraFront = glm::normalize(front);
                
                std::cout << "Baseline: Scene " << baselineSceneIndex << ", Camera " << (camIndex + 1) 
                    << " at resolution " << RESOLUTION_PRESETS[resIndex].height << "p" << std::endl;
                
                configApplied = true;
            }
            
            warmUpTimer += deltaTime;
            return;
        }
         
        timer += deltaTime;
        frameCount++;
        
        if (timer >= benchmarkDurationPerConfig) {
            float avgFps = (timer > 0) ? (frameCount / timer) : 0.0f;
            saveBaselineScreenshot(avgFps, game);
            advanceBaseline(game);
            configApplied = false;
            warmUpTimer = 0.0f;
            timer = 0.0f;
            frameCount = 0;
        }
        return;
    }
     
    if (currentPhase == Phase::Full) { 
        if (warmUpTimer < WARM_UP_DURATION) {
            if (!configApplied) {
                relativisticViewEnabled = true;
                applyConfig(game);
                configApplied = true;
            }
            
            warmUpTimer += deltaTime;
            return;
        }
         
        timer += deltaTime;
        frameCount++;
        
        if (timer >= benchmarkDurationPerConfig) {
            float avgFps = (timer > 0) ? (frameCount / timer) : 0.0f;
            saveBenchmark(avgFps, game);
            advance(game);
            configApplied = false;
            warmUpTimer = 0.0f;
            timer = 0.0f;
            frameCount = 0;
        }
    }
}

inline void BenchmarkAutomator::advanceBaseline(Game* game) {
    camIndex++;
     
    size_t maxCams = (baselineSceneIndex == 6) ? scene6Cameras.size() : defaultCameras.size();
    
    if (camIndex >= maxCams) {
        camIndex = 0;
        baselineSceneIndex++;
        
        if (baselineSceneIndex > 6) {
            baselineSceneIndex = 1;
            game->setResolution(++resIndex);
            
            if (resIndex >= RESOLUTION_PRESETS.size()) {
                baselineFinished = true;
                currentPhase = Phase::Full;
                resIndex = 0;
                metricIndex = 0;
                integratorIndex = 0;
                sceneIndex = 1;
                camIndex = 0;
                stepIndex = 0;
                gravIndex = 0;
                spinIndex = 0;
                baselineSceneIndex = 1;
                warmUpTimer = 0.0f;
                std::cout << ">>> BASELINE PHASE CONCLUÍDO <<<" << std::endl;
                std::cout << ">>> INICIANDO BENCHMARK COMPLETO (RELATIVÍSTICO) <<<" << std::endl;
            }
        }
    }
}

inline void BenchmarkAutomator::saveBaselineScreenshot(float fps, Game* game) { 
    CameraPreset camPreset = (baselineSceneIndex == 6) ? scene6Cameras[camIndex] : defaultCameras[camIndex];

    char filename[512];
    sprintf(filename, "Benchmarks/BASELINE_%.1fFPS_NonRelativistic_S%d_%s_%dp.png",
        fps, baselineSceneIndex, camPreset.name.c_str(), RESOLUTION_PRESETS[resIndex].height);

    game->saveScreenshot(filename);
    saveBaselineToCSV(fps, filename);
}

inline void BenchmarkAutomator::saveBaselineToCSV(float fps, const std::string& imagePath) { 
    CameraPreset camPreset = (baselineSceneIndex == 6) ? scene6Cameras[camIndex] : defaultCameras[camIndex];

    std::string csvPath = "Benchmarks/Baseline_Results.csv";
    bool fileExists = fs::exists(csvPath);

    std::ofstream csvFile;
    csvFile.open(csvPath, std::ios_base::app);

    if (!fileExists) {
        csvFile << "Resolution_W,Resolution_H,Scene_ID,Camera_Name,Average_FPS,Image_Path\n";
    }

    csvFile << RESOLUTION_PRESETS[resIndex].width << ","
        << RESOLUTION_PRESETS[resIndex].height << ","
        << baselineSceneIndex << ","
        << camPreset.name << ","
        << std::fixed << std::setprecision(2) << fps << ","
        << imagePath << "\n";

    csvFile.close();
}

inline void BenchmarkAutomator::applyConfig(Game* game) {
    game->setResolution(resIndex);
    game->metric = metricIndex;
    game->method = (integratorIndex == 0);
    game->currentScene = sceneIndex;

    CameraPreset camPreset = (sceneIndex == 6) ? scene6Cameras[camIndex] : defaultCameras[camIndex];
    StepSizePreset step = stepPresets[stepIndex];
    GravityPreset grav = (sceneIndex == 6) ? customScene6Gravities[gravIndex] : gravityPresets[gravIndex];
    SpinSpeedPreset spin = spinPresets[spinIndex];

    game->cameraPos = camPreset.position;
    game->pitch = camPreset.pitch;
    game->yaw = camPreset.yaw;

    glm::vec3 front;
    front.x = cos(glm::radians(game->yaw)) * cos(glm::radians(game->pitch));
    front.y = sin(glm::radians(game->pitch));
    front.z = sin(glm::radians(game->yaw)) * cos(glm::radians(game->pitch));
    game->cameraFront = glm::normalize(front);

    game->stepSize = step.value;

    game->gravityMultiplier = grav.value / (1.989e31f);
    game->spinSpeed = spin.value;
}

inline void BenchmarkAutomator::advance(Game* game) {
    spinIndex++;

    int maxSpinIdx = (metricIndex == 2 /* Kerr */) ? spinPresets.size() : 1;
    if (spinIndex >= maxSpinIdx) {
        spinIndex = 0;
        gravIndex++;

        int maxGravIdx = (sceneIndex == 6) ? customScene6Gravities.size() : gravityPresets.size();
        if (gravIndex >= maxGravIdx) {
            gravIndex = 0;
            stepIndex++;

            int maxStepIdx = (integratorIndex == 1 /* RK4 */) ? 1 : stepPresets.size();
            if (stepIndex >= maxStepIdx) {
                stepIndex = 0;
                camIndex++;

                int maxCamIdx = (sceneIndex == 6) ? scene6Cameras.size() : defaultCameras.size();
                if (camIndex >= maxCamIdx) {
                    camIndex = 0;
                    sceneIndex++;

                    if (sceneIndex > 6) {
                        sceneIndex = 1;
                        integratorIndex++;

                        if (integratorIndex > 1) {
                            integratorIndex = 0;
                            metricIndex++;

                            if (metricIndex > 2) {
                                metricIndex = 0;
                                resIndex++;

                                if (resIndex >= RESOLUTION_PRESETS.size()) {
                                    isFinished = true;
                                    std::cout << ">>> BENCHMARK VULKAN CONCLUÍDO <<<" << std::endl;
                                    return;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

inline void BenchmarkAutomator::saveBenchmark(float fps, Game* game) {
    CameraPreset camPreset = (sceneIndex == 6) ? scene6Cameras[camIndex] : defaultCameras[camIndex];
    StepSizePreset step = stepPresets[stepIndex];
    GravityPreset grav = (sceneIndex == 6) ? customScene6Gravities[gravIndex] : gravityPresets[gravIndex];
    SpinSpeedPreset spin = spinPresets[spinIndex];

    std::string metricName = (metricIndex == 0) ? "Newton" : (metricIndex == 1) ? "Schwarzschild" : "Kerr";
    std::string integratorName = (integratorIndex == 0) ? "Euler" : "RK4";

    char filename[512];

    // Old Format: {avgFps}FPS_{metric}_{integrator}_S{sceneId}_{camera}_{step}_{grav}_{spin}_{h}p.png
    //sprintf(filename, "Benchmarks/%.1fFPS_%s_%s_S%d_%s_%s_%s_%s_%dp.png",
    //    fps, metricName.c_str(), integratorName.c_str(), sceneIndex,
    //    camPreset.name.c_str(), step.name.c_str(), grav.name.c_str(), spin.name.c_str(),
    //    RESOLUTION_PRESETS[resIndex].height);

    // New Better Format: _S{sceneId}_{camera}_{metric}_{grav}_{step}_{spin}_{integrator}_{h}p_{avgFps}FPS.png
    sprintf(filename, "Benchmarks/_S%d_%s_%s_%s_%s_%s_%s_%dp_%.1fFPS.png",
        sceneIndex, camPreset.name.c_str(), metricName.c_str(), grav.name.c_str(), 
        step.name.c_str(), spin.name.c_str(), integratorName.c_str(),
        RESOLUTION_PRESETS[resIndex].height, fps);

    game->saveScreenshot(filename);
    saveToCSV(fps, filename);
}

inline void BenchmarkAutomator::saveToCSV(float fps, const std::string& imagePath) {
    CameraPreset camPreset = (sceneIndex == 6) ? scene6Cameras[camIndex] : defaultCameras[camIndex];
    StepSizePreset step = stepPresets[stepIndex];
    GravityPreset grav = (sceneIndex == 6) ? customScene6Gravities[gravIndex] : gravityPresets[gravIndex];
    SpinSpeedPreset spin = spinPresets[spinIndex];

    std::string csvPath = "Benchmarks/TCCBenchmark.csv";
    bool fileExists = fs::exists(csvPath);

    std::ofstream csvFile;
    csvFile.open(csvPath, std::ios_base::app);

    if (!fileExists) {
        csvFile << "Timestamp,Application_Type,Resolution,Resolution_W,Resolution_H,Metric,Integrator,Scene_ID,"
            << "Camera_Name,Camera_Position_X,Camera_Position_Y,Camera_Position_Z,"
            << "Camera_Rotation_X,Camera_Rotation_Y,Camera_Rotation_Z,"
            << "Step_Size,Step_Name,Gravity_Value,Gravity_Name,Spin_Speed,Spin_Name,"
            << "Average_FPS,Frame_Count,Duration_Seconds,Image_Path\n";
    }

    std::string metricName = (metricIndex == 0) ? "Newton" : (metricIndex == 1) ? "Schwarzschild" : "Kerr";
    std::string integratorName = (integratorIndex == 0) ? "Euler" : "RK4";

    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    csvFile << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S") << ","
        << "Vulkan,"
        << RESOLUTION_PRESETS[resIndex].height << "p,"
        << RESOLUTION_PRESETS[resIndex].width << ","
        << RESOLUTION_PRESETS[resIndex].height << ","
        << metricName << ","
        << integratorName << ","
        << sceneIndex << ","
        << camPreset.name << ","
        << std::fixed << std::setprecision(2) << camPreset.position.x << "," << camPreset.position.y << "," << camPreset.position.z << ","
        << camPreset.pitch << "," << camPreset.yaw << ",0.00," // Pitch maps to X rot, Yaw maps to Y rot
        << step.value << ","
        << step.name << ","
        << std::scientific << std::setprecision(2) << grav.value << ","
        << grav.name << ","
        << std::fixed << std::setprecision(2) << spin.value << ","
        << spin.name << ","
        << fps << ","
        << frameCount << ","
        << benchmarkDurationPerConfig << ","
        << imagePath << "\n";

    csvFile.close();
}

int play()
{
    Game game;

    try
    {
        game.run();
    }
    catch (const std::runtime_error& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
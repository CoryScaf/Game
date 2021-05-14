#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <map>
#include <optional>
#include <set>
#include <cstdint>
#include <algorithm>
#include <fstream>

VkResult CreateDebugUtilsMessengerEXT( VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger ) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr( instance, "vkCreateDebugUtilsMessengerEXT" );
    if( func != nullptr ) return func( instance, pCreateInfo, pAllocator, pDebugMessenger );
    else return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void DestroyDebugUtilsMessengerEXT( VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator ) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr( instance, "vkDestroyDebugUtilsMessengerEXT" );
    if( func != nullptr ) {
        func( instance, debugMessenger, pAllocator );
    }
}

class Log {
private:
    static std::shared_ptr<spdlog::logger> vLogger;
    static std::shared_ptr<spdlog::logger> vvLogger;
public:
    static void Init() {
        std::vector<spdlog::sink_ptr> logSinks;
        logSinks.emplace_back( std::make_shared<spdlog::sinks::basic_file_sink_mt>( "Game.log", true ) );
        logSinks[0]->set_pattern( "[%T] [%l] %n: %v" );

        vvLogger = std::make_shared<spdlog::logger>( "I-ENGINE", begin(logSinks), end(logSinks) );
        spdlog::register_logger( vvLogger );
        vvLogger->set_level( spdlog::level::trace );
        vvLogger->flush_on( spdlog::level::trace );

        logSinks.emplace_back( std::make_shared<spdlog::sinks::stdout_color_sink_mt>() );
        logSinks[1]->set_pattern( "%^[%T] %n: %v%$" );

        vLogger = std::make_shared<spdlog::logger>( "ENGINE", begin(logSinks), end(logSinks) );
        spdlog::register_logger( vLogger );
        vLogger->set_level( spdlog::level::trace );
        vLogger->flush_on( spdlog::level::trace );
    }

    static std::shared_ptr<spdlog::logger>& log() {
        return vLogger;
    }
    static std::shared_ptr<spdlog::logger>& vlog() {
        return vvLogger;
    }
};

std::shared_ptr<spdlog::logger> Log::vLogger;
std::shared_ptr<spdlog::logger> Log::vvLogger;

class HelloTriangleApplication {
public:
    void run() {
        Log::Init();
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }
    

private:

    GLFWwindow* window;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    #ifdef NDEBUG
        const bool enableValidationLayers = false;
    #else
        const bool enableValidationLayers = true;
    #endif

    /************************************
     * initWindow
     ************************************/ 
    void initWindow() {
        glfwInit();

        glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
        glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );

        window = glfwCreateWindow( 800, 600, "Vulkan", nullptr, nullptr );
    }

    /************************************
     * initVulkan
     ************************************/ 
    void initVulkan() {
        createInstance();
        setupDebugMessenger();
        createSurface(); 
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createGraphicsPipeline();
    }

    /************************************
     * createImageViews
     ************************************/ 
    void createImageViews() {
        swapChainImageViews.resize( swapChainImages.size() );

        for( size_t i = 0; i < swapChainImages.size(); ++i ) {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapChainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = swapChainImageFormat;

            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            if( vkCreateImageView( device, &createInfo, nullptr, &swapChainImageViews[i] ) != VK_SUCCESS ) {
                throw std::runtime_error( "failed to create image views!" );
            }
        }
    }

    /************************************
     * createSwapChain
     ************************************/ 
    void createGraphicsPipeline() {
        auto vertShaderCode = readFile( "shaders/vert.spv" );
        auto fragShaderCode = readFile( "shaders/frag.spv" );
        Log::log()->info( "vertShaderCode size: {}", vertShaderCode.size() );
        Log::log()->info( "fragShaderCode size: {}", fragShaderCode.size() );
    }

    /************************************
     * createSwapChain
     ************************************/ 
    void createSwapChain() {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport( physicalDevice );

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat( swapChainSupport.formats );
        VkPresentModeKHR presentMode = chooseSwapPresentMode( swapChainSupport.presentModes );
        VkExtent2D extent = chooseSwapExtent( swapChainSupport.capabilities );

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if( swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount ) {
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
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices = findQueueFamilies( physicalDevice );
        uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value() };

        if( indices.graphicsFamily != indices.presentFamily ) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;

        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if( vkCreateSwapchainKHR( device, &createInfo, nullptr, &swapChain ) != VK_SUCCESS ) {
            throw std::runtime_error( "failed to create swap chain!" );
        }

        vkGetSwapchainImagesKHR( device, swapChain, &imageCount, nullptr );
        swapChainImages.resize( imageCount );
        vkGetSwapchainImagesKHR( device, swapChain, &imageCount, swapChainImages.data() );

        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
    }

    /************************************
     * createSurface
     ************************************/ 
    void createSurface() {
        if( glfwCreateWindowSurface( instance, window, nullptr, &surface ) != VK_SUCCESS ) {
            throw std::runtime_error( "failed to create window surface!" );
        }
    }

    /************************************
     * createLogicalDevice
     ************************************/ 
    void createLogicalDevice() {
        QueueFamilyIndices indices = findQueueFamilies( physicalDevice );

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

        float queuePriority = 1.0f;
        for( uint32_t queueFamily : uniqueQueueFamilies ) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back( queueCreateInfo );
        }


        VkPhysicalDeviceFeatures deviceFeatures{};

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        createInfo.queueCreateInfoCount = static_cast<uint32_t>( queueCreateInfos.size() );
        createInfo.pQueueCreateInfos = queueCreateInfos.data();

        createInfo.pEnabledFeatures = &deviceFeatures;

        createInfo.enabledExtensionCount = static_cast<uint32_t>( deviceExtensions.size() );
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        if( enableValidationLayers ) {
            createInfo.enabledLayerCount = static_cast<uint32_t>( validationLayers.size() );
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else {
            createInfo.enabledLayerCount = 0;
        }

        if( vkCreateDevice( physicalDevice, &createInfo, nullptr, &device ) != VK_SUCCESS ) {
            throw std::runtime_error( "failed to create logical device!" );
        }

        vkGetDeviceQueue( device, indices.graphicsFamily.value(), 0, &graphicsQueue );
        vkGetDeviceQueue( device, indices.presentFamily.value(), 0, &presentQueue );
    }

    /************************************
     * querySwapChainSupport
     ************************************/ 
    SwapChainSupportDetails querySwapChainSupport( VkPhysicalDevice device ) {
        SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR( device, surface, &details.capabilities );

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR( device, surface, &formatCount, nullptr );

        if( formatCount != 0 ) {
            details.formats.resize( formatCount );
            vkGetPhysicalDeviceSurfaceFormatsKHR( device, surface, &formatCount, details.formats.data() );
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR( device, surface, &presentModeCount, nullptr );

        if( presentModeCount != 0 ) {
            details.presentModes.resize( presentModeCount );
            vkGetPhysicalDeviceSurfacePresentModesKHR( device, surface, &presentModeCount, details.presentModes.data() );
        }

        return details;
    }

    /************************************
     * chooseSwapSurfaceFormat
     ************************************/ 
    VkSurfaceFormatKHR chooseSwapSurfaceFormat( const std::vector<VkSurfaceFormatKHR>& availableFormats ) {
        for( const auto& availableFormat : availableFormats ) {
            if( availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR ) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    /************************************
     * chooseSwapPresentMode
     ************************************/ 
    VkPresentModeKHR chooseSwapPresentMode( const std::vector<VkPresentModeKHR>& availablePresentModes ) {
        for( const auto& availablePresentMode : availablePresentModes ) {
            if( availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR ) {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    /************************************
     * chooseSwapExtent
     ************************************/ 
    VkExtent2D chooseSwapExtent( const VkSurfaceCapabilitiesKHR& capabilities ) {
        if( capabilities.currentExtent.width != UINT32_MAX ) {
            return capabilities.currentExtent;
        }
        else {
            int width, height;
            glfwGetFramebufferSize( window, &width, &height );

            VkExtent2D actualExtent = {
                static_cast<uint32_t>( width ),
                static_cast<uint32_t>( height )
            };

            actualExtent.width = std::max( capabilities.minImageExtent.width, std::min( capabilities.maxImageExtent.width, actualExtent.width ) );
            actualExtent.height = std::max( capabilities.minImageExtent.height, std::min( capabilities.maxImageExtent.height, actualExtent.height ) );

            return actualExtent;
        }
    }

    /************************************
     * findQueueFamilies
     ************************************/ 
    QueueFamilyIndices findQueueFamilies( VkPhysicalDevice device ) {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamilyCount, nullptr );

        std::vector<VkQueueFamilyProperties> queueFamilies( queueFamilyCount );
        vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamilyCount, queueFamilies.data() );

        int i = 0;
        for( const auto& queueFamily : queueFamilies ) {
            if( queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT ) {
                indices.graphicsFamily = i;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR( device, i, surface, &presentSupport );

            if( presentSupport ) {
                indices.presentFamily = i;
            }

            if( indices.isComplete() ) break;

            i++;
        }

        return indices;
    }

    /************************************
     * rateDeviceSuitability
     ************************************/ 
    int rateDeviceSuitability( VkPhysicalDevice device ) {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties( device, &deviceProperties );
        //VkPhysicalDeviceFeatures deviceFeatures;
        //vkGetPhysicalDeviceFeatures( device, &deviceFeatures );

        QueueFamilyIndices indices = findQueueFamilies( device );

        int score = 0;

        if( deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ) {
            score += 1000;
        }

        score += deviceProperties.limits.maxImageDimension2D;

        bool extensionsSupported = checkDeviceExtensionSupport( device );

        bool swapChainAdequate = false;

        if( extensionsSupported ) {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport( device );
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        if( !indices.isComplete() && !extensionsSupported && !swapChainAdequate ) {
            return 0;
        }

        return score;
    }

    /************************************
     * checkDeviceExtensionSupport
     ************************************/ 
    bool checkDeviceExtensionSupport( VkPhysicalDevice device ) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties( device, nullptr, &extensionCount, nullptr );

        std::vector<VkExtensionProperties> availableExtensions( extensionCount );
        vkEnumerateDeviceExtensionProperties( device, nullptr, &extensionCount, availableExtensions.data() );

        std::set<std::string> requiredExtensions( deviceExtensions.begin(), deviceExtensions.end() );

        for( const auto& extension : availableExtensions ) {
            requiredExtensions.erase( extension.extensionName );
        }

        return requiredExtensions.empty();
    }

    /************************************
     * pickPhysicalDevice
     ************************************/ 
    void pickPhysicalDevice() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices( instance, &deviceCount, nullptr );

        if( deviceCount == 0 ) {
            throw std::runtime_error( "failed to find GPU with Vulkan support!" );
        }

        std::vector<VkPhysicalDevice> devices( deviceCount );
        vkEnumeratePhysicalDevices( instance, &deviceCount, devices.data() );

        std::multimap<int, VkPhysicalDevice> candidates;

        for( const auto& device : devices ) {
            int score = rateDeviceSuitability( device );
            candidates.insert( std::make_pair( score, device ) );
        }

        if( candidates.rbegin()->first > 0 ) {
            physicalDevice = candidates.rbegin()->second;
        } else {
            throw std::runtime_error( "failed to find a suitable GPU!" );
        }
    }

    /************************************
     * populateDebugMessengerCreateInfo
     ************************************/ 
    void populateDebugMessengerCreateInfo( VkDebugUtilsMessengerCreateInfoEXT& createInfo ) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
        createInfo.pUserData = nullptr;
    }

    /************************************
     * setupDebugMessenger
     ************************************/ 
    void setupDebugMessenger() {
        if( !enableValidationLayers ) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo( createInfo );

        if( CreateDebugUtilsMessengerEXT( instance, &createInfo, nullptr, &debugMessenger ) != VK_SUCCESS ) {
            throw std::runtime_error( "failed to set up debug messenger!" );
        }
    }

    /************************************
     * mainLoop
     ************************************/ 
    void mainLoop() {
        while( !glfwWindowShouldClose( window ) ) {
            glfwPollEvents();
        }
    }

    /************************************
     * cleanup
     ************************************/ 
    void cleanup() {
        for( auto imageView : swapChainImageViews ) {
            vkDestroyImageView( device, imageView, nullptr );
        }

        vkDestroySwapchainKHR( device, swapChain, nullptr );
        vkDestroyDevice( device, nullptr );

        if( enableValidationLayers ) {
            DestroyDebugUtilsMessengerEXT( instance, debugMessenger, nullptr );
        }

        vkDestroySurfaceKHR( instance, surface, nullptr );
        vkDestroyInstance( instance, nullptr );

        glfwDestroyWindow( window );
        glfwTerminate();
    }

    /************************************
     * createInstance
     ************************************/ 
    void createInstance() {
        if( enableValidationLayers && !checkValidationLayerSupport() ) {
            throw std::runtime_error( "validation layers requested, but not available." );
        }
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1,0,0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1,0,0);
        appInfo.apiVersion = VK_API_VERSION_1_2;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        auto extensions = getRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>( extensions.size() );
        createInfo.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;

        if( enableValidationLayers ) {
            createInfo.enabledLayerCount = static_cast<uint32_t>( validationLayers.size() );
            createInfo.ppEnabledLayerNames = validationLayers.data();

            populateDebugMessengerCreateInfo( debugCreateInfo );
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
        }
        else {
            createInfo.enabledLayerCount = 0;

            createInfo.pNext = nullptr;
        }


        if( vkCreateInstance( &createInfo, nullptr, &instance ) != VK_SUCCESS )
            throw std::runtime_error( "failed to create instance!" );
    }

    /************************************
     * checkValidationLayerSupport
     ************************************/ 
    bool checkValidationLayerSupport() {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties( &layerCount, nullptr );

        std::vector<VkLayerProperties> availableLayers( layerCount );
        vkEnumerateInstanceLayerProperties( &layerCount, availableLayers.data() );

        for( const char* layerName : validationLayers ) {
            bool layerFound = false;
            for( const auto &layerProperties : availableLayers ) {
                if( strcmp( layerName, layerProperties.layerName ) == 0 ) {
                    layerFound = true;
                    break;
                }
            }
            if( !layerFound ) return false;
        }
        return true;
    }

    /************************************
     * getRequiredExtensions
     ************************************/ 
    std::vector<const char*> getRequiredExtensions() {
        uint32_t glfwExtensionCount = 0;
        const char **glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions( &glfwExtensionCount );

        std::vector<const char*> extensions( glfwExtensions, glfwExtensions + glfwExtensionCount );

        if( enableValidationLayers ) {
            extensions.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
        }

        return extensions;
    }

    /************************************
     * readFile
     ************************************/ 
    static std::vector<char> readFile( const std::string& filename ) {
        std::ifstream file( filename, std::ios::ate | std::ios::binary );

        if( !file.is_open() ) {
            throw std::runtime_error( "failed to open file!" );
        }

        size_t fileSize = (size_t) file.tellg();
        std::vector<char> buffer( fileSize );

        file.seekg( 0 );
        file.read( buffer.data(), fileSize );

        file.close();
        return buffer;
    }

    /************************************
     * debugCallback
     ************************************/ 
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
                                                        VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, 
                                                        void *pUserData ) {
        if( messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT )
            Log::vlog()->trace( pCallbackData->pMessage );
        else if( messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT )
            Log::vlog()->info( pCallbackData->pMessage );
        else if( messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT )
            Log::log()->warn( pCallbackData->pMessage );
        else if( messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT )
            Log::log()->error( pCallbackData->pMessage );
        return VK_FALSE;
    }
};

int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    } catch( const std::exception& e ) {
        
        Log::log()->critical( e.what() );
        return 1;
    }

    return 0;
}
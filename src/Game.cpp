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

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;

        bool isComplete() {
            return graphicsFamily.has_value();
        }
    };

    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    #ifdef NDEBUG
        const bool enableValidationLayers = false;
    #else
        const bool enableValidationLayers = true;
    #endif

    void initWindow() {
        glfwInit();

        glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
        glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );

        window = glfwCreateWindow( 800, 600, "Vulkan", nullptr, nullptr );
    }

    void initVulkan() {
        createInstance();
        setupDebugMessenger();
    
        pickPhysicalDevice();
        createLogicalDevice();
    }

    void createLogicalDevice() {
        QueueFamilyIndices indices = findQueueFamilies( physicalDevice );

        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
        queueCreateInfo.queueCount = 1;
        float queuePriority = 1.0f;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        VkPhysicalDeviceFeatures deviceFeatures{};

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        createInfo.pQueueCreateInfos = &queueCreateInfo;
        createInfo.queueCreateInfoCount = 1;

        createInfo.pEnabledFeatures = &deviceFeatures;

        if( vkCreateDevice( physicalDevice, &createInfo, nullptr, &device ) != VK_SUCCESS ) {
            throw std::runtime_error( "failed to create logical device!" );
        }

        vkGetDeviceQueue( device, indices.graphicsFamily.value(), 0, &graphicsQueue );
    }

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
                if( indices.isComplete() ) break;
            }

            i++;
        }

        return indices;
    }

    bool rateDeviceSuitability( VkPhysicalDevice device ) {
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

        if( !indices.isComplete() ) {
            return 0;
        }

        return score;
    }

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

    void populateDebugMessengerCreateInfo( VkDebugUtilsMessengerCreateInfoEXT& createInfo ) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
        createInfo.pUserData = nullptr;
    }

    void setupDebugMessenger() {
        if( !enableValidationLayers ) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo( createInfo );

        if( CreateDebugUtilsMessengerEXT( instance, &createInfo, nullptr, &debugMessenger ) != VK_SUCCESS ) {
            throw std::runtime_error( "failed to set up debug messenger!" );
        }
    }

    void mainLoop() {
        while( !glfwWindowShouldClose( window ) ) {
            glfwPollEvents();
        }
    }

    void cleanup() {
        vkDestroyDevice( device, nullptr );

        if( enableValidationLayers ) {
            DestroyDebugUtilsMessengerEXT( instance, debugMessenger, nullptr );
        }
        vkDestroyInstance( instance, nullptr );

        glfwDestroyWindow( window );
        glfwTerminate();
    }

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
#include "Context.h"

#include "Core/Core.h"
#include "Core/Window.h"
#include "Debug/Instrumentor.h"
#include "Renderer/APIs/Vulkan/Helper.h"

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

#include <map>
#include <optional>
#include <vector>

// This anonymous namespace will host all necessary vulkan calls to create the vulkan context
// These function will mostly follow the structure of https://vulkan-tutorial.com
namespace
{
	static const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};
#ifdef MRG_DEBUG
	static constexpr bool enableValidation = true;
#else
	static constexpr bool enableValidation = false;
#endif

	struct QueueFamilyIndices
	{
		std::optional<uint32_t> graphicsFamily;

		[[nodiscard]] bool isComplete() { return graphicsFamily.has_value(); }
	};

	[[nodiscard]] bool checkValidationLayerSupport()
	{
		MRG_ENGINE_INFO("Validation layers requested. Checking availability...");
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const auto layerName : validationLayers) {
			bool layerFound = false;
			for (const auto& layer : availableLayers) {
				if (strcmp(layer.layerName, layerName) == 0) {
					layerFound = true;
					break;
				}
			}
			if (!layerFound) {
				MRG_ENGINE_ERROR("Validation layers missing!");
				return false;
			}
		}
		MRG_ENGINE_INFO("Validation layers found.");
		return true;
	}

	[[nodiscard]] std::vector<const char*> getRequiredExtensions()
	{
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> requiredExtensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if constexpr (enableValidation) {
			requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return requiredExtensions;
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	                                                    VkDebugUtilsMessageTypeFlagsEXT,
	                                                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	                                                    void*)
	{
		switch (messageSeverity) {
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: {
			MRG_ENGINE_TRACE("[VK] {}", pCallbackData->pMessage);
		} break;

		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: {
			MRG_ENGINE_INFO("[VK] {}", pCallbackData->pMessage);
		} break;

		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: {
			MRG_ENGINE_WARN("[VK] {}", pCallbackData->pMessage);
		} break;

		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: {
			MRG_ENGINE_ERROR("[VK] {}", pCallbackData->pMessage);
		} break;

		default: {
			MRG_ENGINE_FATAL("[VK] {}", pCallbackData->pMessage);
		} break;
		}

		return VK_FALSE;
	}

	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		                             VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		                         VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
	}

	[[nodiscard]] VkInstance createInstance(const char* appName)
	{
		// this if statement is split to allow constexpr if
		if constexpr (enableValidation) {
			if (!checkValidationLayerSupport())
				throw std::runtime_error("Validation layers requested but not found!");
		}

		VkInstance returnInstance;

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pEngineName = "Morrigu";
		appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
		appInfo.pApplicationName = appName;
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);  // TODO: give option to create appropriate versions
		appInfo.apiVersion = VK_API_VERSION_1_0;                // TODO: check if update may be necessary

		const auto requiredExtensions = getRequiredExtensions();

		uint32_t supportedExtensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtensionCount, nullptr);
		std::vector<VkExtensionProperties> supportedExtensions(supportedExtensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtensionCount, supportedExtensions.data());

		MRG_ENGINE_INFO("The following {} vulkan extensions are required:", requiredExtensions.size());
		for (std::size_t i = 0; i < requiredExtensions.size(); ++i) {
			bool isSupported = false;
			for (uint32_t j = 0; j < supportedExtensionCount; ++j) {
				if (strcmp(supportedExtensions[j].extensionName, requiredExtensions[i]) == 0) {
					MRG_ENGINE_INFO(
					  "\t{} (version {}) [SUPPORTED]", supportedExtensions[j].extensionName, supportedExtensions[j].specVersion);
					supportedExtensions.erase(supportedExtensions.begin() + j);
					--j;  // might underflow if set to 0, but will overflow back to 0
					isSupported = true;
					break;
				}
			}
			if (!isSupported) {
				MRG_ENGINE_ERROR("\t{} (NO VERSION FOUND) [NOT SUPPORTED]", requiredExtensions[i]);
				MRG_ASSERT(false, "Required extension not supported by hardware!");
			}
		}
		MRG_ENGINE_INFO("All required extensions have been found.");
		MRG_ENGINE_TRACE("Additional supported extensions ({}): ", supportedExtensions.size());
		for (const auto& extension : supportedExtensions) {
			MRG_ENGINE_TRACE("\t{} (version {})", extension.extensionName, extension.specVersion);
		}

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
		createInfo.ppEnabledExtensionNames = requiredExtensions.data();
		if constexpr (enableValidation) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();

			populateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = &debugCreateInfo;
		} else {
			createInfo.enabledLayerCount = 0;
		}

		MRG_VKVALIDATE(vkCreateInstance(&createInfo, nullptr, &returnInstance), "failed to create instance!");
		return returnInstance;
	}

	[[nodiscard]] VkResult createDebugUtilsMessengerEXT(VkInstance instance,
	                                                    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	                                                    const VkAllocationCallbacks* pAllocator,
	                                                    VkDebugUtilsMessengerEXT* pDebugMessenger)
	{
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr)
			return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}

	void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks* pAllocator)
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr)
			func(instance, messenger, pAllocator);
	}

	[[nodiscard]] VkDebugUtilsMessengerEXT setupDebugMessenger(VkInstance instance)
	{
		VkDebugUtilsMessengerEXT returnMessenger;

		VkDebugUtilsMessengerCreateInfoEXT createInfo{};
		populateDebugMessengerCreateInfo(createInfo);

		MRG_VKVALIDATE(createDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &returnMessenger), "failed to setup messenger!");
		return returnMessenger;
	}

	[[nodiscard]] QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const auto& queueFamily : queueFamilies) {
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				indices.graphicsFamily = i;

			if (indices.isComplete())
				break;
			++i;
		}

		return indices;
	}

	[[nodiscard]] std::size_t evaluateDevice(VkPhysicalDevice device)
	{
		VkPhysicalDeviceProperties properties;
		VkPhysicalDeviceFeatures features;
		std::size_t score{};

		vkGetPhysicalDeviceProperties(device, &properties);
		vkGetPhysicalDeviceFeatures(device, &features);

		if (features.geometryShader == VK_FALSE)
			return 0;

		if (!findQueueFamilies(device).isComplete())
			return 0;

		// Favor discrete GPUs over anything else
		if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			score += 10000;  // This value is completely arbitrary, and subject to change

		score += properties.limits.maxFramebufferHeight;
		score += properties.limits.maxFramebufferWidth;
		return score;
	}

	constexpr const char* vendorStringfromID(uint32_t vendorID)
	{
		switch (vendorID) {
		case 0x1002:
			return "AMD";
		case 0x1010:
			return "ImgTec";
		case 0x10DE:
			return "NVIDIA";
		case 0x13B5:
			return "ARM";
		case 0x5143:
			return "Qualcomm";
		case 0x8086:
			return "Intel";

		default:
			return "Unknown vendor";
		}
	}

	[[nodiscard]] VkPhysicalDevice pickPhysicalDevice(VkInstance instance)
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

		if (deviceCount == 0)
			throw std::runtime_error("no available GPUs available for Vulkan!");

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		std::multimap<std::size_t, VkPhysicalDevice> candidates;

		VkPhysicalDeviceProperties props;
		MRG_ENGINE_TRACE("{} physical devices found:", deviceCount);
		for (const auto& device : devices) {
			vkGetPhysicalDeviceProperties(device, &props);
			MRG_ENGINE_TRACE("\t{} {{ID{}}} ({})", props.deviceName, props.deviceID, vendorStringfromID(props.vendorID));
			candidates.emplace(evaluateDevice(device), device);
		}

		// It's possible that no suitable devices have been found
		if (candidates.rbegin()->first == 0)
			throw std::runtime_error("no suitable GPU found!");

		return candidates.rbegin()->second;
	}

	[[nodiscard]] VkDevice createDevice(VkPhysicalDevice physicalDevice)
	{
		VkDevice returnDevice;

		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
		float queuePriority = 1.f;

		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
		queueCreateInfo.pQueuePriorities = &queuePriority;

		// TODO: Come back to this to select advanced device features
		VkPhysicalDeviceFeatures deviceFeatures{};
		deviceFeatures.geometryShader = VK_TRUE;

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount = 1;
		createInfo.pQueueCreateInfos = &queueCreateInfo;
		if (enableValidation) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		} else {
			createInfo.enabledLayerCount = 0;
		}

		MRG_VKVALIDATE(vkCreateDevice(physicalDevice, &createInfo, nullptr, &returnDevice), "failed to create device!");

		return returnDevice;
	}

}  // namespace

namespace MRG::Vulkan
{
	Context::Context(GLFWwindow* window)
	{
		MRG_PROFILE_FUNCTION();

		MRG_CORE_ASSERT(window, "Window handle is null!");

		MRG_ENGINE_INFO("Using Vulkan as a rendering API. Other useful stats will follow:");

		try {
			auto data = static_cast<WindowProperties*>(glfwGetWindowUserPointer(window));
			m_instance = createInstance(data->title.c_str());

			if constexpr (enableValidation)
				m_messenger = setupDebugMessenger(m_instance);

			MRG_VKVALIDATE(glfwCreateWindowSurface(m_instance, window, nullptr, &m_surface), "failed to create window surface!");

			m_physicalDevice = pickPhysicalDevice(m_instance);

			VkPhysicalDeviceProperties props;
			vkGetPhysicalDeviceProperties(m_physicalDevice, &props);
			MRG_ENGINE_INFO(
			  "Physical device selected: {} {{ID{}}} ({})", props.deviceName, props.deviceID, vendorStringfromID(props.vendorID));

			m_device = createDevice(m_physicalDevice);
			auto queueFamilies = findQueueFamilies(m_physicalDevice);
			vkGetDeviceQueue(m_device, queueFamilies.graphicsFamily.value(), 0, &m_graphicsQueue);
		} catch (std::runtime_error e) {
			MRG_ENGINE_ERROR("Vulkan error detected: {}", e.what());
		}
	}

	Context::~Context()
	{
		vkDestroyDevice(m_device, nullptr);

		if constexpr (enableValidation)
			destroyDebugUtilsMessengerEXT(m_instance, m_messenger, nullptr);

		vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
		vkDestroyInstance(m_instance, nullptr);
	}

	void Context::swapBuffers()
	{
		// MRG_PROFILE_FUNCTION();
	}

	void Context::swapInterval(int) {}
}  // namespace MRG::Vulkan
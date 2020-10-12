#ifndef MRG_VKHELPER
#define MRG_VKHELPER

#include <vulkan/vulkan.hpp>

#include <optional>
#include <stdexcept>

#define MRG_VKVALIDATE(expression, failure_message)                                                                                        \
	if (expression != VK_SUCCESS) {                                                                                                        \
		throw std::runtime_error(failure_message);                                                                                         \
	}

namespace MRG::Vulkan
{
	struct QueueFamilyIndices
	{
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		[[nodiscard]] bool isComplete() { return graphicsFamily.has_value() && presentFamily.has_value(); }
	};

	struct SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	[[nodiscard]] SwapChainSupportDetails querySwapChainSupport(const VkPhysicalDevice device, const VkSurfaceKHR surface);
}  // namespace MRG::Vulkan

#endif
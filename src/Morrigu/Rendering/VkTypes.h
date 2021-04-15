//
// Created by mathi on 2021-04-11.
//

#ifndef MORRIGU_VKTYPES_H
#define MORRIGU_VKTYPES_H

#include "Core/Core.h"

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>

#include <exception>

#define MRG_VK_CHECK(expression, error_message)                                                                                            \
	{                                                                                                                                      \
		if ((expression) != vk::Result::eSuccess) { throw std::runtime_error(error_message); }                                                       \
	}

#endif  // MORRIGU_VKTYPES_H

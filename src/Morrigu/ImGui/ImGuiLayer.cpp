#include "ImGuiLayer.h"

#include "Core/Application.h"
#include "Debug/Instrumentor.h"
#include "Renderer/RenderingAPI.h"

#include "Renderer/APIs/Vulkan/Helper.h"
#include "Renderer/APIs/Vulkan/WindowProperties.h"
#include "Renderer/Renderer2D.h"

#include <ImGui/bindings/imgui_impl_glfw.h>
#include <ImGui/bindings/imgui_impl_opengl3.h>
#include <ImGui/bindings/imgui_impl_vulkan.h>
#include <imgui.h>

namespace MRG
{
	ImGuiLayer::ImGuiLayer() : Layer{"ImGui Layer"} {}

	void ImGuiLayer::onAttach()
	{
		MRG_PROFILE_FUNCTION();

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		auto& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		io.Fonts->AddFontFromFileTTF("engine/fonts/opensans/bold.ttf", 18.f);
		io.FontDefault = io.Fonts->AddFontFromFileTTF("engine/fonts/opensans/regular.ttf", 18.f);

		ImGui::StyleColorsDark();

		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			style.WindowRounding = 0.f;
			style.Colors[ImGuiCol_WindowBg].w = 1.f;
		}

		setDarkThemeColors();

		const auto window = Application::get().getWindow().getGLFWWindow();

		switch (RenderingAPI::getAPI()) {
		case RenderingAPI::API::OpenGL: {
			ImGui_ImplGlfw_InitForOpenGL(window, true);
			ImGui_ImplOpenGL3_Init("#version 460 core");
		} break;

		case RenderingAPI::API::Vulkan: {
			const auto data = static_cast<Vulkan::WindowProperties*>(glfwGetWindowUserPointer(window));

			VkDescriptorPoolSize pool_sizes[] = {{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
			                                     {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
			                                     {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
			                                     {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
			                                     {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
			                                     {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
			                                     {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
			                                     {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
			                                     {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
			                                     {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
			                                     {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

			VkDescriptorPoolCreateInfo pool_info = {};
			pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
			pool_info.maxSets = 1000;
			pool_info.poolSizeCount = static_cast<uint32_t>(std::size(pool_sizes));
			pool_info.pPoolSizes = pool_sizes;

			MRG_VKVALIDATE(vkCreateDescriptorPool(data->device, &pool_info, nullptr, &data->ImGuiPool),
			               "failed to create imgui descriptor pool!");

			ImGui_ImplGlfw_InitForVulkan(window, true);

			ImGui_ImplVulkan_InitInfo init_info = {};
			init_info.Instance = data->instance;
			init_info.PhysicalDevice = data->physicalDevice;
			init_info.Device = data->device;
			init_info.Queue = data->graphicsQueue.handle;
			init_info.DescriptorPool = data->ImGuiPool;
			init_info.MinImageCount = data->swapChain.minImageCount;
			init_info.ImageCount = data->swapChain.imageCount;

			ImGui_ImplVulkan_Init(&init_info, data->ImGuiPipeline.renderPass);

			auto commandBuffer = Vulkan::beginSingleTimeCommand(data);
			ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
			Vulkan::endSingleTimeCommand(data, commandBuffer);

			ImGui_ImplVulkan_DestroyFontUploadObjects();
		} break;

		case RenderingAPI::API::None:
		default: {
			MRG_CORE_ASSERT(false, fmt::format("UNSUPPORTED RENDERER API OPTION ! ({})", RenderingAPI::getAPI()));
		} break;
		}
	}

	void ImGuiLayer::onDetach()
	{
		MRG_PROFILE_FUNCTION();

		switch (RenderingAPI::getAPI()) {
		case RenderingAPI::API::OpenGL: {
			ImGui_ImplOpenGL3_Shutdown();
		} break;

		case RenderingAPI::API::Vulkan: {
			const auto data = static_cast<Vulkan::WindowProperties*>(glfwGetWindowUserPointer(Renderer2D::getGLFWWindow()));

			vkDestroyDescriptorPool(data->device, data->ImGuiPool, nullptr);
			ImGui_ImplVulkan_Shutdown();
		} break;

		case RenderingAPI::API::None:
		default: {
			MRG_CORE_ASSERT(false, fmt::format("UNSUPPORTED RENDERER API OPTION ! ({})", RenderingAPI::getAPI()));
		} break;
		}

		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void ImGuiLayer::onEvent(Event& event)
	{
		if (m_blockEvents) {
			auto& io = ImGui::GetIO();
			event.handled |= event.isInCategory(EventCategoryMouse) & io.WantCaptureMouse;
			event.handled |= event.isInCategory(EventCategoryKeyboard) & io.WantCaptureKeyboard;
		}
	}

	void ImGuiLayer::begin()
	{
		MRG_PROFILE_FUNCTION();

		switch (RenderingAPI::getAPI()) {
		case RenderingAPI::API::OpenGL: {
			m_renderTarget = Renderer2D::getRenderTarget();
			Renderer2D::resetRenderTarget();
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
		} break;

		case RenderingAPI::API::Vulkan: {
			ImGui_ImplVulkan_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
		} break;

		case RenderingAPI::API::None:
		default: {
			MRG_CORE_ASSERT(false, fmt::format("UNSUPPORTED RENDERER API OPTION ! ({})", RenderingAPI::getAPI()));
		} break;
		}
	}

	void ImGuiLayer::end()
	{
		MRG_PROFILE_FUNCTION();

		auto& io = ImGui::GetIO();
		auto& app = Application::get();
		io.DisplaySize = ImVec2{static_cast<float>(app.getWindow().getWidth()), static_cast<float>(app.getWindow().getHeight())};

		switch (RenderingAPI::getAPI()) {
		case RenderingAPI::API::OpenGL: {
			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

			if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
				const auto contextBkp = glfwGetCurrentContext();
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
				glfwMakeContextCurrent(contextBkp);
			}
			Renderer2D::setRenderTarget(m_renderTarget);
		} break;

		case RenderingAPI::API::Vulkan: {
			ImGui::Render();
		} break;

		case RenderingAPI::API::None:
		default: {
			MRG_CORE_ASSERT(false, fmt::format("UNSUPPORTED RENDERER API OPTION ! ({})", RenderingAPI::getAPI()));
		} break;
		}
	}

	void ImGuiLayer::setDarkThemeColors()
	{
		auto& colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_WindowBg] = ImVec4{0.1f, 0.105f, 0.11f, 1.f};

		// Headers
		colors[ImGuiCol_Header] = ImVec4{0.2f, 0.205f, 0.21f, 1.f};
		colors[ImGuiCol_HeaderHovered] = ImVec4{0.3f, 0.305f, 0.31f, 1.f};
		colors[ImGuiCol_HeaderActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.f};

		// Buttons
		colors[ImGuiCol_Button] = ImVec4{0.2f, 0.205f, 0.21f, 1.f};
		colors[ImGuiCol_ButtonHovered] = ImVec4{0.3f, 0.305f, 0.31f, 1.f};
		colors[ImGuiCol_ButtonActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.f};

		// Frame BG
		colors[ImGuiCol_FrameBg] = ImVec4{0.2f, 0.205f, 0.21f, 1.f};
		colors[ImGuiCol_FrameBgHovered] = ImVec4{0.3f, 0.305f, 0.31f, 1.f};
		colors[ImGuiCol_FrameBgActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.f};

		// Tabs
		colors[ImGuiCol_Tab] = ImVec4{0.15f, 0.1505f, 0.151f, 1.f};
		colors[ImGuiCol_TabHovered] = ImVec4{0.38f, 0.3805f, 0.381f, 1.f};
		colors[ImGuiCol_TabActive] = ImVec4{0.28f, 0.2805f, 0.281f, 1.f};
		colors[ImGuiCol_TabUnfocused] = ImVec4{0.15f, 0.1505f, 0.151f, 1.f};
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4{0.2f, 0.205f, 0.21f, 1.f};

		// Title
		colors[ImGuiCol_TitleBg] = ImVec4{0.15f, 0.1505f, 0.151f, 1.f};
		colors[ImGuiCol_TitleBgActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.f};
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4{0.15f, 0.1505f, 0.151f, 1.f};
	}
}  // namespace MRG

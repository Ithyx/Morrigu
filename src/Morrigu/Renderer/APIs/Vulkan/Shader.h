#ifndef MRG_VULKAN_IMPL_SHADER
#define MRG_VULKAN_IMPL_SHADER

#include "Renderer/APIs/Vulkan/VulkanHPPIncludeHelper.h"
#include "Renderer/Shader.h"

#include <cstdint>

namespace MRG::Vulkan
{
	class Shader : public MRG::Shader
	{
	public:
		explicit Shader(const std::string& filePath);
		Shader(const Shader&) = delete;
		Shader(Shader&&) = delete;
		~Shader() override;

		Shader& operator=(const Shader&) = delete;
		Shader& operator=(Shader&&) = delete;

		void destroy() override;

		void bind() const override;
		void unbind() const override;

		void upload(const std::string& name, int value) override;
		void upload(const std::string& name, int* value, std::size_t count) override;
		void upload(const std::string& name, float value) override;
		void upload(const std::string& name, const glm::vec3& value) override;
		void upload(const std::string& name, const glm::vec4& value) override;
		void upload(const std::string& name, const glm::mat4& value) override;

		[[nodiscard]] const std::string& getName() const override { return m_name; }

		VkShaderModule m_vertexShaderModule, m_fragmentShaderModule;

	private:
		std::string m_name;
	};
}  // namespace MRG::Vulkan

#endif

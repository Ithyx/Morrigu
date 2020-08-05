#ifndef MRG_OPENGL_IMPL_SHADER
#define MRG_OPENGL_IMPL_SHADER

#include "Renderer/Shader.h"

#include <glm/glm.hpp>

#include <cstdint>

namespace MRG::OpenGL
{
	class Shader : public MRG::Shader
	{
	public:
		Shader(const std::string& vertexSrc, const std::string& fragmentSrc);
		virtual ~Shader();

		void bind() const override;
		void unbind() const override;

		void uploadUniform(const std::string& name, int value);

		void uploadUniform(const std::string& name, float value);
		void uploadUniform(const std::string& name, const glm::vec2& value);
		void uploadUniform(const std::string& name, const glm::vec3& value);
		void uploadUniform(const std::string& name, const glm::vec4& value);

		void uploadUniform(const std::string& name, const glm::mat3& matrix);
		void uploadUniform(const std::string& name, const glm::mat4& matrix);

	private:
		uint32_t m_rendererID;
	};
}  // namespace MRG::OpenGL

#endif
#include "VertexArray.h"

#include "Debug/Instrumentor.h"

#include <glad/glad.h>

namespace
{
	[[nodiscard]] GLenum ShaderDataTypeToOpenGLBaseType(MRG::ShaderDataType type)
	{
		// clang-format off
		switch (type)
		{
			case MRG::ShaderDataType::Float:
			case MRG::ShaderDataType::Float2:
			case MRG::ShaderDataType::Float3:
			case MRG::ShaderDataType::Float4:
			case MRG::ShaderDataType::Mat3:
			case MRG::ShaderDataType::Mat4:   return GL_FLOAT;

			case MRG::ShaderDataType::Int:
			case MRG::ShaderDataType::Int2:
			case MRG::ShaderDataType::Int3:
			case MRG::ShaderDataType::Int4:   return GL_INT;

			case MRG::ShaderDataType::UInt:
			case MRG::ShaderDataType::UInt2:
			case MRG::ShaderDataType::UInt3:
			case MRG::ShaderDataType::UInt4:   return GL_UNSIGNED_INT;

			case MRG::ShaderDataType::Bool:   return GL_BOOL;

			case MRG::ShaderDataType::None:
			default: {
				MRG_CORE_ASSERT(false, fmt::format("Invalid shader data type! ({})", type))
				return GL_INVALID_ENUM;
			}
		}
		// clang-format on
	}
}  // namespace

namespace MRG::OpenGL
{
	VertexArray::VertexArray()
	{
		MRG_PROFILE_FUNCTION()

		glCreateVertexArrays(1, &m_rendererID);
	}
	VertexArray::~VertexArray()
	{
		MRG_PROFILE_FUNCTION()

		VertexArray::destroy();
	}

	void VertexArray::bind() const
	{
		MRG_PROFILE_FUNCTION()

		glBindVertexArray(m_rendererID);
	}
	void VertexArray::unbind() const
	{
		MRG_PROFILE_FUNCTION()

		glBindVertexArray(0);
	}

	void VertexArray::addVertexBuffer(const Ref<MRG::VertexBuffer>& vertexBuffer)
	{
		MRG_PROFILE_FUNCTION()

		MRG_CORE_ASSERT(!vertexBuffer->layout.getElements().empty(), "Vertex Buffer layout has not been set!")

		glBindVertexArray(m_rendererID);
		vertexBuffer->bind();

		auto index = 0;
		const auto& layout = vertexBuffer->layout;
		for (const auto& element : layout) {
			glEnableVertexAttribArray(index);
			glVertexAttribPointer(index,
			                      element.getComponentCount(),
			                      ShaderDataTypeToOpenGLBaseType(element.type),
			                      element.isNormalized ? GL_TRUE : GL_FALSE,
			                      layout.getStride(),
			                      (const void*)(element.offset));
			++index;
		}

		m_vertexBuffers.emplace_back(vertexBuffer);
	}

	void VertexArray::setIndexBuffer(const Ref<MRG::IndexBuffer>& indexBuffer)
	{
		MRG_PROFILE_FUNCTION()

		glBindVertexArray(m_rendererID);
		indexBuffer->bind();

		m_indexBuffer = indexBuffer;
	}
}  // namespace MRG::OpenGL

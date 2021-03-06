#ifndef MRG_CLASSES_BUFFERS
#define MRG_CLASSES_BUFFERS

#include "Core/Core.h"

#include <cstdint>

namespace MRG
{
	// clang-format off
	enum class ShaderDataType
	{
		None = 0,
		Float, Float2, Float3, Float4,
		Mat3, Mat4,
		Int, Int2, Int3, Int4,
		UInt, UInt2, UInt3, UInt4,
		Bool
	};

	[[nodiscard]] static uint32_t ShaderDataTypeSize(ShaderDataType type)
	{
		switch (type)
		{
		case ShaderDataType::Float:  return 4 * 1;
		case ShaderDataType::Float2: return 4 * 2;
		case ShaderDataType::Float3: return 4 * 3;
		case ShaderDataType::Float4: return 4 * 4;

		case ShaderDataType::Mat3:   return 4 * 3 * 3;
		case ShaderDataType::Mat4:   return 4 * 4 * 4;

		case ShaderDataType::Int:    return 4 * 1;
		case ShaderDataType::Int2:   return 4 * 2;
		case ShaderDataType::Int3:   return 4 * 3;
		case ShaderDataType::Int4:   return 4 * 4;

		case ShaderDataType::UInt:    return 4 * 1;
		case ShaderDataType::UInt2:   return 4 * 2;
		case ShaderDataType::UInt3:   return 4 * 3;
		case ShaderDataType::UInt4:   return 4 * 4;

		case ShaderDataType::Bool:   return 1;

		case ShaderDataType::None:
		default: {
			MRG_CORE_ASSERT(false, fmt::format("Invalid shader data type! ({})", type))
			return 0;
		}
		}

	}
	// clang-format on

	struct BufferElement
	{
		std::string name;
		ShaderDataType type{ShaderDataType::None};
		uint32_t size{0};
		std::size_t offset{0};
		bool isNormalized{false};

		BufferElement() = default;
		BufferElement(ShaderDataType bufferType, const char* bufferName, bool normalized = false)
		    : name(bufferName), type(bufferType), size(ShaderDataTypeSize(bufferType)), isNormalized(normalized)
		{}

		[[nodiscard]] uint32_t getComponentCount() const
		{
			// clang-format off
			switch (type)
			{
			case ShaderDataType::Float:  return 1;
			case ShaderDataType::Float2: return 2;
			case ShaderDataType::Float3: return 3;
			case ShaderDataType::Float4: return 4;

			case ShaderDataType::Mat3:   return 3; // This is returning 3 Float3
			case ShaderDataType::Mat4:   return 4; // This is returning 4 Float4

			case ShaderDataType::Int:    return 1;
			case ShaderDataType::Int2:   return 2;
			case ShaderDataType::Int3:   return 3;
			case ShaderDataType::Int4:   return 4;

			case ShaderDataType::UInt:    return 1;
			case ShaderDataType::UInt2:   return 2;
			case ShaderDataType::UInt3:   return 3;
			case ShaderDataType::UInt4:   return 4;

			case ShaderDataType::Bool:   return 1;

			case ShaderDataType::None:
			default: {
				MRG_CORE_ASSERT(false, fmt::format("Invalid shader data type! ({})", type))
				return 0;
			}
			}
			// clang-format on
		}
	};

	class BufferLayout
	{
	public:
		BufferLayout() = default;
		BufferLayout(const std::initializer_list<BufferElement> elements) : m_elements(elements)
		{
			std::size_t offset = 0;
			for (auto& element : m_elements) {
				element.offset = offset;
				m_stride += element.size;
				offset += element.size;
			}
		}

		[[nodiscard]] auto getStride() const { return m_stride; }
		[[nodiscard]] auto getElements() const { return m_elements; }

		[[nodiscard]] auto begin() const { return m_elements.begin(); }
		[[nodiscard]] auto end() const { return m_elements.end(); }

	private:
		std::vector<BufferElement> m_elements;
		uint32_t m_stride = 0;
	};

	class VertexBuffer
	{
	public:
		VertexBuffer(const VertexBuffer&) = delete;
		VertexBuffer(VertexBuffer&&) = delete;
		virtual ~VertexBuffer() = default;

		VertexBuffer& operator=(const VertexBuffer&) = delete;
		VertexBuffer& operator=(VertexBuffer&&) = delete;

		virtual void destroy() = 0;

		virtual void bind() const = 0;
		virtual void unbind() const = 0;

		virtual void setData(const void* data, uint32_t size) = 0;

		BufferLayout layout;

		[[nodiscard]] static Ref<VertexBuffer> create(uint32_t size);
		[[nodiscard]] static Ref<VertexBuffer> create(const void* vertices, uint32_t size);

	protected:
		VertexBuffer() = default;
		bool m_isDestroyed = false;
	};

	// 32-bits Index buffers only for now
	class IndexBuffer
	{
	public:
		IndexBuffer(const IndexBuffer&) = delete;
		IndexBuffer(IndexBuffer&&) = delete;
		virtual ~IndexBuffer() = default;

		IndexBuffer& operator=(const IndexBuffer&) = delete;
		IndexBuffer& operator=(IndexBuffer&&) = delete;

		virtual void destroy() = 0;

		virtual void bind() const = 0;
		virtual void unbind() const = 0;
		[[nodiscard]] virtual uint32_t getCount() const = 0;

		[[nodiscard]] static Ref<IndexBuffer> create(const uint32_t* indices, uint32_t count);

	protected:
		IndexBuffer() = default;
		bool m_isDestroyed = false;
	};
}  // namespace MRG

#endif

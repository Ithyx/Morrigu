#ifndef MRG_CLASS_SHADER
#define MRG_CLASS_SHADER

#include "Core/Core.h"

#include <string>

namespace MRG
{
	enum class Encoding
	{
		LF = 0,
		CRLF,
		CR
	};

	class Shader
	{
	public:
		virtual ~Shader() = default;

		virtual void bind() const = 0;
		virtual void unbind() const = 0;

		[[nodiscard]] static Ref<Shader> create(const std::string& vertexSrc, const std::string& fragmentSrc);
		[[nodiscard]] static Ref<Shader> create(const std::string& filePath, Encoding encoding = Encoding::LF);
	};
}  // namespace MRG

#endif
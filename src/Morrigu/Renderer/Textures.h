#ifndef MRG_CLASSES_TEXTURES
#define MRG_CLASSES_TEXTURES

#include "Core/Core.h"

#include <string>

namespace MRG
{
	class Texture
	{
	public:
		virtual ~Texture() = default;

		[[nodiscard]] virtual uint32_t getWidth() const = 0;
		[[nodiscard]] virtual uint32_t getHeight() const = 0;

		virtual void bind(uint32_t slot = 0) const = 0;
	};

	class Texture2D : public Texture
	{
	public:
		[[nodiscard]] static Ref<Texture2D> create(const std::string& path);
	};
}  // namespace MRG

#endif
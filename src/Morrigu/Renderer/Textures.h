#ifndef MRG_CLASSES_TEXTURES
#define MRG_CLASSES_TEXTURES

#include "Core/Core.h"

#include <imgui.h>

#include <string>

namespace MRG
{
	class Texture
	{
	public:
		Texture(const Texture&) = delete;
		Texture(Texture&&) = delete;
		virtual ~Texture() = default;

		Texture& operator=(const Texture&) = delete;
		Texture& operator=(Texture&&) = delete;

		virtual void destroy() = 0;

		virtual bool operator==(const Texture& other) const = 0;

		[[nodiscard]] virtual uint32_t getWidth() const = 0;
		[[nodiscard]] virtual uint32_t getHeight() const = 0;
		[[nodiscard]] virtual ImTextureID getImTextureID() = 0;

		virtual void setData(void* data, uint32_t size) = 0;

		virtual void bind(uint32_t slot) const = 0;

	protected:
		Texture() = default;
		bool m_isDestroyed = false;
	};

	class Texture2D : public Texture
	{
	public:
		[[nodiscard]] static Ref<Texture2D> create(uint32_t width, uint32_t height);
		[[nodiscard]] static Ref<Texture2D> create(const std::string& path);
	};
}  // namespace MRG

#endif

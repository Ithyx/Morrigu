#ifndef MRG_CLASS_TIMESTEP
#define MRG_CLASS_TIMESTEP

namespace MRG
{
	class Timestep
	{
	public:
		Timestep(float time = 0.f) : m_time(time) {}  // NOLINT (explicit)

		operator float() const { return m_time; }  // NOLINT (explicit)

		[[nodiscard]] float getSeconds() const { return m_time; }
		[[nodiscard]] float getMillieconds() const { return m_time * 1000; }

	private:
		float m_time;
	};
}  // namespace MRG

#endif

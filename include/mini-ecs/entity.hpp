#pragma once

#include <cstdint>

namespace me {

	class Registry;

	namespace entity {
		using entity_id = std::uint32_t;
		using version = std::uint32_t;
		constexpr entity_id null = 0;
	}

	class Entity {
	public:
		Entity() = default;
		Entity(entity::entity_id id, entity::version generation, Registry* registry);

		bool is_valid() const;

		// Implicit conversion to ID
		operator entity::entity_id() const { return m_id; }
		bool operator==(const Entity& other) const { return m_id == other.m_id && m_registry == other.m_registry; }
		bool operator!=(const Entity& other) const { return !(*this == other); }

		// --- Object-Oriented Component API ---
		template <typename T> void add_component(const T& component);
		template <typename T> T* try_get_component();
		template <typename T> T& get_component();
		template <typename T> bool has_component();
		template <typename T> void remove_component();
		void destroy();

		entity::entity_id get_id() const {
			return m_id;
		}

	private:
		entity::entity_id m_id = entity::null;
		entity::version m_generation = 0;
		Registry* m_registry = nullptr;
	};

} // namespace me

#pragma once

#include <cstdint>

namespace me {

	namespace entity {
		using entity_id = std::uint32_t;
		constexpr entity_id null = 0;
	}

	// Lightweight handle - just an ID.
	// No longer holds Registry pointer for safety.
	struct Entity {
		entity::entity_id id = entity::null;

		Entity() = default;
		Entity(entity::entity_id id) : id(id) {}

		bool is_valid() const { return id != entity::null; }

		// Implicit conversion to ID
		operator entity::entity_id() const { return id; }

		bool operator==(const Entity& other) const { return id == other.id; }
		bool operator!=(const Entity& other) const { return id != other.id; }
	};

} // namespace me
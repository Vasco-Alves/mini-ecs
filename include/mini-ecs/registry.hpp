#pragma once

#include "mini-ecs/entity.hpp"
#include "mini-ecs/component_pool.hpp"
#include "mini-ecs/view.hpp"
#include <unordered_map>
#include <typeindex>
#include <memory>
#include <vector>
#include <functional>
#include <cassert>

namespace me {

	namespace detail {
		struct EntityRecord {
			bool alive = true;
			bool marked_for_deletion = false;
			entity::version generation = 0;
		};
	}

	class Registry {
	public:
		Entity create_entity() {
			// Reuse a recycled id if one is available. Bumping its generation
			// invalidates any stale handles that still point at the old entity.
			if (!m_free_ids.empty()) {
				entity::entity_id id = m_free_ids.back();
				m_free_ids.pop_back();

				auto& record = m_entities[id];
				record.alive = true;
				record.marked_for_deletion = false;
				++record.generation;

				return Entity(id, record.generation, this);
			}

			entity::entity_id id = m_nextId++;
			m_entities[id] = { true, false, 0 };

			// Pass 'this' so the entity knows which Registry owns it
			return Entity(id, 0, this);
		}

		// Rebuilds an Entity handle for an existing id, stamping it with the
		// current generation so its validity tracks whatever entity now owns
		// that id. Returns a null (invalid) handle if the id was never created.
		// Handy for turning the raw id from a view back into a handle.
		Entity get_entity(entity::entity_id e) {
			auto it = m_entities.find(e);
			if (it == m_entities.end()) return Entity{};
			return Entity(e, it->second.generation, this);
		}

		// =====================================================================
		// DEFERRED DELETION PIPELINE
		// =====================================================================

		// Systems call this. It safely defers deletion until the end of a loop.
		void destroy_entity(entity::entity_id e) {
			auto it = m_entities.find(e);
			if (it != m_entities.end() && it->second.alive && !it->second.marked_for_deletion) {
				it->second.marked_for_deletion = true;
				m_entities_to_delete.push_back(e);
			}
		}

		void process_deletions(std::function<void(entity::entity_id)> pre_delete_callback = nullptr) {
			// Move the kill list aside first: a pre_delete_callback may itself
			// queue new deletions, and mutating the vector we are iterating
			// would invalidate it. Anything queued during this pass is handled
			// on the next call to process_deletions.
			std::vector<entity::entity_id> to_delete;
			to_delete.swap(m_entities_to_delete);

			for (auto e : to_delete) {
				if (pre_delete_callback) {
					pre_delete_callback(e);
				}
				destroy_entity_immediate(e);
			}
		}

		void destroy_entity_immediate(entity::entity_id e) {
			auto it = m_entities.find(e);
			if (it != m_entities.end() && it->second.alive) {
				it->second.alive = false;
				it->second.marked_for_deletion = false;
				remove_entity_from_pools(e);
				m_free_ids.push_back(e); // Recycle the id for a future entity
			}
		}

		bool is_alive(entity::entity_id e) const {
			auto it = m_entities.find(e);
			// Hide entities the moment they are marked for deletion.
			return (it != m_entities.end()) && it->second.alive && !it->second.marked_for_deletion;
		}

		// Generation-aware check used by Entity handles. Returns false if the
		// id has since been recycled into a different entity.
		bool is_valid_handle(entity::entity_id e, entity::version generation) const {
			auto it = m_entities.find(e);
			return (it != m_entities.end())
				&& it->second.alive
				&& !it->second.marked_for_deletion
				&& it->second.generation == generation;
		}

		void clear() {
			m_entities.clear();
			m_entities_to_delete.clear(); // Clear the kill list too
			m_free_ids.clear();           // ...and the recycle list
			for (auto& pair : m_pools) {
				pair.second->clear();
			}
			m_nextId = 1;
		}

		// ---------------------------------------------------------------------
		// Component Management
		// ---------------------------------------------------------------------

		template <typename T>
		void add_component(entity::entity_id e, const T& component) {
			get_pool<T>()->add(e, component);
		}

		template <typename T>
		T* try_get_component(entity::entity_id e) {
			return get_pool<T>()->try_get(e);
		}

		template <typename T>
		bool has_component(entity::entity_id e) {
			return get_pool<T>()->has(e);
		}

		template <typename T>
		void remove_component(entity::entity_id e) {
			get_pool<T>()->remove(e);
		}

		// Returns a view over every entity that owns all of the given components.
		// Iterating yields a tuple {entity_id, Components&...}.
		template <typename... Ts>
		View<Ts...> view() {
			return View<Ts...>(get_pool<Ts>()...);
		}

	private:
		template<typename T>
		detail::ComponentPool<T>* get_pool() {
			auto type_id = std::type_index(typeid(T));
			auto it = m_pools.find(type_id);
			if (it == m_pools.end())
				it = m_pools.emplace(type_id, std::make_unique<detail::ComponentPool<T>>()).first;
			return static_cast<detail::ComponentPool<T>*>(it->second.get());
		}

		void remove_entity_from_pools(me::entity::entity_id e) {
			for (auto& pair : m_pools)
				pair.second->remove(e);
		}

	private:
		std::unordered_map<entity::entity_id, detail::EntityRecord> m_entities;
		std::unordered_map<std::type_index, std::unique_ptr<detail::IPool>> m_pools;
		std::vector<entity::entity_id> m_entities_to_delete; // Deferred deletion queue
		std::vector<entity::entity_id> m_free_ids;           // Recycled ids ready for reuse
		entity::entity_id m_nextId = 1;
	};

	// =========================================================================
	// ENTITY INLINE IMPLEMENTATIONS
	// =========================================================================

	inline Entity::Entity(entity::entity_id id, entity::version generation, Registry* registry)
		: m_id(id), m_generation(generation), m_registry(registry) {}

	inline bool Entity::is_valid() const {
		if (m_id == entity::null || m_registry == nullptr) return false;
		return m_registry->is_valid_handle(m_id, m_generation);
	}

	template <typename T>
	inline void Entity::add_component(const T& component) {
		m_registry->add_component<T>(m_id, component);
	}

	template <typename T>
	inline T* Entity::try_get_component() {
		return m_registry->try_get_component<T>(m_id);
	}

	template <typename T>
	inline T& Entity::get_component() {
		T* component = m_registry->try_get_component<T>(m_id);
		assert(component && "Entity::get_component<T>() called on an entity that has no component T");
		return *component;
	}

	template <typename T>
	inline bool Entity::has_component() {
		return m_registry->has_component<T>(m_id);
	}

	template <typename T>
	inline void Entity::remove_component() {
		m_registry->remove_component<T>(m_id);
	}

	inline void Entity::destroy() {
		m_registry->destroy_entity(m_id);
	}

} // namespace me
#pragma once

#include "mini-ecs/entity.hpp"
#include "mini-ecs/component_pool.hpp"

#include <unordered_map>
#include <typeindex>
#include <memory>
#include <string>

namespace me {

	namespace detail {
		struct EntityRecord {
			bool alive = true;
		};
	}

	class Registry {
	public:
		Entity create_entity() {
			entity::entity_id id = m_nextId++;
			m_entities[id] = { true };

			// Pass 'this' so the entity knows which Registry owns it
			return Entity(id, this);
		}

		void destroy_entity(entity::entity_id e) {
			if (m_entities.find(e) != m_entities.end()) {
				m_entities[e].alive = false;
				remove_entity_from_pools(e);
			}
		}

		bool is_alive(entity::entity_id e) const {
			auto it = m_entities.find(e);
			return (it != m_entities.end()) && it->second.alive;
		}

		void clear() {
			m_entities.clear();
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

		// Returns the raw pool (Dense Vector + Map)
		template <typename T>
		detail::ComponentPool<T>& view() {
			return *get_pool<T>();
		}

	private:
		template<typename T>
		detail::ComponentPool<T>* get_pool() {
			auto typeId = std::type_index(typeid(T));
			if (m_pools.find(typeId) == m_pools.end())
				m_pools[typeId] = std::make_unique<detail::ComponentPool<T>>();
			return static_cast<detail::ComponentPool<T>*>(m_pools[typeId].get());
		}

		void remove_entity_from_pools(me::entity::entity_id e) {
			for (auto& pair : m_pools)
				pair.second->remove(e);
		}

	private:
		std::unordered_map<entity::entity_id, detail::EntityRecord> m_entities;
		std::unordered_map<std::type_index, std::unique_ptr<detail::IPool>> m_pools;
		entity::entity_id m_nextId = 1;
	};


	// =========================================================================
	// ENTITY INLINE IMPLEMENTATIONS
	// =========================================================================

	inline Entity::Entity(entity::entity_id id, Registry* registry)
		: m_id(id), m_registry(registry) {}

	inline bool Entity::is_valid() const {
		if (m_id == entity::null || m_registry == nullptr) return false;
		return m_registry->is_alive(m_id);
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
		return *m_registry->try_get_component<T>(m_id);
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
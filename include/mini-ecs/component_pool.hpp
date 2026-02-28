#pragma once

#include "mini-ecs/entity.hpp"

#include <vector>
#include <limits>

namespace me::detail {

	struct IPool {
		virtual ~IPool() = default;
		virtual void remove(me::entity::entity_id e) = 0;
		virtual bool has(me::entity::entity_id e) const = 0;
		virtual void clear() = 0;
	};

	template <typename T>
	struct ComponentPool : public IPool {
		std::vector<T> components; // Data
		std::vector<me::entity::entity_id> entity_map; // Index -> Entity ID
		std::vector<size_t> sparse; // Entity ID -> Index

		static constexpr size_t NONE = std::numeric_limits<size_t>::max();

		ComponentPool() {
			// Pre-allocate some space to avoid immediate resizing
			sparse.resize(100, NONE);
			components.reserve(100);
			entity_map.reserve(100);
		}

		void add(me::entity::entity_id e, const T& component) {
			if (has(e)) {
				// Update existing
				components[sparse[e]] = component;
				return;
			}

			if (e >= sparse.size()) {
				sparse.resize(e + 100, NONE);
			}

			sparse[e] = components.size();
			components.push_back(component);
			entity_map.push_back(e);
		}

		T* try_get(me::entity::entity_id e) {
			if (e >= sparse.size() || sparse[e] == NONE) return nullptr;
			return &components[sparse[e]];
		}

		bool has(me::entity::entity_id e) const override {
			return e < sparse.size() && sparse[e] != NONE;
		}

		void remove(me::entity::entity_id e) override {
			if (!has(e)) return;

			size_t dense_index = sparse[e];
			size_t last_index = components.size() - 1;
			me::entity::entity_id last_entity = entity_map[last_index];

			// Swap & Pop
			components[dense_index] = components[last_index];
			entity_map[dense_index] = last_entity;

			// Update sparse map
			sparse[last_entity] = dense_index;
			sparse[e] = NONE;

			components.pop_back();
			entity_map.pop_back();
		}

		void clear() override {
			components.clear();
			entity_map.clear();
			std::fill(sparse.begin(), sparse.end(), NONE);
		}

		size_t size() const { return components.size(); }
		bool empty() const { return components.empty(); }
	};

} // namespace me::detail
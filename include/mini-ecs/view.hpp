#pragma once

#include "mini-ecs/entity.hpp"
#include "mini-ecs/component_pool.hpp"

#include <tuple>
#include <vector>
#include <cstddef>
#include <utility>

namespace me {

	// A lazy view over every entity that owns all of the given components.
	// Iterating yields a tuple {entity_id, Components&...}, so callers can use
	// structured bindings directly:
	//
	//   for (auto [id, transform, velocity] : registry.view<Transform, Velocity>()) { ... }
	//
	// Iteration is driven by the smallest matching pool, so the cost scales with
	// the rarest requested component rather than the total entity count.
	template <typename... Ts>
	class View {
		static_assert(sizeof...(Ts) >= 1, "view<>() requires at least one component type");

	public:
		explicit View(detail::ComponentPool<Ts>*... pools)
			: m_pools(pools...), m_driver(pick_driver(pools...)) {}

		class Iterator {
		public:
			Iterator(const View* view, std::size_t index)
				: m_view(view), m_index(index) {
				skip_invalid();
			}

			bool operator==(const Iterator& other) const { return m_index == other.m_index; }
			bool operator!=(const Iterator& other) const { return m_index != other.m_index; }

			Iterator& operator++() {
				++m_index;
				skip_invalid();
				return *this;
			}

			auto operator*() const {
				entity::entity_id e = m_view->driver_entities()[m_index];
				return m_view->components_for(e, std::index_sequence_for<Ts...>{});
			}

		private:
			// Advance until m_index points at an entity owning every requested
			// component, or reaches the end of the driver pool.
			void skip_invalid() {
				const auto& ents = m_view->driver_entities();
				while (m_index < ents.size() && !m_view->all_have(ents[m_index]))
					++m_index;
			}

			const View* m_view;
			std::size_t m_index;
		};

		Iterator begin() const { return Iterator(this, 0); }
		Iterator end() const { return Iterator(this, driver_entities().size()); }

	private:
		const std::vector<entity::entity_id>& driver_entities() const {
			return m_driver->entities();
		}

		bool all_have(entity::entity_id e) const {
			return all_have_impl(e, std::index_sequence_for<Ts...>{});
		}

		template <std::size_t... Is>
		bool all_have_impl(entity::entity_id e, std::index_sequence<Is...>) const {
			return (std::get<Is>(m_pools)->has(e) && ...);
		}

		template <std::size_t... Is>
		std::tuple<entity::entity_id, Ts&...> components_for(entity::entity_id e, std::index_sequence<Is...>) const {
			return std::tuple<entity::entity_id, Ts&...>(e, (*std::get<Is>(m_pools)->try_get(e))...);
		}

		// The pool with the fewest components drives iteration.
		static detail::IPool* pick_driver(detail::ComponentPool<Ts>*... pools) {
			detail::IPool* candidates[] = { static_cast<detail::IPool*>(pools)... };
			detail::IPool* smallest = candidates[0];
			for (detail::IPool* p : candidates)
				if (p->size() < smallest->size()) smallest = p;
			return smallest;
		}

		std::tuple<detail::ComponentPool<Ts>*...> m_pools;
		detail::IPool* m_driver;
	};

} // namespace me

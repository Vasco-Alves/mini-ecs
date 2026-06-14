// Minimal assertion-based smoke tests for mini-ecs.
// Build with: cmake -DMINI_ECS_BUILD_TESTS=ON .. && cmake --build . && ctest
#include <mini-ecs/registry.hpp>

#include <cassert>
#include <iostream>

struct Transform { float x = 0.0f; float y = 0.0f; };
struct Velocity  { float vx = 0.0f; float vy = 0.0f; };
struct Tag {};

int main() {
	using namespace me;
	Registry r;

	// --- create / add / has / get -------------------------------------------
	Entity e1 = r.create_entity();
	assert(e1.is_valid());
	e1.add_component(Transform{ 1.0f, 2.0f });
	e1.add_component(Velocity{ 3.0f, 4.0f });
	assert(e1.has_component<Transform>());
	assert(e1.has_component<Velocity>());
	assert(!e1.has_component<Tag>());
	assert(e1.get_component<Transform>().x == 1.0f);
	assert(e1.try_get_component<Tag>() == nullptr);

	// add_component on an existing component updates in place
	e1.add_component(Transform{ 10.0f, 20.0f });
	assert(e1.get_component<Transform>().x == 10.0f);

	// --- swap-and-pop removal of a middle element ---------------------------
	Entity a = r.create_entity(); a.add_component(Transform{ 0.0f, 0.0f });
	Entity b = r.create_entity(); b.add_component(Transform{ 1.0f, 1.0f });
	Entity c = r.create_entity(); c.add_component(Transform{ 2.0f, 2.0f });
	b.remove_component<Transform>();
	assert(!b.has_component<Transform>());
	assert(a.get_component<Transform>().x == 0.0f);
	assert(c.get_component<Transform>().x == 2.0f);

	// --- view iteration + mutation through the reference --------------------
	int count = 0;
	for (auto [id, t] : r.view<Transform>()) {
		(void)id;
		t.x += 100.0f;
		++count;
	}
	assert(count == 3); // e1, a, c (b was removed)
	assert(e1.get_component<Transform>().x == 110.0f);
	assert(a.get_component<Transform>().x == 100.0f);
	assert(c.get_component<Transform>().x == 102.0f);

	// --- multi-component view (intersection of pools) -----------------------
	{
		Registry mv;
		Entity m1 = mv.create_entity(); // both
		m1.add_component(Transform{ 1.0f, 1.0f });
		m1.add_component(Velocity{ 2.0f, 0.0f });
		Entity m2 = mv.create_entity(); // transform only
		m2.add_component(Transform{ 3.0f, 3.0f });
		Entity m3 = mv.create_entity(); // both
		m3.add_component(Transform{ 4.0f, 4.0f });
		m3.add_component(Velocity{ 5.0f, 0.0f });
		Entity m4 = mv.create_entity(); // velocity only
		m4.add_component(Velocity{ 6.0f, 0.0f });

		int matched = 0;
		float sum_x = 0.0f;
		for (auto [id, tr, ve] : mv.view<Transform, Velocity>()) {
			(void)id;
			tr.x += ve.vx; // mutate transform through the reference
			sum_x += tr.x;
			++matched;
		}
		assert(matched == 2);          // only m1 and m3 own both
		assert(sum_x == 12.0f);        // (1+2) + (4+5)
		assert(m1.get_component<Transform>().x == 3.0f);
		assert(m3.get_component<Transform>().x == 9.0f);
		assert(m2.get_component<Transform>().x == 3.0f); // untouched (no Velocity)

		// Order of type arguments is independent of the result set.
		int reversed = 0;
		for (auto [id, ve, tr] : mv.view<Velocity, Transform>()) {
			(void)id; (void)ve; (void)tr;
			++reversed;
		}
		assert(reversed == 2);

		// A view requiring an unused component type matches nothing.
		int none = 0;
		for (auto [id, tag] : mv.view<Tag>()) { (void)id; (void)tag; ++none; }
		assert(none == 0);

		// get_entity upgrades a raw view id back into a full handle.
		for (auto [id, tr] : mv.view<Transform>()) {
			(void)tr;
			Entity handle = mv.get_entity(id);
			assert(handle.is_valid());
			assert(handle.get_id() == id);
		}
	}

	// --- deferred deletion ---------------------------------------------------
	entity::entity_id aid = a.get_id();
	a.destroy();
	assert(!a.is_valid());          // hidden the moment it is marked
	assert(!r.is_alive(aid));
	r.process_deletions();
	assert(!r.has_component<Transform>(aid));

	// --- id recycling + generation invalidates stale handles ----------------
	Entity recycled = r.create_entity();
	assert(recycled.get_id() == aid); // index reused
	assert(recycled.is_valid());
	assert(!a.is_valid());            // old handle stale via generation bump

	// get_entity hands back the entity that currently owns the id...
	assert(r.get_entity(aid) == recycled);
	assert(r.get_entity(aid).is_valid());
	// ...and a null handle for an id that was never created.
	assert(!r.get_entity(999999u).is_valid());

	// --- reentrancy: a callback may safely queue more deletions -------------
	Entity p = r.create_entity();
	Entity child = r.create_entity();
	p.destroy();
	r.process_deletions([&](entity::entity_id) {
		child.destroy(); // queued for the next pass, must not crash
	});
	assert(!p.is_valid());
	assert(!child.is_valid()); // marked for deletion
	r.process_deletions();
	assert(!child.is_valid());

	// --- clear resets everything --------------------------------------------
	r.clear();
	assert(!e1.is_valid());
	Entity fresh = r.create_entity();
	assert(fresh.get_id() == 1); // ids reset to 1

	std::cout << "All mini-ecs tests passed\n";
	return 0;
}

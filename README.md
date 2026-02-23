# Mini ECS

A lightweight, header-only Entity Component System (ECS) written in modern C++.

Designed to be simple, type-safe, and incredibly easy to drop into any game engine or project.

## Features

- **Header-Only:** No source files to compile. Just link the `include` directory and you're good to go.
- **Type-Safe:** Uses standard C++ templates and `std::type_index` to manage components securely.
- **Clean API:** Entity interactions are handled through a safe `Entity` handle class rather than raw IDs.
- **No Dependencies:** Relies entirely on the C++ Standard Library.

## Installation

Since **Mini ECS** is a header-only CMake `INTERFACE` library, integrating it into your project is very simple.

### Git Submodule (Recommended)

Add this repository to your project's `vendor` folder:

```bash
git submodule add https://github.com/Vasco-Alves/mini-ecs.git vendor/mini-ecs
```

Then, in your project's `CMakeLists.txt`:

```cmake
# Add the subdirectory
add_subdirectory(vendor/mini-ecs)

# Link it to your executable or library
target_link_libraries(my_game PUBLIC mini-ecs)`
```

## Quick Start

### 1. Define your Components

Components are just plain C++ structs. No inheritance required.

```cpp
struct Transform {
    float x = 0.0f;
    float y = 0.0f;
};

struct Velocity {
    float vx = 0.0f;
    float vy = 0.0f;
};
```

### 2. Create Entities and Add Components

Include the registry and start building your world.

```cpp
#include <mini-ecs/registry.hpp>
#include <iostream>

int main() {
    // Create the ECS World
    me::Registry registry;

    // Create an entity handle
    me::Entity player = registry.create_entity("Player");

    // Add components
    player.add_component(Transform{10.0f, 20.0f});
    player.add_component(Velocity{1.0f, 0.0f});

    // Check for components
    if (player.has_component<Transform>()) {
        std::cout << "Player has a transform!" << std::endl;
    }

    return 0;
}
```

### 3. Iterate over Components (Systems)

To update your game, request a `view` of a specific component pool and iterate through it.

```cpp
void update_physics(me::Registry& registry, float dt) {
    auto& transforms = registry.view<Transform>();
    
    for (auto& [entity_id, transform] : transforms) {
        // You can use the ID to lookup other components!
        if (Velocity* vel = registry.try_get_component<Velocity>(entity_id)) {
            transform.x += vel->vx * dt;
            transform.y += vel->vy * dt;
        }
    }
}
```
## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
#ifndef ENTITY_H
#define ENTITY_H

//#include "model.h"
#include <util.h>

// entity flags
#define MODEL_FLAG_DRAW (1 << 0)
#define MODEL_FLAG_WIREFRAME (1 << 1)
#define MODEL_FLAG_OUTLINE (1 << 2)
#define MODEL_FLAG_FLIPTEX (1 << 3)
#define MODEL_FLAG_BLEND (1 << 4)

static uint64_t __total_entities = 0;



struct Transformation
{
	float scale;
    float default_scale;
	glm::vec3 rotation;
	glm::vec3 position;
	glm::mat4 matrix;
};

const Transformation default_transformation
{
        .scale = float(1.0f),
        .default_scale = float(1.0f),
        .rotation = glm::vec3(0.0f),
        .position = glm::vec3(0.0f),
        .matrix = glm::mat4(glm::scale(glm::identity<glm::mat4>(), glm::vec3(0.1f)))
};
/*
struct ModelComponent
{
    Model model;
    Transformation transformation;
    uint64_t flags; // can set up to 64 conditions
    Shader* shaderOverride{ nullptr };

    ModelComponent(const Model& m, const Transformation& t = default_transformation, uint64_t f = 0)
        : model(m), transformation(t), flags(MODEL_FLAG_DRAW | MODEL_FLAG_FLIPTEX) {}

    ModelComponent() = delete;
};

void create_ModelComponent(ModelComponent& modComp, const Model& mod, const Transformation& transformComp);
*/


struct entity
{
    uint64_t ID;

    entity() { ID = ++__total_entities; };

    ~entity() = default;
};

extern std::unordered_map<std::type_index, std::unordered_map<uint64_t, std::any>> component_storage;

template <typename T>
void add_component(entity& entity, const T& component)
{
    std::type_index type_id = std::type_index(typeid(T));

    T comp_copy = component;

    component_storage[type_id][entity.ID] = std::make_any<T>(comp_copy);
}

template <typename T>
T* get_component(const entity& entity)
{
    std::type_index type_id = std::type_index(typeid(T));

    auto type_it = component_storage.find(type_id);
    if (type_it == component_storage.end())
        return nullptr;


    auto comp_it = type_it->second.find(entity.ID);
    if (comp_it == type_it->second.end())
        return nullptr;

    try
    {
        return std::any_cast<T>(&comp_it->second);
    }
    catch (const std::bad_any_cast&)
    {
        return nullptr;
    }

}

template <typename T>
void delete_component(entity& entity)
{
    std::type_index type_id = std::type_index(typeid(T));

    auto type_it = component_storage.find(type_id);
    if (type_it != component_storage.end()) {
        type_it->second.erase(entity.ID);
    }
}

#endif
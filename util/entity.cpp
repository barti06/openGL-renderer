#include <rtGL/entity.h>

/*
void create_ModelComponent(ModelComponent& modComp, const Model& mod, const Transformation& transformComp)
{
    modComp.model = mod;
    modComp.transformation = transformComp;
    modComp.flags = MODEL_FLAG_DRAW;
}
*/

std::unordered_map<std::type_index, std::unordered_map<uint64_t, std::any>> component_storage;

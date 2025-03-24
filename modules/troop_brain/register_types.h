#ifndef REGISTER_TYPES_H
#define REGISTER_TYPES_H// Stellt ModuleInitializationLevel bereit

#include "modules/register_module_types.h"

void initialize_troop_brain_module(ModuleInitializationLevel p_level);
void uninitialize_troop_brain_module(ModuleInitializationLevel p_level);

#endif // REGISTER_TYPES_H
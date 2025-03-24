#include "register_types.h"

#include "core/object/class_db.h"
#include "TroopBrain.h"

void initialize_troop_brain_module(ModuleInitializationLevel p_level) {
    if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
        ClassDB::register_class<TroopBrain>();
    }
}

void uninitialize_troop_brain_module(ModuleInitializationLevel p_level) {
    // Cleanup-Code falls nötig
}
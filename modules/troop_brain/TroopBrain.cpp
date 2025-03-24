#include "TroopBrain.h"
#include "core/object/class_db.h"
#include "core/math/math_funcs.h"
#include "core/os/os.h"

void TroopBrain::_bind_methods() {
    ClassDB::bind_method(D_METHOD("initialize", "troop_unit", "grid_manager"), &TroopBrain::initialize);
    ClassDB::bind_method(D_METHOD("update_target"), &TroopBrain::update_target);
    ClassDB::bind_method(D_METHOD("update", "delta"), &TroopBrain::update);
    ClassDB::bind_method(D_METHOD("get_first_wall_on_path", "ignore_path"), &TroopBrain::get_first_wall_on_path);
    ClassDB::bind_method(D_METHOD("set_detection_range", "range"), &TroopBrain::set_detection_range);
    ClassDB::bind_method(D_METHOD("set_attack_range", "range"), &TroopBrain::set_attack_range);
    ClassDB::bind_method(D_METHOD("set_speed", "speed"), &TroopBrain::set_speed);
    ClassDB::bind_method(D_METHOD("set_max_building_path_length", "length"), &TroopBrain::set_max_building_path_length);
    ClassDB::bind_method(D_METHOD("set_max_detour", "detour"), &TroopBrain::set_max_detour);
}

TroopBrain::TroopBrain() {
    // Standardwerte werden im Header initialisiert.
    path_index = 0;
    attack_target_point = Vector2();
}

void TroopBrain::initialize(Node2D *p_troop_unit, GridManager *p_grid_manager) {
    troop_unit = p_troop_unit;
    grid_manager = p_grid_manager;
}

void TroopBrain::update_target() {
    if (!grid_manager || !troop_unit)
        return;
        
    Dictionary occupancy = grid_manager->get_grid_occupancy();
    float best_dist = detection_range;
    attack_target = nullptr;
    attack_target_point = Vector2();
    
    Array keys = occupancy.keys();
    // Zuerst Gebäude-Ziel suchen (nicht Wand)
    for (int i = 0; i < keys.size(); i++) {
        Vector2 cell = keys[i];
        Dictionary obj = occupancy[cell];
        bool is_wall = obj.has("is_wall") ? bool(obj["is_wall"]) : false;
        if (!is_wall) {
            Vector2 cell_center = grid_manager->grid_to_screen(cell + Vector2(0.5, 0.5));
            float d = troop_unit->get_global_position().distance_to(cell_center);
            if (d < best_dist) {
                best_dist = d;
                attack_target_point = cell_center;
                attack_target = Object::cast_to<Node2D>(obj["obj"]);
            }
        }
    }
    
    // Falls kein Gebäude-Ziel gefunden wurde, suche nach einem Wand-Ziel.
    if (!attack_target) {
        for (int i = 0; i < keys.size(); i++) {
            Vector2 cell = keys[i];
            Dictionary obj = occupancy[cell];
            bool is_wall = obj.has("is_wall") ? bool(obj["is_wall"]) : false;
            if (is_wall) {
                Vector2 cell_center = grid_manager->grid_to_screen(cell + Vector2(0.5, 0.5));
                float d = troop_unit->get_global_position().distance_to(cell_center);
                if (d < best_dist) {
                    best_dist = d;
                    attack_target_point = cell_center;
                    attack_target = Object::cast_to<Node2D>(obj["obj"]);
                }
            }
        }
    }
}

void TroopBrain::move(float delta) {
    if (!troop_unit || !grid_manager)
        return;

    // Falls noch kein Pfad berechnet oder abgelaufen: Pfad neu ermitteln
    if (path.size() == 0 || path_index >= path.size()) {
        print_line("Berechne neuen Pfad");
        Array avoid_path = grid_manager->get_simple_path_iso_avoid_walls_smoothed(
            troop_unit->get_global_position(), attack_target_point);
        // Falls der Umwegs-Pfad zu lang ist, ignore-Pfad betrachten:
        if (avoid_path.size() > max_building_path_length) {
            print_line("Umweg zu lang, ignoriere Wände");
            Array ignore_path = grid_manager->get_simple_path_iso_ignore_walls(
                troop_unit->get_global_position(), attack_target_point);
            Node2D *wall_target = get_first_wall_on_path(ignore_path);
            if (wall_target) {
                print_line("Wand gefunden");
                attack_target = wall_target;
                attack_target_point = wall_target->get_global_position();
                avoid_path = grid_manager->get_simple_path_iso_avoid_walls_smoothed(
                    troop_unit->get_global_position(), attack_target_point);
            }
        }
        path = avoid_path;
        path_index = 0;
    }

    // Bewege die Truppe entlang des Pfades
    if (path.size() > 0) {
        Vector2 target_point = path[path_index];
        Vector2 direction = target_point - troop_unit->get_global_position();
        float distance = direction.length();
        Vector2 velocity = direction.normalized() * speed * delta;
        // Beispiel: Anpassung der Y-Komponente
        velocity.y *= 0.5f;
        if (distance <= velocity.length()) {
            troop_unit->set_global_position(target_point);
            path_index++;
            if (path_index >= path.size()) {
                // Pfad abgeschlossen, reset
                path.resize(0);
                path_index = 0;
            }
        } else {
            troop_unit->set_global_position(troop_unit->get_global_position() + velocity);
        }
    }
}

void TroopBrain::update(float delta) {
    if (!troop_unit || !grid_manager)
        return;

    // Wenn kein Ziel vorhanden, neues Ziel bestimmen
    if (!attack_target) {
        update_target();
        return;
    }
    
    float dist_to_target = troop_unit->get_global_position().distance_to(attack_target_point);
    if (dist_to_target <= attack_range) {
        // Ziel ist im Angriffsbereich – hier könnten Trigger gesetzt oder Angriffe ausgeführt werden
        return;
    }
    
    // Alle logischen Entscheidungen (z. B. Zielwechsel) können hier getroffen werden.
    // Anschließend nur noch bewegen:
    move(delta);
}

Node2D *TroopBrain::get_first_wall_on_path(const Array &ignore_path) {
    for (int i = 0; i < ignore_path.size(); i++) {
        Vector2 screen_point = ignore_path[i];
        Vector2 cell = grid_manager->screen_to_grid(screen_point);
        cell.x = Math::floor(cell.x);
        cell.y = Math::floor(cell.y);
        Dictionary occ = grid_manager->get_grid_occupancy();
        if (occ.has(cell)) {
            Dictionary obj = occ[cell];
            if (obj.has("is_wall") && obj["is_wall"]) {
                return Object::cast_to<Node2D>(obj["obj"]);
            }
        }
    }
    return nullptr;
}
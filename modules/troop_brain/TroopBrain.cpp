#include "TroopBrain.h"
#include "core/object/class_db.h"
#include "core/math/math_funcs.h"
#include "core/os/os.h"
#include <cmath> // for fabsf and FLT_MAX

void TroopBrain::_bind_methods() {
    ClassDB::bind_method(D_METHOD("initialize", "troop_unit", "grid_manager"), &TroopBrain::initialize);
    ClassDB::bind_method(D_METHOD("update_target"), &TroopBrain::update_target);
    ClassDB::bind_method(D_METHOD("update", "delta"), &TroopBrain::update);
    ClassDB::bind_method(D_METHOD("get_first_wall_on_path", "ignore_path"), &TroopBrain::get_first_wall_on_path);
    ClassDB::bind_method(D_METHOD("move", "delta"), &TroopBrain::move); // Newly added binding
    
    ClassDB::bind_method(D_METHOD("set_detection_range", "range"), &TroopBrain::set_detection_range);
    ClassDB::bind_method(D_METHOD("set_attack_range", "range"), &TroopBrain::set_attack_range);
    ClassDB::bind_method(D_METHOD("set_speed", "speed"), &TroopBrain::set_speed);
    ClassDB::bind_method(D_METHOD("set_max_building_path_length", "length"), &TroopBrain::set_max_building_path_length);
    ClassDB::bind_method(D_METHOD("set_max_detour", "detour"), &TroopBrain::set_max_detour);

    // Optionally bind getters if needed in scripts
    ClassDB::bind_method(D_METHOD("get_detection_range"), &TroopBrain::get_detection_range);
    ClassDB::bind_method(D_METHOD("get_attack_range"), &TroopBrain::get_attack_range);
    ClassDB::bind_method(D_METHOD("get_speed"), &TroopBrain::get_speed);
    ClassDB::bind_method(D_METHOD("get_max_building_path_length"), &TroopBrain::get_max_building_path_length);
    ClassDB::bind_method(D_METHOD("get_max_detour"), &TroopBrain::get_max_detour);
    ClassDB::bind_method(D_METHOD("get_attack_target_point"), &TroopBrain::get_attack_target_point);
    ClassDB::bind_method(D_METHOD("get_attack_target"), &TroopBrain::get_attack_target);
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
    if (!grid_manager || !troop_unit) {
        //print_line("update_target: grid_manager oder troop_unit nicht gesetzt.");
        return;
    }
        
    Dictionary occupancy = grid_manager->get_grid_occupancy();
    //print_line(vformat("update_target: occupancy keys count: %d", occupancy.keys().size()));
    
    float best_dist = detection_range;
    attack_target = nullptr;
    attack_target_point = Vector2();
    
    Array keys = occupancy.keys();
    // Zuerst Gebäude-Ziel suchen (nicht Wand)
    for (int i = 0; i < keys.size(); i++) {
        Vector2 cell = keys[i];
        Dictionary obj = occupancy[cell];
        bool is_wall = obj.has("is_wall") ? bool(obj["is_wall"]) : false;
        //print_line(vformat("update_target: cell: (%f, %f) is_wall: %s", cell.x, cell.y, is_wall ? "true" : "false"));
        if (!is_wall) {
            // If a building size is provided, use it. Otherwise assume size = 1.
            int b_size = obj.has("size") ? int(obj["size"]) : 1;
            Vector2 offset = Vector2(b_size * 0.5f, b_size * 0.5f);
            Vector2 center = grid_manager->grid_to_screen(cell + offset);
            
            // Calculate the building border point along the line from its center to troop position.
            Vector2 diff = troop_unit->get_global_position() - center;
            if (diff == Vector2()) {
                // Avoid divide-by-zero; choose center.
                diff = Vector2(1,0);
            }
            Vector2 direction = diff.normalized();
            float hx = b_size * 0.5f;
            float hy = b_size * 0.5f;
            float tx = fabsf(direction.x) > 0.0001f ? hx / fabsf(direction.x) : FLT_MAX;
            float ty = fabsf(direction.y) > 0.0001f ? hy / fabsf(direction.y) : FLT_MAX;
            float t = MIN(tx, ty);
            Vector2 target_point = center + direction * t;
            
            float d = troop_unit->get_global_position().distance_to(target_point);
            Node2D *potential_target = Object::cast_to<Node2D>(obj["obj"]);
            if (potential_target && d < best_dist) {
                best_dist = d;
                attack_target_point = target_point;
                attack_target = potential_target;
                //print_line("update_target: Found non-wall target.");
            }
        }
    }
    
    // Falls kein gültiges Gebäude-Ziel gefunden wurde, suche nach einem Wand-Ziel.
    if (!attack_target) {
        //print_line("update_target: Kein Building-Ziel gefunden, suche Wand-Ziel.");
        for (int i = 0; i < keys.size(); i++) {
            Vector2 cell = keys[i];
            Dictionary obj = occupancy[cell];
            bool is_wall = obj.has("is_wall") ? bool(obj["is_wall"]) : false;
            if (is_wall) {
                int b_size = obj.has("size") ? int(obj["size"]) : 1;
                Vector2 offset = Vector2(b_size * 0.5f, b_size * 0.5f);
                Vector2 center = grid_manager->grid_to_screen(cell + offset);
                
                Vector2 diff = troop_unit->get_global_position() - center;
                if (diff == Vector2()) {
                    diff = Vector2(1,0);
                }
                Vector2 direction = diff.normalized();
                float hx = b_size * 0.5f;
                float hy = b_size * 0.5f;
                float tx = fabsf(direction.x) > 0.0001f ? hx / fabsf(direction.x) : FLT_MAX;
                float ty = fabsf(direction.y) > 0.0001f ? hy / fabsf(direction.y) : FLT_MAX;
                float t = MIN(tx, ty);

                float cell_adjustment = 1.0f; // adjust this constant if needed
                float t_adjusted = (t > cell_adjustment) ? (t - cell_adjustment) : t;

                Vector2 target_point = center + direction * t_adjusted;
                
                float d = troop_unit->get_global_position().distance_to(target_point);
                Node2D *potential_target = Object::cast_to<Node2D>(obj["obj"]);
                if (potential_target && d < best_dist) {
                    best_dist = d;
                    attack_target_point = target_point;
                    attack_target = potential_target;
                    //print_line("update_target: Found wall target.");
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
        //print_line("Berechne neuen Pfad");
        Array avoid_path = grid_manager->get_simple_path_iso_avoid_walls_smoothed(
            troop_unit->get_global_position(), attack_target_point);
        // Falls der Umwegs-Pfad zu lang ist, ignore-Pfad betrachten:
        if (avoid_path.size() > max_building_path_length) {
            //print_line("Umweg zu lang, ignoriere Wände");
            Array ignore_path = grid_manager->get_simple_path_iso_ignore_walls(
                troop_unit->get_global_position(), attack_target_point);
            Node2D *wall_target = get_first_wall_on_path(ignore_path);
            if (wall_target) {
                //print_line("Wand gefunden");
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
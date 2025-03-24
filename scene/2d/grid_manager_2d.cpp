#include <queue>
#include <unordered_map>
#include <cmath>
#include <functional>

#include "core/math/vector2.h"
#include "grid_manager_2d.h"
#include "core/object/class_db.h"
#include "core/math/math_funcs.h"

// Spezialisierung für std::hash<Vector2>
namespace std {
    template <>
    struct hash<Vector2> {
        std::size_t operator()(const Vector2 &v) const {
            std::hash<float> float_hash;
            std::size_t h1 = float_hash(v.x);
            std::size_t h2 = float_hash(v.y);
            return h1 ^ (h2 << 1);
        }
    };
}

struct AStarNode {
    Vector2 cell;
    float priority;
    bool operator<(const AStarNode& other) const {
        return priority > other.priority; // Min-Heap für die Priority Queue
    }
};

GridManager::GridManager() {
    // Standardwerte sind bereits im Header gesetzt
    last_path_init_point = Vector2(0, 0);
}

GridManager::~GridManager() {
    // Keine dynamischen Ressourcen, die freigegeben werden müssen
}

void GridManager::_bind_methods() {
    // API-Methoden binden
    ClassDB::bind_method(D_METHOD("api_place_building", "top_left_cell", "building_scene", "building_size"), &GridManager::api_place_building);
    ClassDB::bind_method(D_METHOD("api_move_building", "building_instance", "new_top_left_cell", "building_size"), &GridManager::api_move_building);
    ClassDB::bind_method(D_METHOD("api_spawn_troop", "screen_pos", "troop_scene"), &GridManager::api_spawn_troop);
    ClassDB::bind_method(D_METHOD("api_enable_build_mode"), &GridManager::api_enable_build_mode);
    ClassDB::bind_method(D_METHOD("api_enable_move_mode", "state"), &GridManager::api_enable_move_mode);
    ClassDB::bind_method(D_METHOD("api_disable_modes"), &GridManager::api_disable_modes);

    // Intern verwendete Funktionen binden
    ClassDB::bind_method(D_METHOD("can_place_building", "top_left_cell", "building_size"), &GridManager::can_place_building);
    ClassDB::bind_method(D_METHOD("place_building", "top_left_cell", "building_scene", "building_size"), &GridManager::place_building);
    ClassDB::bind_method(D_METHOD("move_building", "building_instance", "new_top_left_cell", "building_size"), &GridManager::move_building);
    ClassDB::bind_method(D_METHOD("can_spawn_troop", "screen_pos"), &GridManager::can_spawn_troop);
    ClassDB::bind_method(D_METHOD("spawn_troop", "screen_pos", "troop_scene"), &GridManager::spawn_troop);
    ClassDB::bind_method(D_METHOD("update_z_index"), &GridManager::update_z_index);
    ClassDB::bind_method(D_METHOD("toggle_modes", "p_last_active_state"), &GridManager::toggle_modes);

    // Pfadfindungsmethoden binden (ergänzt um get_simple_path_through_wall)
    ClassDB::bind_method(D_METHOD("get_simple_path", "start_pos", "end_pos"), &GridManager::get_simple_path);
    ClassDB::bind_method(D_METHOD("get_simple_path_iso_avoid_walls", "start_pos", "end_pos"), &GridManager::get_simple_path_iso_avoid_walls);
    ClassDB::bind_method(D_METHOD("get_simple_path_iso_ignore_walls", "start_pos", "end_pos"), &GridManager::get_simple_path_iso_ignore_walls);
    ClassDB::bind_method(D_METHOD("smooth_path", "path", "segments"), &GridManager::smooth_path, DEFVAL(10));
    ClassDB::bind_method(D_METHOD("get_simple_path_iso_avoid_walls_smoothed", "start_pos", "end_pos"), &GridManager::get_simple_path_iso_avoid_walls_smoothed);
    ClassDB::bind_method(D_METHOD("get_direct_path", "start_pos", "end_pos", "segments"), &GridManager::get_direct_path, DEFVAL(10));
    ClassDB::bind_method(D_METHOD("get_simple_path_through_wall", "start_pos", "end_pos", "max_detour"), &GridManager::get_simple_path_through_wall);
    ClassDB::bind_method(D_METHOD("get_natural_path_through_wall", "start_pos", "end_pos"), &GridManager::get_natural_path_through_wall);

    // Utility-Funktionen binden, damit sie in GDScript verfügbar sind:
    ClassDB::bind_method(D_METHOD("grid_to_screen", "grid_pos"), &GridManager::grid_to_screen);
    ClassDB::bind_method(D_METHOD("screen_to_grid", "screen_pos"), &GridManager::screen_to_grid);

    // Getter und Setter binden
    ClassDB::bind_method(D_METHOD("set_cell_size", "size"), &GridManager::set_cell_size);
    ClassDB::bind_method(D_METHOD("get_cell_size"), &GridManager::get_cell_size);
    ClassDB::bind_method(D_METHOD("set_grid_size", "size"), &GridManager::set_grid_size);
    ClassDB::bind_method(D_METHOD("get_grid_size"), &GridManager::get_grid_size);
    ClassDB::bind_method(D_METHOD("set_min_z_index", "index"), &GridManager::set_min_z_index);
    ClassDB::bind_method(D_METHOD("get_min_z_index"), &GridManager::get_min_z_index);
    ClassDB::bind_method(D_METHOD("set_max_z_index", "index"), &GridManager::set_max_z_index);
    ClassDB::bind_method(D_METHOD("get_max_z_index"), &GridManager::get_max_z_index);
    ClassDB::bind_method(D_METHOD("set_grid_lines", "enable"), &GridManager::set_grid_lines);
    ClassDB::bind_method(D_METHOD("get_grid_lines"), &GridManager::get_grid_lines);
    ClassDB::bind_method(D_METHOD("set_show_occupied", "enable"), &GridManager::set_show_occupied);
    ClassDB::bind_method(D_METHOD("get_show_occupied"), &GridManager::get_show_occupied);
    ClassDB::bind_method(D_METHOD("get_grid_occupancy"), &GridManager::get_grid_occupancy);
    ClassDB::add_property("GridManager", PropertyInfo(Variant::DICTIONARY, "grid_occupancy"), "", "get_grid_occupancy");

    // Eigenschaften binden
    ClassDB::add_property("GridManager", PropertyInfo(Variant::VECTOR2, "cell_size"), "set_cell_size", "get_cell_size");
    ClassDB::add_property("GridManager", PropertyInfo(Variant::INT, "grid_size"), "set_grid_size", "get_grid_size");
    ClassDB::add_property("GridManager", PropertyInfo(Variant::INT, "min_z_index"), "set_min_z_index", "get_min_z_index");
    ClassDB::add_property("GridManager", PropertyInfo(Variant::INT, "max_z_index"), "set_max_z_index", "get_max_z_index");
    ClassDB::add_property("GridManager", PropertyInfo(Variant::BOOL, "grid_lines"), "set_grid_lines", "get_grid_lines");
    ClassDB::add_property("GridManager", PropertyInfo(Variant::BOOL, "show_occupied"), "set_show_occupied", "get_show_occupied");
}

// Getter und Setter
void GridManager::set_cell_size(Vector2 size) { cell_size = size; }
Vector2 GridManager::get_cell_size() const { return cell_size; }
void GridManager::set_grid_size(int size) { grid_size = size; }
int GridManager::get_grid_size() const { return grid_size; }
void GridManager::set_min_z_index(int index) { min_z_index = index; }
int GridManager::get_min_z_index() const { return min_z_index; }
void GridManager::set_max_z_index(int index) { max_z_index = index; }
int GridManager::get_max_z_index() const { return max_z_index; }
void GridManager::set_grid_lines(bool enable) { grid_lines = enable; }
bool GridManager::get_grid_lines() const { return grid_lines; }
void GridManager::set_show_occupied(bool enable) { show_occupied = enable; }
bool GridManager::get_show_occupied() const { return show_occupied; }

void GridManager::_ready() {
    set_process(true);
}

// API-Methoden
bool GridManager::api_place_building(Vector2 top_left_cell, Ref<PackedScene> building_scene, int building_size) {
    return place_building(top_left_cell, building_scene, building_size);
}

bool GridManager::api_move_building(Node2D* building_instance, Vector2 new_top_left_cell, int building_size) {
    return move_building(building_instance, new_top_left_cell, building_size);
}

bool GridManager::api_spawn_troop(Vector2 screen_pos, Ref<PackedScene> troop_scene) {
    return spawn_troop(screen_pos, troop_scene);
}

void GridManager::api_enable_build_mode() {
    build_mode = true;
    move_mode = false;
    last_active_state.clear();
    print_line("Baumodus aktiviert.");
}

void GridManager::api_enable_move_mode(Dictionary state) {
    last_active_state = state.duplicate();
    build_mode = false;
    move_mode = true;
    print_line("Verschiebemodus aktiviert.");
}

void GridManager::api_disable_modes() {
    build_mode = false;
    move_mode = false;
    last_active_state.clear();
    print_line("Alle Modi deaktiviert.");
}

// Utility-Funktionen
Vector2 GridManager::grid_to_screen(Vector2 grid_pos) const {
    return Vector2((grid_pos.x - grid_pos.y) * (cell_size.x / 2),
                   (grid_pos.x + grid_pos.y) * (cell_size.y / 2));
}

Vector2 GridManager::screen_to_grid(Vector2 screen_pos) const {
    float A = cell_size.x / 2;
    float B = cell_size.y / 2;
    float i = (screen_pos.y / B + screen_pos.x / A) / 2;
    float j = (screen_pos.y / B - screen_pos.x / A) / 2;
    return Vector2(i, j);
}

// Interne Funktionen
bool GridManager::can_place_building(Vector2 top_left_cell, int building_size) const {
    if (top_left_cell.x < 0 || top_left_cell.y < 0 || top_left_cell.x + building_size > grid_size || top_left_cell.y + building_size > grid_size) {
        return false;
    }
    for (int x = 0; x < building_size; x++) {
        for (int y = 0; y < building_size; y++) {
            Vector2 cell = top_left_cell + Vector2(x, y);
            if (grid_occupancy.has(cell)) {
                return false;
            }
        }
    }
    return true;
}

bool GridManager::place_building(Vector2 top_left_cell, Ref<PackedScene> building_scene, int building_size) {
    if (!can_place_building(top_left_cell, building_size)) {
        return false;
    }
    Vector2 center_cell = top_left_cell + Vector2(building_size / 2.0, building_size / 2.0);
    Vector2 building_center = grid_to_screen(center_cell);
    Node2D* building_instance = Object::cast_to<Node2D>(building_scene->instantiate());
    if (building_instance) {
        building_instance->set_position(building_center);
        building_instance->set_z_index(0);
        add_child(building_instance);
        for (int x = 0; x < building_size; x++) {
            for (int y = 0; y < building_size; y++) {
                Vector2 cell = top_left_cell + Vector2(x, y);
                grid_occupancy[cell] = building_instance;
            }
        }
        update_z_index();
        return true;
    }
    return false;
}

bool GridManager::move_building(Node2D* building_instance, Vector2 new_top_left_cell, int building_size) {
    if (!can_place_building(new_top_left_cell, building_size)) {
        return false;
    }
    Array keys_to_remove;
    for (const Variant& key : grid_occupancy.keys()) {
        if (Object::cast_to<Node2D>(grid_occupancy[key]) == building_instance) {
            keys_to_remove.append(key);
        }
    }
    for (const Variant& key : keys_to_remove) {
        grid_occupancy.erase(key);
    }
    Vector2 center_cell = new_top_left_cell + Vector2(building_size / 2.0, building_size / 2.0);
    Vector2 building_center = grid_to_screen(center_cell);
    building_instance->set_position(building_center);
    building_instance->set_z_index(0);
    for (int x = 0; x < building_size; x++) {
        for (int y = 0; y < building_size; y++) {
            Vector2 cell = new_top_left_cell + Vector2(x, y);
            grid_occupancy[cell] = building_instance;
        }
    }
    update_z_index();
    if (last_active_state.size() > 0) {
        toggle_modes(last_active_state);
    }
    return true;
}

bool GridManager::can_spawn_troop(Vector2 screen_pos) const {
    Vector2 troop_cell = screen_to_grid(screen_pos);
    troop_cell = Vector2(floor(troop_cell.x), floor(troop_cell.y));
    for (const Variant& cell : grid_occupancy.keys()) {
        Vector2 occ_cell = cell;
        if (abs(troop_cell.x - occ_cell.x) <= 1 && abs(troop_cell.y - occ_cell.y) <= 1) {
            return false;
        }
    }
    return true;
}

bool GridManager::spawn_troop(Vector2 screen_pos, Ref<PackedScene> troop_scene) {
    if (!troop_scene.is_valid()) {
        ERR_PRINT("Keine troop_scene definiert!");
        return false;
    }
    if (!can_spawn_troop(screen_pos)) {
        return false;
    }
    Node2D* troop_instance = Object::cast_to<Node2D>(troop_scene->instantiate());
    if (troop_instance) {
        troop_instance->set_position(screen_pos);
        add_child(troop_instance);
        return true;
    }
    return false;
}

void GridManager::update_z_index() {
    float board_height = grid_to_screen(Vector2(grid_size, grid_size)).y;
    for (int i = 0; i < get_child_count(); i++) {
        Node2D* building = Object::cast_to<Node2D>(get_child(i));
        if (building) {
            float clamped_y = CLAMP(building->get_position().y, 0.0f, board_height);
            float t = clamped_y / board_height;
            building->set_z_index(Math::lerp(min_z_index, max_z_index, t));
        }
    }
}

void GridManager::toggle_modes(Dictionary p_last_active_state) {
    if (p_last_active_state.size() == 0) {
        ERR_PRINT("toggle_modes aufgerufen ohne gespeicherten Zustand!");
        return;
    }
    if (build_mode && !move_mode) {
        last_active_state = p_last_active_state.duplicate();
        build_mode = false;
        move_mode = true;
        print_line("Wechsle von Baumodus zu Verschiebemodus.");
    } else if (move_mode && !build_mode) {
        move_mode = false;
        if (last_active_state.size() != 0) {
            build_mode = true;
            print_line("Wechsle zurück in den Baumodus.");
        }
        last_active_state.clear();
    } else {
        ERR_PRINT(vformat("Ungültiger Modus-Status: build_mode = %s, move_mode = %s", build_mode, move_mode));
    }
}

void GridManager::_process(double delta) {
    if (grid_lines || show_occupied) {
        queue_redraw();
    }
}

void GridManager::_draw() {
    print_line("Draw grid");
    if (grid_lines) {
        // Print to console
        print_line("Draw grid lines");
        for (int i = 0; i < grid_size; i++) {
            for (int j = 0; j < grid_size; j++) {
                Vector2 center = grid_to_screen(Vector2(i + 0.5, j + 0.5));
                Vector2 top = center + Vector2(0, -cell_size.y / 2);
                Vector2 right = center + Vector2(cell_size.x / 2, 0);
                Vector2 bottom = center + Vector2(0, cell_size.y / 2);
                Vector2 left = center + Vector2(-cell_size.x / 2, 0);
                draw_line(top, right, Color(1, 0, 0), 1);
                draw_line(right, bottom, Color(1, 0, 0), 1);
                draw_line(bottom, left, Color(1, 0, 0), 1);
                draw_line(left, top, Color(1, 0, 0), 1);
            }
        }
    }
    if (show_occupied) {
        // Print to editor console
        print_line("Draw occupied cells");
        Dictionary drawn_yellow;
        for (const Variant& key : grid_occupancy.keys()) {
            Vector2 cell = key;
            Vector2 cell_center = grid_to_screen(cell + Vector2(0.5, 0.5));
            Vector2 points[4] = {
                cell_center + Vector2(0, -cell_size.y / 2),
                cell_center + Vector2(cell_size.x / 2, 0),
                cell_center + Vector2(0, cell_size.y / 2),
                cell_center + Vector2(-cell_size.x / 2, 0)
            };
            {
                PackedVector2Array poly_array;
                for (int k = 0; k < 4; k++) {
                    poly_array.push_back(points[k]);
                }
                draw_colored_polygon(poly_array, Color(0, 1, 0, 0.5));
            }

            Vector2 neighbor_offsets[8] = {
                Vector2(1, 0), Vector2(-1, 0), Vector2(0, 1), Vector2(0, -1),
                Vector2(1, 1), Vector2(1, -1), Vector2(-1, 1), Vector2(-1, -1)
            };
            for (int i = 0; i < 8; i++) {
                Vector2 neighbor = cell + neighbor_offsets[i];
                if (grid_occupancy.has(neighbor) || drawn_yellow.has(neighbor)) {
                    continue;
                }
                Vector2 neighbor_center = grid_to_screen(neighbor + Vector2(0.5, 0.5));
                Vector2 neighbor_points[4] = {
                    neighbor_center + Vector2(0, -cell_size.y / 2),
                    neighbor_center + Vector2(cell_size.x / 2, 0),
                    neighbor_center + Vector2(0, cell_size.y / 2),
                    neighbor_center + Vector2(-cell_size.x / 2, 0)
                };
                PackedVector2Array neighbor_poly;
                for (int k = 0; k < 4; k++) {
                    neighbor_poly.push_back(neighbor_points[k]);
                }
                draw_colored_polygon(neighbor_poly, Color(1, 1, 0, 0.5));
                drawn_yellow[neighbor] = true;
            }
        }
    }
}

Array GridManager::get_simple_path(Vector2 start_pos, Vector2 end_pos) {
    Vector2 start_grid = screen_to_grid(start_pos);
    Vector2 end_grid = screen_to_grid(end_pos);
    start_grid = Vector2(floor(start_grid.x), floor(start_grid.y));
    end_grid = Vector2(floor(end_grid.x), floor(end_grid.y));
    Array path;
    Vector2 diff = end_grid - start_grid;
    int steps = std::max(abs(diff.x), abs(diff.y));
    if (steps == 0) {
        path.append(grid_to_screen(start_grid + Vector2(0.5, 0.5)));
        return path;
    }
    Vector2 step = diff / static_cast<float>(steps);
    Vector2 current = start_grid;
    for (int i = 0; i <= steps; i++) {
        Vector2 cell = Vector2(round(current.x), round(current.y));
        path.append(grid_to_screen(cell + Vector2(0.5, 0.5)));
        current += step;
    }
    return path;
}

Array GridManager::get_simple_path_iso_avoid_walls(Vector2 start_pos, Vector2 end_pos) {
    // Start- und Endposition in Gitterkoordinaten umwandeln
    Vector2 start_grid = screen_to_grid(start_pos);
    Vector2 end_grid = screen_to_grid(end_pos);
    Vector2 start_cell = Vector2(std::floor(start_grid.x), std::floor(start_grid.y));
    Vector2 goal_cell = Vector2(std::floor(end_grid.x), std::floor(end_grid.y));

    // Wenn Start und Ziel identisch sind, leeren Pfad zurückgeben
    if (start_cell == goal_cell) {
        return Array();
    }

    // A*-Datenstrukturen initialisieren
    std::priority_queue<AStarNode> frontier;
    frontier.push({start_cell, 0.0f});
    std::unordered_map<Vector2, Vector2> came_from;
    std::unordered_map<Vector2, float> cost_so_far;
    came_from[start_cell] = start_cell;
    cost_so_far[start_cell] = 0.0f;

    // Heuristik: Manhattan-Distanz
    auto heuristic = [](Vector2 a, Vector2 b) {
        return std::abs(a.x - b.x) + std::abs(a.y - b.y);
    };

    // Bewegungsrichtungen (8-Wege-Bewegung: horizontal, vertikal, diagonal)
    Vector2 directions[8] = {
        Vector2(1, 0), Vector2(-1, 0), Vector2(0, 1), Vector2(0, -1),
        Vector2(1, 1), Vector2(1, -1), Vector2(-1, 1), Vector2(-1, -1)
    };

    // A*-Algorithmus
    while (!frontier.empty()) {
        Vector2 current = frontier.top().cell;
        frontier.pop();

        if (current == goal_cell) {
            break;
        }

        for (const auto& offset : directions) {
            Vector2 next = current + offset;
            // Prüfen, ob die Position außerhalb des Gitters liegt
            if (next.x < 0 || next.y < 0 || next.x >= grid_size || next.y >= grid_size) {
                continue;
            }
            // Prüfen, ob eine Wand im Weg ist
            if (grid_occupancy.has(next)) {
                Node2D* occupant = Object::cast_to<Node2D>(grid_occupancy[next]);
                if (occupant && occupant->has_method("is_wall") && occupant->call("is_wall")) {
                    continue;
                }
            }
            // Kosten berechnen (1 für gerade, 1.414 für diagonal)
            float new_cost = cost_so_far[current] + (offset.length() == 1 ? 1.0f : 1.414f);
            if (cost_so_far.find(next) == cost_so_far.end() || new_cost < cost_so_far[next]) {
                cost_so_far[next] = new_cost;
                float priority = new_cost + heuristic(next, goal_cell);
                frontier.push({next, priority});
                came_from[next] = current;
            }
        }
    }

    // Wenn kein Pfad gefunden wurde, leeres Array zurückgeben
    if (came_from.find(goal_cell) == came_from.end()) {
        return Array();
    }

    // Pfad rückwärts aufbauen
    Array grid_path;
    Vector2 current = goal_cell;
    while (current != start_cell) {
        grid_path.insert(0, current);
        current = came_from[current];
    }
    grid_path.insert(0, start_cell);

    // Pfad in Bildschirmkoordinaten umwandeln
    Array screen_path;
    for (const auto& cell : grid_path) {
        screen_path.append(grid_to_screen(static_cast<Vector2>(cell) + Vector2(0.5, 0.5)));
    }
    return screen_path;
}

Array GridManager::smooth_path(Array path, int segments) {
    // Wenn der Pfad zu kurz ist, unverändert zurückgeben
    if (path.size() < 2) {
        return path;
    }

    Array smoothed;
    for (int i = 0; i < path.size() - 1; i++) {
        // Kontrollpunkte für die Spline-Berechnung
        Vector2 p0 = (i == 0) ? (Vector2)path[i] : (Vector2)path[i - 1];
        Vector2 p1 = (Vector2)path[i];
        Vector2 p2 = (Vector2)path[i + 1];
        Vector2 p3 = (i + 2 < path.size()) ? (Vector2)path[i + 2] : p2;

        // Zwischenpunkte mit Catmull-Rom-Spline berechnen
        for (int j = 0; j < segments; j++) {
            float t = j / static_cast<float>(segments);
            float t2 = t * t;
            float t3 = t2 * t;
            Vector2 point = 0.5f * (
                (2 * p1) +
                (-p0 + p2) * t +
                (2 * p0 - 5 * p1 + 4 * p2 - p3) * t2 +
                (-p0 + 3 * p1 - 3 * p2 + p3) * t3
            );
            smoothed.append(point);
        }
    }
    // Letzten Punkt hinzufügen
    smoothed.append(path[path.size() - 1]);
    return smoothed;
}

Array GridManager::get_simple_path_iso_avoid_walls_smoothed(Vector2 start_pos, Vector2 end_pos) {
    // Einfachen Pfad berechnen
    Array path = get_simple_path_iso_avoid_walls(start_pos, end_pos);
    if (path.size() < 2) {
        return path;
    }
    // Pfad glätten mit 10 Segmenten pro Abschnitt
    return smooth_path(path, 10);
}

Array GridManager::get_simple_path_through_wall(Vector2 start_pos, Vector2 end_pos, float max_detour) {
    // Umwandeln in Gitterkoordinaten (Zellenpositionen)
    Vector2 start_grid = screen_to_grid(start_pos);
    Vector2 end_grid = screen_to_grid(end_pos);
    Vector2 start_cell = Vector2(floor(start_grid.x), floor(start_grid.y));
    Vector2 goal_cell = Vector2(floor(end_grid.x), floor(end_grid.y));

    // Falls Start und Ziel in derselben Zelle liegen
    if (start_cell == goal_cell) {
        Array path;
        path.append(grid_to_screen(start_cell + Vector2(0.5, 0.5)));
        return path;
    }

    // Versuche, einen Pfad zu finden, der Hindernisse umgeht.
    Array path_around = get_simple_path_iso_avoid_walls(start_pos, end_pos);
    float cost_around = 0.0f;
    if (path_around.size() > 1) {
        // Summiere die Distanzen zwischen den aufeinanderfolgenden Punkten
        for (int i = 1; i < path_around.size(); i++) {
            Vector2 p1 = (Vector2)path_around[i - 1];
            Vector2 p2 = (Vector2)path_around[i];
            cost_around += p1.distance_to(p2);
        }
    }

    // Direkte Entfernung zwischen Start- und Zielzelle (Mittelpunkt in Bildschirmkoordinaten)
    Vector2 start_center = grid_to_screen(start_cell + Vector2(0.5, 0.5));
    Vector2 goal_center = grid_to_screen(goal_cell + Vector2(0.5, 0.5));
    float direct_cost = start_center.distance_to(goal_center);

    // Wird der Umweg (zusätzliche Länge) akzeptiert?
    if ((cost_around - direct_cost) <= max_detour) {
        // Detour ist innerhalb der Toleranz, also gib den kompletten Umweichpfad zurück
        return path_around;
    } else {
        // Ansonsten: Ermittle den idealen Pfad, der Hindernisse ignoriert.
        Array path_ignore = get_simple_path_iso_ignore_walls(start_pos, end_pos);
        Array partial;
        // Durchlaufe den idealen Pfad, bis zur ersten Zelle, die belegt ist.
        for (int i = 0; i < path_ignore.size(); i++) {
            Vector2 screen_point = (Vector2)path_ignore[i];
            // Bestimme die zugehörige Gitterzelle (mittels Abschneiden der Nachkommastellen)
            Vector2 grid_pt = screen_to_grid(screen_point);
            grid_pt.x = floor(grid_pt.x);
            grid_pt.y = floor(grid_pt.y);
            if (grid_occupancy.has(grid_pt)) {
                // Sobald ein Hindernis gefunden wurde, abbrechen
                break;
            }
            partial.append(screen_point);
        }
        return partial;
    }
}

Array GridManager::get_simple_path_iso_ignore_walls(Vector2 start_pos, Vector2 end_pos) {
    // Umwandeln der Bildschirmkoordinaten in Gitterkoordinaten
    Vector2 start_grid = screen_to_grid(start_pos);
    Vector2 end_grid = screen_to_grid(end_pos);
    Vector2 start_cell = Vector2(floor(start_grid.x), floor(start_grid.y));
    Vector2 goal_cell = Vector2(floor(end_grid.x), floor(end_grid.y));

    // Falls Start und Ziel identisch sind, leeren Pfad zurückgeben
    if (start_cell == goal_cell) {
        return Array();
    }

    // A*-Algorithmus initialisieren
    std::priority_queue<AStarNode> frontier;
    frontier.push({start_cell, 0.0f});
    std::unordered_map<Vector2, Vector2> came_from;
    std::unordered_map<Vector2, float> cost_so_far;
    came_from[start_cell] = start_cell;
    cost_so_far[start_cell] = 0.0f;

    // Heuristik: Manhattan-Distanz
    auto heuristic = [](Vector2 a, Vector2 b) {
        return std::abs(a.x - b.x) + std::abs(a.y - b.y);
    };

    // 8-Wege-Bewegung
    Vector2 directions[8] = {
        Vector2(1, 0), Vector2(-1, 0), Vector2(0, 1), Vector2(0, -1),
        Vector2(1, 1), Vector2(1, -1), Vector2(-1, 1), Vector2(-1, -1)
    };

    // A*-Algorithmus ausführen, dabei werden keine Zellen als blockiert betrachtet
    while (!frontier.empty()) {
        Vector2 current = frontier.top().cell;
        frontier.pop();

        if (current == goal_cell) {
            break;
        }

        for (const auto &offset : directions) {
            Vector2 next = current + offset;
            // Gittergrenzen prüfen
            if (next.x < 0 || next.y < 0 || next.x >= grid_size || next.y >= grid_size) {
                continue;
            }
            // Kosten berechnen (1 für horizontal/vertikal, ~1.414 für diagonal)
            float new_cost = cost_so_far[current] + ((offset.length() == 1.0f) ? 1.0f : 1.414f);
            if (cost_so_far.find(next) == cost_so_far.end() || new_cost < cost_so_far[next]) {
                cost_so_far[next] = new_cost;
                float priority = new_cost + heuristic(next, goal_cell);
                frontier.push({next, priority});
                came_from[next] = current;
            }
        }
    }

    // Falls kein Pfad gefunden wurde, leeres Array zurückgeben
    if (came_from.find(goal_cell) == came_from.end()) {
        return Array();
    }

    // Pfad rückwärts rekonstruieren
    Array grid_path;
    Vector2 current = goal_cell;
    while (current != start_cell) {
        grid_path.insert(0, current);
        current = came_from[current];
    }
    grid_path.insert(0, start_cell);

    // Umwandeln der Gitterkoordinaten in Bildschirmkoordinaten (Zentrierung berücksichtigen)
    Array screen_path;
    for (int i = 0; i < grid_path.size(); i++) {
        Vector2 cell = grid_path[i];
        screen_path.append(grid_to_screen(cell + Vector2(0.5, 0.5)));
    }
    return screen_path;
}

Array GridManager::get_direct_path(Vector2 start_pos, Vector2 end_pos, int segments) {
    return Array(); // Platzhalter-Implementierung
}

Array GridManager::get_natural_path_through_wall(Vector2 start_pos, Vector2 end_pos) {
    // Bildschirmkoordinaten in Gitterkoordinaten umwandeln
    Vector2 start_grid = screen_to_grid(start_pos);
    Vector2 end_grid = screen_to_grid(end_pos);
    Vector2 start_cell = Vector2(floor(start_grid.x), floor(start_grid.y));
    Vector2 end_cell = Vector2(floor(end_grid.x), floor(end_grid.y));

    // Falls Start und Ziel in derselben Zelle liegen, direkten Mittelpunkt zurückgeben
    if (start_cell == end_cell) {
        Array path;
        path.append(grid_to_screen(start_cell + Vector2(0.5, 0.5)));
        return path;
    }

    // Zuerst Pfad ermitteln, der Hindernisse umgeht
    Array avoid_path = get_simple_path_iso_avoid_walls(start_pos, end_pos);
    if (avoid_path.size() > 0) {
        // Glätten des Pfades für einen natürlicheren Verlauf
        Array natural = smooth_path(avoid_path, 10);
        return natural;
    } else {
        // Falls kein Umweichpfad gefunden wurde:
        // Ermittle den idealen Pfad, der Hindernisse ignoriert
        Array ignore_path = get_simple_path_iso_ignore_walls(start_pos, end_pos);
        Array partial;
        // Füge Punkte so lange hinzu, bis ein Hindernis erreicht wird
        for (int i = 0; i < ignore_path.size(); i++) {
            Vector2 screen_point = (Vector2)ignore_path[i];
            // Bestimme die zugehörige Gitterzelle (mittels Abschneiden der Nachkommastellen)
            Vector2 grid_pt = screen_to_grid(screen_point);
            grid_pt.x = floor(grid_pt.x);
            grid_pt.y = floor(grid_pt.y);
            if (grid_occupancy.has(grid_pt)) {
                // Hindernis gefunden – Abbruch des Pfades
                break;
            }
            partial.append(screen_point);
        }
        // Glätten, falls ausreichend Punkte vorhanden sind
        if (partial.size() >= 2) {
            return smooth_path(partial, 10);
        }
        return partial;
    }
}

Dictionary GridManager::get_grid_occupancy() const {
    return grid_occupancy;
}

// Weitere Pfadfindungsmethoden können ähnlich implementiert werden (z. B. get_simple_path_iso, A*-basiert etc.).
// Für Kürze hier ausgelassen, aber die Logik bleibt gleich wie im GDScript.
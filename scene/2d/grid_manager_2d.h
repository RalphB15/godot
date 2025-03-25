#ifndef GRID_MANAGER_H
#define GRID_MANAGER_H

#include "scene/2d/node_2d.h"
#include "core/variant/dictionary.h"
#include "core/variant/array.h"
#include "core/typedefs.h"
#include "scene/resources/packed_scene.h"
#include <cmath>

class GridManager : public Node2D {
    GDCLASS(GridManager, Node2D);

    // Eigenschaften (mit @export Entsprechung)
    Vector2 cell_size = Vector2(32, 32);  // Basisgröße der Zelle
    int grid_size = 10;                   // Spielfeldgröße (quadratisch)
    int min_z_index = 0;                  // Minimaler Z-Index
    int max_z_index = 100;                // Maximaler Z-Index
    bool grid_lines = false;              // Gitterlinien anzeigen
    bool show_occupied = false;           // Belegte Zellen anzeigen

    // Interne Daten
    Node2D* selection = nullptr;          // Ausgewähltes Objekt
    bool build_mode = false;              // Baumodus-Flag
    bool move_mode = false;               // Verschiebemodus-Flag
    Dictionary last_active_state;         // Gespeicherter Zustand
    Dictionary grid_occupancy;            // Zellenbelegung
    Array last_path;                      // Letzter berechneter Pfad
    Vector2 last_path_init_point;         // Startpunkt des letzten Pfades

protected:
    static void _bind_methods();          // Methoden- und Eigenschaftsbindung

public:
    // Konstruktor/Destruktor
    GridManager();
    ~GridManager();

    // Interne Hilfsfunktionen
    bool can_place_building(Vector2 top_left_cell, int building_size) const;
    bool place_building(Vector2 top_left_cell, Ref<PackedScene> building_scene, int building_size);
    bool move_building(Node2D* building_instance, Vector2 new_top_left_cell, int building_size);
    bool can_spawn_troop(Vector2 screen_pos) const;
    bool spawn_troop(Vector2 screen_pos, Ref<PackedScene> troop_scene);
    void update_z_index();
    void toggle_modes(Dictionary p_last_active_state);

    // API-Methoden
    bool api_place_building(Vector2 top_left_cell, Ref<PackedScene> building_scene, int building_size);
    bool api_move_building(Node2D* building_instance, Vector2 new_top_left_cell, int building_size);
    bool api_spawn_troop(Vector2 screen_pos, Ref<PackedScene> troop_scene);
    void api_enable_build_mode();
    void api_enable_move_mode(Dictionary state);
    void api_disable_modes();
    bool api_get_build_mode() const;
    bool api_get_move_mode() const;
    Node2D* api_get_selection() const;
    void api_set_selection(Node2D* building_instance);
 
    // Utility-Funktionen
    Vector2 grid_to_screen(Vector2 grid_pos) const;
    Vector2 screen_to_grid(Vector2 screen_pos) const;

    // Pfadfindungsmethoden
    Array get_simple_path(Vector2 start_pos, Vector2 end_pos);
    Array get_simple_path_iso_avoid_walls(Vector2 start_pos, Vector2 end_pos);
    Array get_simple_path_iso_ignore_walls(Vector2 start_pos, Vector2 end_pos);
    Array smooth_path(Array path, int segments = 10);
    Array get_simple_path_iso_avoid_walls_smoothed(Vector2 start_pos, Vector2 end_pos);
    Array get_simple_path_through_wall(Vector2 start_pos, Vector2 end_pos, float max_detour);
    Array get_direct_path(Vector2 start_pos, Vector2 end_pos, int segments = 10);
    Array get_natural_path_through_wall(Vector2 start_pos, Vector2 end_pos);

    // Getter und Setter für Eigenschaften
    void set_cell_size(Vector2 size);
    Vector2 get_cell_size() const;
    void set_grid_size(int size);
    int get_grid_size() const;
    void set_min_z_index(int index);
    int get_min_z_index() const;
    void set_max_z_index(int index);
    int get_max_z_index() const;
    void set_grid_lines(bool enable);
    bool get_grid_lines() const;
    void set_show_occupied(bool enable);
    bool get_show_occupied() const;
    Dictionary get_grid_occupancy() const;

    // Godot-Methoden
    void _ready();

    // Überladene Godot-Methoden
    void _process(double delta);
    void _draw();
};

#endif // GRID_MANAGER_H
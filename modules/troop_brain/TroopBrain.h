#ifndef TROOP_BRAIN_H
#define TROOP_BRAIN_H

//#include "scene/2d/node_2d.h"
#include "scene/2d/grid_manager_2d.h"
#include "core/variant/array.h"
#include "core/math/vector2.h"
#include "core/variant/dictionary.h"
#include "core/object/ref_counted.h"

class TroopBrain : public RefCounted {
    GDCLASS(TroopBrain, RefCounted);

private:
    // Referenzen zur Truppe und zu GridManager (wird im Konstruktor gesetzt)
    Node2D *troop_unit = nullptr;
    GridManager *grid_manager = nullptr;

    // Ziel, Pfad und Pfadindex
    Node2D *attack_target = nullptr;
    Vector2 attack_target_point;
    Array path;
    int path_index = 0;

    // Konfigurationsparameter
    float detection_range = 100.0f;
    float attack_range = 50.0f;
    float speed = 100.0f;
    int max_building_path_length = 50;
    float max_detour = 5.0f;

protected:
    static void _bind_methods();

public:
    // Konstruktor und Initialisierung
    TroopBrain();
    void initialize(Node2D *p_troop_unit, GridManager *p_grid_manager);

    // Methode zum bewegen der Truppe
    void move(float delta);

    // Aktualisiert das Ziel (auf Basis von GridManager-Daten)
    void update_target();

    // Aktualisierung pro Frame (delta-Zeit)
    void update(float delta);

    // Hilfsmethode: Liefert aus einem idealen (ignore) Pfad das erste Wandziel (falls vorhanden)
    Node2D *get_first_wall_on_path(const Array &ignore_path);

    // Setter für Parameter (zum Beispiel aus GDScript, falls nötig)
    void set_detection_range(float p_range) { detection_range = p_range; }
    void set_attack_range(float p_range) { attack_range = p_range; }
    void set_speed(float p_speed) { speed = p_speed; }
    void set_max_building_path_length(int p_length) { max_building_path_length = p_length; }
    void set_max_detour(float p_detour) { max_detour = p_detour; }

    // Getter um via GDScript auf die Parameter zuzugreifen
    float get_detection_range() const { return detection_range; }
    float get_attack_range() const { return attack_range; }
    float get_speed() const { return speed; }
    int get_max_building_path_length() const { return max_building_path_length; }
    float get_max_detour() const { return max_detour; }
    Vector2 get_attack_target_point() const { return attack_target_point; }
    Node2D *get_attack_target() const { return attack_target; }
};

#endif // TROOP_BRAIN_H
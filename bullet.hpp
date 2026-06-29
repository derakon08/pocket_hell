#pragma once

#include <godot_cpp/classes/resource.hpp>

namespace godot {

class Bullet : public Resource {
    GDCLASS(Bullet, Resource)

protected:
    static void _bind_methods();

public:
    godot::Vector2 position;
    double speed = 0.0f;
    double lifetime = 0.0f;
    double rotation = 0.0f;
    double size = 0.0f;
    int sprite_in_atlas = 0;
    double angular_velocity = 0.0f;
    double visual_size = 1.0f;
    godot::String collision_group = "dummy";

    void set_bullet_pos(const Vector2& value);
    Vector2 get_bullet_pos() const;

    void set_bullet_speed(float value);
    float get_bullet_speed() const;

    void set_bullet_lifetime(float value);
    float get_bullet_lifetime() const;

    void set_bullet_rotation(float value);
    float get_bullet_rotation() const;

    void set_bullet_size(float value);
    float get_bullet_size() const;

    void set_sprite_in_atlas(int value);
    int get_sprite_in_atlas() const;

    void set_angular_velocity(float value);
    float get_angular_velocity() const;

    void set_visual_size(double value);
    double get_visual_size() const;

    void set_collision_group(String group);
    String get_collision_group() const;
};
};
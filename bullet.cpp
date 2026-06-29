#include "bullet.hpp"

using namespace godot;

void Bullet::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_bullet_pos", "value"), &Bullet::set_bullet_pos);
    ClassDB::bind_method(D_METHOD("get_bullet_pos"), &Bullet::get_bullet_pos);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "pos"), "set_bullet_pos", "get_bullet_pos");

    ClassDB::bind_method(D_METHOD("set_bullet_speed", "value"), &Bullet::set_bullet_speed);
    ClassDB::bind_method(D_METHOD("get_bullet_speed"), &Bullet::get_bullet_speed);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "speed"), "set_bullet_speed", "get_bullet_speed");

    ClassDB::bind_method(D_METHOD("set_bullet_lifetime", "value"), &Bullet::set_bullet_lifetime);
    ClassDB::bind_method(D_METHOD("get_bullet_lifetime"), &Bullet::get_bullet_lifetime);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "lifetime"), "set_bullet_lifetime", "get_bullet_lifetime");

    ClassDB::bind_method(D_METHOD("set_bullet_rotation", "value"), &Bullet::set_bullet_rotation);
    ClassDB::bind_method(D_METHOD("get_bullet_rotation"), &Bullet::get_bullet_rotation);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "rotation"), "set_bullet_rotation", "get_bullet_rotation");

    ClassDB::bind_method(D_METHOD("set_bullet_size", "value"), &Bullet::set_bullet_size);
    ClassDB::bind_method(D_METHOD("get_bullet_size"), &Bullet::get_bullet_size);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "size"), "set_bullet_size", "get_bullet_size");

    ClassDB::bind_method(D_METHOD("set_sprite_in_atlas", "value"), &Bullet::set_sprite_in_atlas);
    ClassDB::bind_method(D_METHOD("get_sprite_in_atlas"), &Bullet::get_sprite_in_atlas);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "sprite_in_atlas"), "set_sprite_in_atlas", "get_sprite_in_atlas");

    ClassDB::bind_method(D_METHOD("set_angular_velocity", "value"), &Bullet::set_angular_velocity);
    ClassDB::bind_method(D_METHOD("get_angular_velocity"), &Bullet::get_angular_velocity);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "angular_velocity"), "set_angular_velocity", "get_angular_velocity");

    ClassDB::bind_method(D_METHOD("set_visual_size", "value"), &Bullet::set_visual_size);
    ClassDB::bind_method(D_METHOD("get_visual_size"), &Bullet::get_visual_size);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "visual_size"), "set_visual_size", "get_visual_size");

    ClassDB::bind_method(D_METHOD("set_collision_group", "value"), &Bullet::set_collision_group);
    ClassDB::bind_method(D_METHOD("get_collision_group"), &Bullet::get_collision_group);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "collision_group"), "set_collision_group", "get_collision_group");
}

void Bullet::set_bullet_pos(const Vector2& value) { position = value; }
Vector2 Bullet::get_bullet_pos() const { return position; }

void Bullet::set_bullet_speed(float value) { speed = value; }
float Bullet::get_bullet_speed() const { return speed; }

void Bullet::set_bullet_lifetime(float value) { lifetime = value; }
float Bullet::get_bullet_lifetime() const { return lifetime; }

void Bullet::set_bullet_rotation(float value) { rotation = value; }
float Bullet::get_bullet_rotation() const { return rotation; }

void Bullet::set_bullet_size(float value) { size = value; }
float Bullet::get_bullet_size() const { return size; }

void Bullet::set_sprite_in_atlas(int value) { sprite_in_atlas = value; }
int Bullet::get_sprite_in_atlas() const { return sprite_in_atlas; }

void Bullet::set_angular_velocity(float value) { angular_velocity = value; }
float Bullet::get_angular_velocity() const { return angular_velocity; }

void Bullet::set_visual_size(double value) { visual_size = value; }
double Bullet::get_visual_size() const { return visual_size; }

void Bullet::set_collision_group(String group) { collision_group = group; }
String Bullet::get_collision_group() const { return collision_group; }
//This script relies on enemies having a Hit(godot::Vector2) method. Bullet map is a bullet manager which will set the position of bullets shot by using the function Shoot
#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include "bullet.hpp"
#include <godot_cpp/classes/multi_mesh_instance2d.hpp>
#include <godot_cpp/classes/multi_mesh.hpp>

namespace godot {

class BulletMap : public godot::MultiMeshInstance2D {
	GDCLASS(BulletMap, godot::MultiMeshInstance2D)

public:
    enum MovementType { MOVEMENT_NORMAL, MOVEMENT_NORMAL_NORENDER, MOVEMENT_COUNT };
	enum VisualsType {VISUALS_COUNT, VISUALS_NONE }; //VISUALS_NONE point the Shoot() method to assign to MovementDefault aka the one with built-in visuals
	enum BulletData { BULLET_DATA_ORIGIN, BULLET_DATA_SPEED, BULLET_DATA_SIZE,
					  BULLET_DATA_ROTATION, BULLET_DATA_ROTATION_DEGREES,
					  BULLET_DATA_LIFETIME,
					  BULLET_DATA_ANGULAR_VEL, BULLET_DATA_ANGULAR_VEL_DEGREES,
					  BULLET_DATA_COLLISION_GROUP, BULLET_DATA_COLLISION_GROUP_STRING,
					  BULLET_DATA_SPRITE_INDEX,
					  BULLET_DATA_VISUALS_SIZE,
					  BULLET_DATA_COUNT };





private:
	//bullet data
	godot::PackedFloat32Array bullet_buffer;
	std::vector<double> bullet_size;
	std::vector<double> bullet_angular_velocity;
	std::vector<double> bullet_rotation;
	std::vector<double> bullet_lifetime;
	std::vector<double> bullet_speed;


	//collision variables
	std::vector<int> bullet_collision_group;
	std::vector<std::vector<std::array<double, 2>>> collision_group_node_positions;
	std::vector<std::vector<double>> collision_group_node_radius;
	std::vector<std::vector<godot::Node2D*>> collision_group_nodes;

	std::vector<std::string> collision_group_names;
	std::unordered_map<std::string, int> collision_groups;

	//Atlas variables
	int sprites_per_atlas_row;
	std::vector<int> bullet_sprite_index;
	std::vector<double> bullet_visuals_size;
	

	//utilities
	static const Vector2 vector2_right;
	Ref<godot::MultiMesh> multim;
	std::vector<int> bullet_instance;
	std::vector<int> dead_bullets;
	std::vector<int> active_bullets;
	//Godot RenderingServer docs
	//Position + Custom data: 12 floats (8 floats for Transform2D, 4 floats of custom data)
	static constexpr int buffer_unit = 12;
	int pool_size;


	//state flags
	bool allow_shooting = false;
	bool paused = true;
	bool clearing_bullets = false;


	//Bullet buckets
	//set manually. PS they need to be in the same order as the enums
	std::vector<std::vector<int>> visuals_buckets;
	std::vector<std::vector<int>> movement_buckets;

	using BucketFunc = void (BulletMap::*)(double, int);
	
	BucketFunc movement_type_methods[BulletMap::MovementType::MOVEMENT_COUNT] = {
		&BulletMap::MovementDefault,
		&BulletMap::MovementDefaultNoRender
	};

	BucketFunc visuals_type_methods[BulletMap::VisualsType::VISUALS_COUNT] = {
		
	};

	
	void BulletCollision();
	void ManageBulletLifetimes(double delta);
	//Resizing is both expensive, and will introduce bugs... apparently. Anyway, every resize resets all instance custom data and flickers if not handled correctly
	void IncreaseMultimeshInstanceCount();
	//Called at ready(), will check every movement type and append an array for each
	void SetupBuckets();
	bool OverlapsArea(double vec[2], double collision_area);
	void MovementDefault(double delta, int bucket_index);
	void MovementDefaultNoRender(double delta, int bucket_index);


public:
	BulletMap();
	~BulletMap();

	int preloaded_pool_size;
	void set_preloaded_pool_size(int value) {preloaded_pool_size = value; }
	int get_preloaded_pool_size() const {return preloaded_pool_size; }


	godot::Vector2 sprite_size;
	void set_sprite_size(Vector2 value) { sprite_size = value; }
	Vector2 get_sprite_size() const {return sprite_size; }


	int get_pool_size() {return pool_size; }
	int get_bullet_count() const { return active_bullets.size(); }
	void _ready() override;
	void _physics_process(double delta) override;
	void _process(double delta) override;

	//Returns the BULLET ID in the shape of a Vector2, both numbers are necessary to ensure modification is possible.
	//Where s = sprites in line: (x + sy) = sprite in atlas.
	//Collision offset : godot::Vector2(push back, size reduction)	
	Vector2i Shoot(Ref<Bullet> bullet, MovementType movement_type, VisualsType visuals_type);
	//this gets you anything about the movement. Excludes collision variables and sprite stuff.
	double GetBulletBasicData(Vector2i bullet_id, BulletData what_data);
	//returns x,y,rotation
	Vector3 GetBulletPosition(Vector2i bullet_id);
	//array of BulletData, of course
	bool SwapBulletData(Vector2i bullet_id, Ref<Bullet> new_bullet, Array what_data);
	//Dead bullets stay in memory to be recycled, ResetPoolSize is meant as a literal way of clean up. 
	void ResetPoolSize();
	void Reset();
	void ClearMap(int speed = 0);
	bool AddNewCollisionGroup(const String& group_name);
	//(This can easily lead to problem, so unless you're sure that nothing is checking the specific group, don't use it often)
	//Remove a collision group from the registered collision groups. It affects every array related to collision and collision cheking
	bool RemoveCollisionGroup(const String& group_name);
	bool AddObjective(const String& group_name, Node2D* node, double hitbox_radius);
	bool RemoveObjective(const String& group_name, const Node2D* node);
	//Stop processing on this script
	void Pause();
	//Use to resume processing 
	void Unpause();
	//Use to stop/allow shooting for external scripts. Use IsClearingBullets() to avoid desync
	void AllowShooting(bool allowed);
	bool IsClearingBullets() { return clearing_bullets; }
	bool ValidateBulletInstance(const godot::Vector2i bullet_id) { return bullet_instance[bullet_id.x] == bullet_id.y; }




protected:
    static void _bind_methods();
	
}; //class
} //godot

VARIANT_ENUM_CAST(BulletMap::MovementType);
VARIANT_ENUM_CAST(BulletMap::BulletData);
VARIANT_ENUM_CAST(BulletMap::VisualsType);
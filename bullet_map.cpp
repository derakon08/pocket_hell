//#include <unordered_map>
//#include <vector>
#include <cmath>
#include <iostream>
#include "bullet_map.hpp"
#include <godot_cpp/classes/canvas_texture.hpp>
#include <godot_cpp/classes/engine.hpp>

using namespace godot;

void BulletMap::_bind_methods() {
    ClassDB::bind_method(D_METHOD("Shoot", "bullet", "movement_type", "visuals_type"), &BulletMap::Shoot);
	ClassDB::bind_method(D_METHOD("ClearMap", "speed"), &BulletMap::ClearMap);
    ClassDB::bind_method(D_METHOD("AddNewCollisionGroup", "group_name"), &BulletMap::AddNewCollisionGroup);
    ClassDB::bind_method(D_METHOD("AddObjective", "group_name", "node", "hitbox_radius"), &BulletMap::AddObjective);
	ClassDB::bind_method(D_METHOD("RemoveCollisionGroup", "group_name"), &BulletMap::RemoveCollisionGroup);
    ClassDB::bind_method(D_METHOD("RemoveObjective", "group_name", "node"), &BulletMap::RemoveObjective);
	ClassDB::bind_method(D_METHOD("GetBulletBasicData", "bullet_id", "what_data"), &BulletMap::GetBulletBasicData);
	ClassDB::bind_method(D_METHOD("GetBulletPosition", "bullet_id"), &BulletMap::GetBulletPosition);
	ClassDB::bind_method(D_METHOD("SwapBulletData", "bullet_id", "new_bullet", "what_data"), &BulletMap::SwapBulletData);
	ClassDB::bind_method(D_METHOD("Reset"), &BulletMap::Reset);
	ClassDB::bind_method(D_METHOD("Pause"), &BulletMap::Pause);
	ClassDB::bind_method(D_METHOD("Unpause"), &BulletMap::Unpause);

	ClassDB::bind_method(D_METHOD("get_pool_size"), &BulletMap::get_pool_size);
	ClassDB::bind_method(D_METHOD("get_bullet_count"), &BulletMap::get_bullet_count);

	ClassDB::bind_method(D_METHOD("set_preloaded_pool_size", "value"), &BulletMap::set_preloaded_pool_size);
	ClassDB::bind_method(D_METHOD("get_preloaded_pool_size"), &BulletMap::get_preloaded_pool_size);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "preloaded_pool_size"), "set_preloaded_pool_size", "get_preloaded_pool_size");

	ClassDB::bind_method(D_METHOD("set_sprite_size", "value"), &BulletMap::set_sprite_size);
	ClassDB::bind_method(D_METHOD("get_sprite_size"), &BulletMap::get_sprite_size);
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "sprite_size"), "set_sprite_size", "get_sprite_size");

	ClassDB::bind_method(D_METHOD("set_game_area", "value"), &BulletMap::set_game_area);
	ClassDB::bind_method(D_METHOD("get_game_area"), &BulletMap::get_game_area);
	ADD_PROPERTY(PropertyInfo(Variant::RECT2, "game_area"), "set_game_area", "get_game_area");

	BIND_ENUM_CONSTANT(MOVEMENT_NORMAL);
	BIND_ENUM_CONSTANT(MOVEMENT_NORMAL_NORENDER);
	BIND_ENUM_CONSTANT(BULLET_DATA_ORIGIN);
	BIND_ENUM_CONSTANT(BULLET_DATA_SPEED);
	BIND_ENUM_CONSTANT(BULLET_DATA_SIZE);
	BIND_ENUM_CONSTANT(BULLET_DATA_ROTATION);
	BIND_ENUM_CONSTANT(BULLET_DATA_ROTATION_DEGREES);
	BIND_ENUM_CONSTANT(BULLET_DATA_LIFETIME);
	BIND_ENUM_CONSTANT(BULLET_DATA_ANGULAR_VEL);
	BIND_ENUM_CONSTANT(BULLET_DATA_ANGULAR_VEL_DEGREES);
	BIND_ENUM_CONSTANT(BULLET_DATA_COLLISION_GROUP);
	BIND_ENUM_CONSTANT(BULLET_DATA_COLLISION_GROUP_STRING);
	BIND_ENUM_CONSTANT(BULLET_DATA_SPRITE_INDEX);
	BIND_ENUM_CONSTANT(BULLET_DATA_VISUALS_SIZE);
	BIND_ENUM_CONSTANT(VISUALS_NONE);

	ADD_SIGNAL(MethodInfo("spawner_cleared"));
}

const godot::Vector2 BulletMap::vector2_right = godot::Vector2(1.0, 0.0);


BulletMap::BulletMap() {}
BulletMap::~BulletMap() {}




//Private functions
void BulletMap::_ready() {
	if (Engine::get_singleton()->is_editor_hint())
		{ 
			this->set_physics_process(false);
			this->set_process(false);
			return;
		}
	AddNewCollisionGroup("dummy");

	if (Engine::get_singleton()->is_editor_hint()) {
        set_process(false);
        set_physics_process(false);
        return;
    }

	multim = this->get_multimesh();
	multim->set_instance_count(0);
	multim->set_visible_instance_count(-1);
	multim->set_use_custom_data(true);

	if (this->get_texture() == nullptr) {
		this->set_texture(Ref<godot::CanvasTexture>());
		sprites_per_atlas_row = 1;
	} else {
		Vector2 atlas_size = this->get_texture()->get_size();
		sprites_per_atlas_row = int(atlas_size.x / sprite_size.x);
		sprite_size /= atlas_size;
	}

	if(multim->is_using_colors()) {
		//the order of transform-colors-custom data really messes it up
		//alternative: bullet_index + buffer_unit - slot to set custom data without fault
		multim->set_use_colors(false); // i honestly don't even know what vertex colors do so i'll just disable it anyway
	}
	
	SetupBuckets();
	PopulateBuckets(preloaded_pool_size);
	ResetPoolSize();

	paused = false;
	allow_shooting = true;
}

void BulletMap::_physics_process(double delta) {
	if (!paused) {
		ManageBulletLifetimes(delta);

		for (int method = 0; method < MovementType::MOVEMENT_COUNT; method++) {
			(this->*movement_type_methods[method])(delta, method);
		}
	}
}


void BulletMap::_process(double delta) {
	if (!paused) {
		Vector2 godot_vec = Vector2();
		for (int group = 0; group < collision_group_nodes.size(); group++){
			for (int node = 0; node < collision_group_nodes[group].size(); node++){
				godot_vec = collision_group_nodes[group][node]->get_global_position();
				collision_group_node_positions[group][node] = {godot_vec.x, godot_vec.y};
			}
		}

		BulletCollision();

		multim->set_buffer(bullet_buffer);
	}
}


void BulletMap::BulletCollision() {
	const float* buffer = bullet_buffer.ptr();
    int collision_group;
    double bullet_origin[2];
    double collision_area;
	double vec_difference[2];
	int array_size;

    for (int index : active_bullets) {
        collision_group = bullet_collision_group[index];
        bullet_origin[0] = buffer[index * buffer_unit + 3];
		bullet_origin[1] = buffer[index * buffer_unit + 7];
        collision_area = bullet_size[index];

        const std::vector<std::array<double, 2>>& nodes_position = collision_group_node_positions[collision_group];
        const std::vector<double>& nodes_radius = collision_group_node_radius[collision_group];

		array_size = collision_group_nodes[collision_group].size();

        for (int node_index = 0; node_index < array_size; node_index++) {
			vec_difference[0] = bullet_origin[0] - nodes_position[node_index][0];
			vec_difference[1] = bullet_origin[1] - nodes_position[node_index][1];

            if (OverlapsArea(vec_difference, nodes_radius[node_index] + collision_area * 0.5)) {
                collision_group_nodes[collision_group][node_index]->call("Hit", Vector2(index, bullet_instance[index]));
            }
        }
    }
}


void BulletMap::ManageBulletLifetimes(double delta) {
	float* buffer = bullet_buffer.ptrw();
	int bullet_index;
	int live_array_size = active_bullets.size() -1;
	float bullet_radius;

	for (int index = live_array_size; index > -1; index--) {
		bullet_index = active_bullets[index];

		if (bullet_lifetime[bullet_index] < 0) { //if -1 then it means it never dies on screen, else it can go offscreen
			bullet_radius = bullet_visuals_size[bullet_index];

			if (buffer[bullet_index * buffer_unit + 3] > game_area_left + bullet_radius && buffer[bullet_index * buffer_unit + 3] < game_area_right + bullet_radius &&
				buffer[bullet_index * buffer_unit + 7] > game_area_top + bullet_radius && buffer[bullet_index * buffer_unit + 7] < game_area_bottom + bullet_radius)
				{ continue; }

		} else if (bullet_lifetime[bullet_index] > delta) { //checking if the bullet can get past this frame without dying (avoids setting values lower than 0)
			bullet_lifetime[bullet_index] -= delta;
			continue;
		}
		
		bullet_lifetime[bullet_index] = 0;
		bullet_instance[bullet_index] += 1;
		dead_bullets.push_back(bullet_index);
		active_bullets[index] = active_bullets[live_array_size];
		live_array_size--;

		for (int access = bullet_index * buffer_unit; access < (bullet_index * buffer_unit) + buffer_unit; access++) {
			buffer[access] = 0;
		}
		
	}

	active_bullets.resize(live_array_size + 1);
}


//Resizing is both expensive, and will introduce bugs... apparently. Anyway, every resize resets all instance custom data and flickers if not handled correctly
void BulletMap::IncreaseMultimeshInstanceCount() {
	int instance_count = multim->get_instance_count();

	if (instance_count > 0)
		{multim->set_instance_count(instance_count * 2);} //I believe this makes sense, as the more you resize, the more likely it is that you're using an insane amount of bullets
	else
		{multim->set_instance_count(100);}
	
	instance_count = multim->get_instance_count();

	bullet_buffer.resize(instance_count * buffer_unit);

	bullet_angular_velocity.resize(instance_count);
	bullet_sprite_index.resize(instance_count);
	bullet_visuals_size.resize(instance_count);
	bullet_lifetime.resize(instance_count);
	bullet_rotation.resize(instance_count);
	bullet_speed.resize(instance_count);
	bullet_size.resize(instance_count);

	bullet_collision_group.resize(instance_count);
	bullet_instance.resize(instance_count);

	active_bullets.reserve(instance_count);
	dead_bullets.reserve(instance_count);

	float* buffer = bullet_buffer.ptrw();

	PopulateBuckets(instance_count);

	for (int bullet_number :active_bullets){
		buffer[bullet_number * buffer_unit + 8] = (bullet_sprite_index[bullet_number] % sprites_per_atlas_row);
		buffer[bullet_number * buffer_unit + 9] = (bullet_sprite_index[bullet_number] / sprites_per_atlas_row);
		buffer[bullet_number * buffer_unit + 10] = sprite_size.x;
		buffer[bullet_number * buffer_unit + 11] = sprite_size.y;
	}

	multim->set_buffer(bullet_buffer);
}


void BulletMap::ClearMap(const int speed) {
	if (speed < 0) {
		return;
	}

	if (speed > 0) {	
		//allow_shooting = false;
		//paused = true;
		//clearing_bullets = true;

		//set new variables about how many bullets to clear per _process call
	}
	else {
		for (int bullet_index :active_bullets) {
			bullet_lifetime[bullet_index] = 0;

			for (int buffer_index = bullet_index * buffer_unit; buffer_index < ((bullet_index * buffer_unit) + buffer_unit); buffer_index++) //i wonder if disabling all at once would be better...
				{ bullet_buffer[buffer_index] = 0; }
		}
	}
}


bool BulletMap::OverlapsArea(double vec[2], double collision_area) {
	return (vec[0] * vec[0] + vec[1] * vec[1]) < collision_area * collision_area;
}


//Called at ready(), will check every movement type and push_back an array for each
void BulletMap::SetupBuckets() {
	movement_buckets.resize(MovementType::MOVEMENT_COUNT);
	visuals_buckets.resize(VisualsType::VISUALS_COUNT);

	for (int type = 0; type < MovementType::MOVEMENT_COUNT; type++) {
		movement_buckets[type] = {};
	}

	for (int type = 0; type < VisualsType::VISUALS_COUNT; type++) {
		visuals_buckets[type] = {};
	}
}


void BulletMap::PopulateBuckets(int reserve) {
	for (int type = 0; type < MovementType::MOVEMENT_COUNT; type++) {
		movement_buckets[type].reserve(reserve);
	}

	for (int type = 0; type < VisualsType::VISUALS_COUNT; type++) {
		visuals_buckets[type].reserve(reserve);
	}
}







//Public methods

//Returns the BULLET ID in the shape of a Vector2, both numbers are necessary to ensure modification is possible.
//Where s = sprites in line: (x + sy) = sprite in atlas.
//Collision offset : Vector2(push back, size reduction)
Vector2i BulletMap::Shoot(Ref<Bullet> bullet, MovementType movement_type = MovementType::MOVEMENT_NORMAL, VisualsType visuals_type = VisualsType::VISUALS_NONE) { 
	if (!allow_shooting) { return Vector2i(); }
	
	int i;
	double cosine = std::cos(bullet->rotation) * bullet->visual_size;
	double sine = std::sin(bullet->rotation) * bullet->visual_size;

	if (!dead_bullets.empty()){
		i = dead_bullets[dead_bullets.size() -1];
		dead_bullets.pop_back();
	}
	else {
		i = pool_size;
		pool_size += 1;

		if (multim->get_instance_count() < pool_size)
			{ IncreaseMultimeshInstanceCount(); }
	}

	bullet_angular_velocity[i] = bullet->angular_velocity;
	bullet_sprite_index[i] = bullet->sprite_in_atlas;
	bullet_visuals_size[i] = bullet->visual_size;
	bullet_lifetime[i] = (bullet->clip_bullet)? -1 : bullet->lifetime;
	bullet_rotation[i] = bullet->rotation;
	bullet_speed[i] = bullet->speed;
	bullet_size[i] = bullet->size;

	bullet_collision_group[i] = collision_groups[bullet->collision_group.utf8().get_data()];
	

	if (visuals_type != VisualsType::VISUALS_NONE) { visuals_buckets[visuals_type].push_back(i); }

	movement_buckets[movement_type].push_back(i);
	active_bullets.push_back(i);

	bullet_buffer[i * buffer_unit] = cosine; //x.x
	bullet_buffer[i * buffer_unit + 1] = -sine; //y.x
	//bullet_buffer[i * buffer_unit + 2];
	bullet_buffer[i * buffer_unit + 3] = bullet->position.x;
	bullet_buffer[i * buffer_unit + 4] = sine; //x.y
	bullet_buffer[i * buffer_unit + 5] = cosine; //y.y
	//bullet_buffer[i * buffer_unit + 6];
	bullet_buffer[i * buffer_unit + 7] = bullet->position.y;
	bullet_buffer[i * buffer_unit + 8] = (bullet_sprite_index[i] % sprites_per_atlas_row);
	bullet_buffer[i * buffer_unit + 9] = (bullet_sprite_index[i] / sprites_per_atlas_row);
	bullet_buffer[i * buffer_unit + 10] = sprite_size.x;
	bullet_buffer[i * buffer_unit + 11] = sprite_size.y;


	return godot::Vector2i(i, bullet_instance[i]);
}



double BulletMap::GetBulletBasicData(Vector2i bullet_id, BulletData what_data) {
	if (!ValidateBulletInstance(bullet_id))
		{ return NAN; }

	switch (what_data) {
		case BulletData::BULLET_DATA_LIFETIME:
			return bullet_lifetime[bullet_id.x];

		case BulletData::BULLET_DATA_SPEED:
			return bullet_speed[bullet_id.x];

		case BulletData::BULLET_DATA_ROTATION:
			return bullet_rotation[bullet_id.x];

		case BulletData::BULLET_DATA_ROTATION_DEGREES:
			return Math::rad_to_deg(bullet_rotation[bullet_id.x]);

		case BulletData::BULLET_DATA_ANGULAR_VEL:
			return bullet_angular_velocity[bullet_id.x];

		case BulletData::BULLET_DATA_ANGULAR_VEL_DEGREES:
			return Math::rad_to_deg(bullet_angular_velocity[bullet_id.x]);

		case BulletData::BULLET_DATA_SIZE:
			return bullet_size[bullet_id.x];
		
		case BulletData::BULLET_DATA_ORIGIN:
			UtilityFunctions::push_error("GetBulletBasicData: Use GetBulletPosition to get a bullet's origin instead");
			return NAN;

		default:
			UtilityFunctions::push_error("GetBulletBasicData: Method only accepts:\nSPEED\nROTATION\nSIZE\nROTATION_DEGREES\nLIFETIME\nANGULAR_VEL\nANGULAR_VEL_DEGREES");
			return NAN;
	}
}


Vector3 BulletMap::GetBulletPosition(Vector2i bullet_id) {
	if (!ValidateBulletInstance(bullet_id)) { return Vector3(); }

	return Vector3(bullet_buffer[bullet_id.x * buffer_unit + 3], bullet_buffer[bullet_id.x * buffer_unit + 7], bullet_rotation[bullet_id.x]);
}


//prefer calling a new Shoot() bullet, unless you only need to change one or two things.
//Only changes a few parts of the bullet
bool BulletMap::SwapBulletData(Vector2i bullet_id, Ref<Bullet> new_bullet, Array what_data) {
	if (!ValidateBulletInstance(bullet_id)) { return false; }

	int what_data_size = what_data.size();
	for (int i = 0; i < what_data_size; i++) {
		int parsedval = static_cast<BulletData>((int)what_data[i]);
		
        ERR_FAIL_COND_V_MSG(parsedval < BulletData::BULLET_DATA_ORIGIN || parsedval > BulletData::BULLET_DATA_COUNT, false, "SwapBullet: invalid value passed in fields. Expected BulletMap.BulletData");

		switch (parsedval) {
			case BulletData::BULLET_DATA_VISUALS_SIZE:
				bullet_visuals_size[bullet_id.x] = new_bullet->visual_size;
				break;
			case BulletData::BULLET_DATA_SIZE:
				bullet_size[bullet_id.x] = new_bullet->size;
				break;
			case BulletData::BULLET_DATA_LIFETIME:
				bullet_lifetime[bullet_id.x] = new_bullet->lifetime;
				break;

			case BulletData::BULLET_DATA_SPEED:
				bullet_speed[bullet_id.x] = new_bullet->speed;
				break;

			case BulletData::BULLET_DATA_SPRITE_INDEX:
				bullet_sprite_index[bullet_id.x] = new_bullet->sprite_in_atlas;
				break;

			case BulletData::BULLET_DATA_COLLISION_GROUP:
				bullet_collision_group[bullet_id.x] = collision_groups[new_bullet->collision_group.utf8().get_data()];
				break;

			default:
				UtilityFunctions::push_error("SwapBullet: data swap not implemented: " + itos((int)parsedval));
				break;
		}
	}

	return true;
}


//Dead bullets stay in memory to be recycled, ResetPoolSize is meant as a literal way of clean up.
void BulletMap::ResetPoolSize() {
	for (std::vector<int>& bucket :movement_buckets) {
		bucket.clear(); }


	bullet_visuals_size.resize(preloaded_pool_size);
	bullet_angular_velocity.resize(preloaded_pool_size);
	bullet_collision_group.resize(preloaded_pool_size);
	bullet_sprite_index.resize(preloaded_pool_size);
	bullet_instance.resize(preloaded_pool_size);
	bullet_rotation.resize(preloaded_pool_size);
	bullet_lifetime.resize(preloaded_pool_size);
	bullet_speed.resize(preloaded_pool_size);
	bullet_size.resize(preloaded_pool_size);

	
	multim->set_instance_count(preloaded_pool_size); //set the instance count first
	pool_size = preloaded_pool_size;

	bullet_buffer.resize(preloaded_pool_size * buffer_unit);
	dead_bullets.resize(preloaded_pool_size);
	active_bullets.reserve(preloaded_pool_size);

	active_bullets.clear();

	for (int index = 0; index < preloaded_pool_size; index++) {
		for (int buffer_index = index * buffer_unit; buffer_index < ((index * buffer_unit) + buffer_unit); buffer_index++)
			{ bullet_buffer[buffer_index] = 0.0f; }
		
		bullet_lifetime[index] = 0;
		bullet_instance[index] = 0;
		dead_bullets[index] = index;
	}
}


void BulletMap::Reset() {
	ResetPoolSize();

	for (int array = 0; array < collision_group_nodes.size(); array++) {
		collision_group_nodes[array].clear();
		collision_group_node_radius[array].clear();
		collision_group_node_positions[array].clear();
	}
}


bool BulletMap::AddNewCollisionGroup(const String& group_name) {
	const std::string group_string = group_name.utf8().get_data();

	if (collision_groups.find(group_string) != collision_groups.end()) {
		UtilityFunctions::print("Group already exists: " + group_name);
		return false;
	}

	collision_groups[group_string] = collision_group_nodes.size();
	collision_group_names.push_back(group_string);
	
	collision_group_node_positions.push_back(std::vector<std::array<double, 2>>());
	collision_group_node_radius.push_back(std::vector<double>());
	collision_group_nodes.push_back(std::vector<Node2D*>());

	return true;
}


//Remove a collision group from the registered collision groups. It affects every array related to collision and collision cheking
bool BulletMap::RemoveCollisionGroup(const String& group_name){
	const std::string group_string = group_name.utf8().get_data();
	std::unordered_map<std::string,int>::const_iterator collision_group = collision_groups.find(group_string);
	
	if (collision_group != collision_groups.end() && collision_group->second != 0) {
		int group_index = collision_group->second;

		if (group_string != collision_group_names[collision_group_names.size() - 1]) { //if not last group
			collision_groups[collision_group_names[collision_group_names.size() - 1]] = group_index; //overwrite the group to delete with the last
			collision_group_nodes[group_index] = collision_group_nodes[collision_group_nodes.size() - 1];
			collision_group_node_radius[group_index] = collision_group_node_radius[collision_group_node_radius.size() - 1];
			collision_group_node_positions[group_index] = collision_group_node_positions[collision_group_node_positions.size() - 1];
		}

		collision_group_nodes.pop_back();
		collision_group_node_radius.pop_back();
		collision_group_node_positions.pop_back();
		collision_group_names.pop_back();

		collision_groups.erase(group_string);
		
		return true;
	}
	else {
		UtilityFunctions::push_error("Invalid collision group to remove");
		return false;
	}
}


bool BulletMap::AddObjective(const String& group_name, Node2D* node, double hitbox_radius) {
	std::unordered_map<std::string,int>::const_iterator collision_group = collision_groups.find(group_name.utf8().get_data());

	if (collision_group == collision_groups.end()) {
		UtilityFunctions::push_warning("Invalid collision group for new node: " + group_name);
		return false;
	}

	int group_index = collision_group->second;

	collision_group_nodes[group_index].push_back(node);
	collision_group_node_radius[group_index].push_back(hitbox_radius);
	collision_group_node_positions[group_index].push_back({0,0});

	return true;
}


//Removes objective from collision checking
bool BulletMap::RemoveObjective(const String& group_name, const Node2D* node) {
	std::unordered_map<std::string,int>::const_iterator collision_group = collision_groups.find(group_name.utf8().get_data());
	bool result = false;

	if (collision_group == collision_groups.end()) {
		UtilityFunctions::push_warning("Invalid collision group for node removal: " + group_name);
		return result;
	}

	const int group_index = collision_group->second;
	int array_size = collision_group_nodes[group_index].size() - 1;

	for (int node_index = array_size; node_index > -1; node_index--){
		if (collision_group_nodes[group_index][node_index] == node) {
			collision_group_nodes[group_index][node_index] = collision_group_nodes[group_index][array_size];
			collision_group_node_radius[group_index][node_index] = collision_group_node_radius[group_index][array_size];
			collision_group_node_positions[group_index][node_index] = collision_group_node_positions[group_index][array_size];
			
			array_size--;
			result = true;
		}
	}

	collision_group_nodes[group_index].resize(array_size + 1);
	collision_group_node_radius[group_index].resize(array_size + 1);
	collision_group_node_positions[group_index].resize(array_size + 1);

	return result;
}


//Stop processing on this script
void BulletMap::Pause() {
	paused = true;
	allow_shooting = false;
}


//Use to resume processing 
void BulletMap::Unpause() {
	paused = false;
	allow_shooting = true;
}

//Use to stop/allow shooting for external scripts. Use IsClearingBullets() to avoid desync
void BulletMap::AllowShooting(bool allow) {
	if (!clearing_bullets) {
		allow_shooting = allow;
	}
}





//Movement Callables

void BulletMap::MovementDefaultNoRender(double delta, int bucket_index) {
	std::vector<int>& bucket = movement_buckets[bucket_index];
	float* buffer = bullet_buffer.ptrw();
	int array_size = bucket.size() - 1;
	double sine = 0.0f;
	double cosine = 0.0f;

	for (int index = array_size; index > -1; index--) {
		int& bullet = bucket[index];

		if (bullet_lifetime[bullet] > 0) {
			sine = std::sin(bullet_rotation[bullet]);
			cosine = std::cos(bullet_rotation[bullet]);

			bullet_rotation[bullet] += bullet_angular_velocity[bullet];

			buffer[bullet * buffer_unit + 3] += cosine * bullet_speed[bullet] * delta;
			buffer[bullet * buffer_unit + 7] += sine * bullet_speed[bullet] * delta;
		}
		else {
			bullet = bucket[array_size];
			array_size--;
		}
	}

	bucket.resize(array_size + 1);
}


void BulletMap::MovementDefault(double delta, int bucket_index) {
	std::vector<int>& bucket = movement_buckets[bucket_index];
	float* buffer = bullet_buffer.ptrw();
	int array_size = bucket.size() - 1;
	double sine = 0.0f;
	double cosine = 0.0f;
	double visual_size = 0.0f;

	for (int index = array_size; index > -1; index--) {
		int& bullet = bucket[index];

		if (bullet_lifetime[bullet] > 0) {
			visual_size = bullet_visuals_size[bullet];
			sine = std::sin(bullet_rotation[bullet]);
			cosine = std::cos(bullet_rotation[bullet]);

			bullet_rotation[bullet] += bullet_angular_velocity[bullet];

			buffer[bullet * buffer_unit] = cosine * visual_size; //x.x
			buffer[bullet * buffer_unit + 1] = -sine * visual_size; //y.x
			buffer[bullet * buffer_unit + 2];
			buffer[bullet * buffer_unit + 3] += cosine * bullet_speed[bullet] * delta;
			buffer[bullet * buffer_unit + 4] = sine * visual_size; //x.y
			buffer[bullet * buffer_unit + 5] = cosine * visual_size; //y.y
			buffer[bullet * buffer_unit + 6];
			buffer[bullet * buffer_unit + 7] += sine * bullet_speed[bullet] * delta;
		}
		else {
			bullet = bucket[array_size];
			array_size--;
		}
	}

	bucket.resize(array_size + 1);
}
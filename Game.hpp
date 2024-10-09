#pragma once

#include "Scene.hpp"

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <string>
#include <list>
#include <array>
#include <random>

struct Connection;

//Game state, separate from rendering.

//Currently set up for a "client sends controls" / "server sends whole state" situation.

enum class Message : uint8_t {
	C2S_Controls = 1, //Greg!
	C2S_Handshake = 2, //Get the player role
	S2C_State = 's',
	//...
};

//used to represent a control input:
struct Button {
	uint8_t downs = 0; //times the button has been pressed
	bool pressed = false; //is the button pressed now
};

enum PlayerType : uint8_t {
	RedHamster = 0,
	BlueHamster = 1,
	Spectator = 2,
	Uninitialized = 3,
};

//state of one player in the game:
struct Player {
	//player inputs (sent from client):
	struct Controls {
		Button left, right, up, down, jump, LMB;

		float mouse_x = 0.0f;

		void send_controls_message(Connection *connection) const;

		//returns 'false' if no message or not a controls message,
		//returns 'true' if read a controls message,
		//throws on malformed controls message
		bool recv_controls_message(Connection *connection);
	} controls;

	//player state (sent from server):
	bool dead = false;
	int8_t health = 10;
	float since_attack = 0.0f;
	glm::quat rotation = glm::quat();
	glm::quat lance_rotation = glm::quat();
	glm::vec3 velocity = glm::vec3(0.0f);
	glm::vec3 lance_position = glm::vec3();
	glm::quat wheel_rotation = glm::quat();
	glm::vec3 position = glm::vec3();

	float cur_lance_angle = 0.0f;

	Player() = default;
};

struct Game {
	std::array< Player, 2 > players; //two hamsters
	std::array< Player, 2 > initial_player_state;
	PlayerType player_type = Spectator;

	Player *spawn_player(); //add player the end of the players list (may also, e.g., play some spawn anim)
	void remove_player(Player *); //remove player from game (may also, e.g., play some despawn anim)

	uint8_t next_player_number = 0; //used for keeping track of who is in the game and who is spectating
	bool player_ready[2] = {false, false};

	enum GameState {
		WaitingForPlayer,
		InGame,
		Ended,
	} game_state = GameState::WaitingForPlayer;

	Game();

	//state update function:
	void update(float elapsed);

	// game scene
	Scene main_scene_server;

	Scene::Transform *lance_tip_transform[2] = {nullptr, nullptr};

	//constants:
	//the update rate on the server:
	inline static constexpr float Tick = 1.0f / 30.0f;

	//arena size:
	inline static constexpr glm::vec2 ArenaMin = glm::vec2(-22.5f, -22.5f);
	inline static constexpr glm::vec2 ArenaMax = glm::vec2( 22.5f,  22.5f);

	//player constants:
	inline static constexpr float PlayerRadius = 1.1f;
	inline static constexpr float PlayerSpeed = 20.0f;
	inline static constexpr float PlayerAccelHalflife = 1.0f;

	//---- game state helpers ----
	void reset_hamsters();

	//---- communication helpers ----

	//used by client:
	//set game state from data in connection buffer
	// (return true if data was read)
	bool recv_state_message(Connection *connection);

	//used by server:
	//send game state.
	//  Will move "connection_player" to the front of the front of the sent list.
	void send_state_message(Connection *connection, Player *connection_player = nullptr) const;
};

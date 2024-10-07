#pragma once

#include <glm/glm.hpp>

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

//state of one player in the game:
struct Player {
	//player inputs (sent from client):
	struct Controls {
		Button left, right, up, down, jump;

		void send_controls_message(Connection *connection) const;

		//returns 'false' if no message or not a controls message,
		//returns 'true' if read a controls message,
		//throws on malformed controls message
		bool recv_controls_message(Connection *connection);
	} controls;

	//player state (sent from server):
	glm::vec2 position = glm::vec2(0.0f, 0.0f);
	glm::vec2 velocity = glm::vec2(0.0f, 0.0f);

	glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);
	enum PlayerType : uint8_t {
		RedHamster = 0,
		BlueHamster = 1,
		Spectator = 2,
		Uninitialized = 3,
	} player_type = PlayerType::Spectator;

	Player();
	Player(uint8_t player_index);
};

struct Game {
	std::array< Player, 3 > players; //last one is spectator
	Player *spawn_player(); //add player the end of the players list (may also, e.g., play some spawn anim)
	void remove_player(Player *); //remove player from game (may also, e.g., play some despawn anim)

	uint8_t next_player_number = 0; //used for naming players and keeping track of who is in the game and who is spectating

	Game();

	//state update function:
	void update(float elapsed);

	//constants:
	//the update rate on the server:
	inline static constexpr float Tick = 1.0f / 30.0f;

	//arena size:
	inline static constexpr glm::vec2 ArenaMin = glm::vec2(-22.5f, -22.5f);
	inline static constexpr glm::vec2 ArenaMax = glm::vec2( 22.5f,  22.5f);

	//player constants:
	inline static constexpr float PlayerRadius = 0.06f;
	inline static constexpr float PlayerSpeed = 2.0f;
	inline static constexpr float PlayerAccelHalflife = 0.25f;

	//hamster positions
	const glm::vec3 red_hamster_start = {1.0f, -22.0f, 1.8477f};
	const glm::vec3 blue_hamster_start = {1.0f, 22.0f, 1.8477f};

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

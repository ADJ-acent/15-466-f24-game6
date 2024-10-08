#include "Mode.hpp"

#include "Scene.hpp"
#include "Connection.hpp"
#include "Game.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode(Client &client);
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//ui rendering
	void draw_ui(glm::uvec2 const &drawable_size);

	void draw_start_menu(glm::uvec2 const &drawable_size);

	void RenderText(std::string text, float x, float y, float scale, glm::vec3 color, glm::uvec2 const &drawable_size);

	//----- game state -----

	//handle new game state passed from server
	void update_to_server_state();

	//input tracking for local player:
	Player::Controls controls;

	//latest game state (from server):
	Game game;

	//local copy of scene
	Scene scene;

	//player camera
	Scene::Camera *camera = nullptr;

	//last message from server:
	std::string server_message;

	//hamsters
	Hamster hamster_red, hamster_blue;

	//connection to server:
	Client &client;

};

#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"
#include "DrawLines.hpp"
#include "gl_errors.hpp"
#include "Mesh.hpp"
#include "data_path.hpp"
#include "hex_dump.hpp"
#include "UIRenderProgram.hpp"
#include "FontRenderProgram.hpp"
// for image import
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "Font.hpp"


#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <random>
#include <array>

extern Load< UIRenderProgram > ui_render_program;

GLuint main_meshes_for_lit_color_texture_program = 0;
GLuint hamster_tex = 0;
GLuint wall_tex = 0;
GLuint blue_hamster_UI = 0;
GLuint red_hamster_UI = 0;
GLuint health_UI_fill = 0;
GLuint main_menu = 0;

Load< Font > font(LoadTagDefault, []() -> Font const * {
	return new Font(data_path("ui/Fredoka-Medium.ttf"));
});

Load< MeshBuffer > main_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("arena.pnct"));
	main_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< void > load_texture(LoadTagDefault, []() -> void {
	stbi_set_flip_vertically_on_load(true);
	auto load_tex_to_GL = [&](std::string const &filename) {
		int width, height, channels;
		stbi_uc* data = stbi_load(filename.c_str(), &width, &height, &channels, 4); 
		if (data == nullptr) {
			std::cerr << "Failed to load texture: "<< filename<<", "<< stbi_failure_reason() << std::endl;
			assert(false);
			return GLuint(0);
		}

		GLuint tex;
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		GL_ERRORS();
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		GL_ERRORS();
		glGenerateMipmap(GL_TEXTURE_2D); 

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D, 0);
		stbi_image_free(data);
		GL_ERRORS();
		return tex;
	};

	hamster_tex = load_tex_to_GL(data_path("textures/hamster_tex.png"));
	wall_tex = load_tex_to_GL(data_path("textures/colormap.png"));
	red_hamster_UI = load_tex_to_GL(data_path("ui/HamsterHealthRed.png"));
	blue_hamster_UI = load_tex_to_GL(data_path("ui/HamsterHealthBlue.png"));
	health_UI_fill = load_tex_to_GL(data_path("ui/HamsterHealthFill.png"));
	main_menu = load_tex_to_GL(data_path("ui/MainMenu.png"));

});


Load< Scene > main_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("arena.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = main_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = main_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;
		if (mesh_name.substr(0,7) == "Hamster") {
			drawable.pipeline.textures[0].texture = hamster_tex;
		}
		else if (mesh_name.substr(0,5) == "Scene") {
			drawable.pipeline.textures[0].texture = wall_tex;
		}
	});
});


PlayMode::PlayMode(Client &client_) : client(client_) {
	scene = *main_scene;

	for (auto &transform : scene.transforms) {
		if (transform.name == "RedHamster") {
			hamster_red.hamster_transform = &transform;
		}
		else if (transform.name == "BlueHamster") {
			hamster_blue.hamster_transform = &transform;
		}
		else if (transform.name == "BlueLance") {
			hamster_blue.lance_transform = &transform;
		}
		else if (transform.name == "BlueWheel") {
			hamster_blue.wheel_transform = &transform;
		}
		else if (transform.name == "RedLance") {
			hamster_red.lance_transform = &transform;
		}
		else if (transform.name == "RedWheel") {
			hamster_red.wheel_transform = &transform;
		}
	}
	auto camera_it = scene.cameras.begin();
	std::advance(camera_it,2);
	camera = &(*camera_it);
}

PlayMode::~PlayMode() {
}

void PlayMode::update_to_server_state()
{
	auto camera_it = scene.cameras.begin();
	if (game.game_state != Game::WaitingForPlayer) {
		std::advance(camera_it,static_cast<uint8_t>(game.player_type));
		camera = &(*camera_it);
	}
	else { //everyone should spectate when game is missing player
		std::advance(camera_it,2);
		camera = &(*camera_it);
	}
	hamster_red.hamster_transform->position = game.players[0].position;
	hamster_red.hamster_transform->rotation = game.players[0].rotation;
	hamster_red.wheel_transform->rotation = game.players[0].wheel_rotation;
	hamster_red.lance_transform->rotation = game.players[0].lance_rotation;
	hamster_red.lance_transform->position = game.players[0].lance_position;

	hamster_blue.hamster_transform->position = game.players[1].position;
	hamster_blue.hamster_transform->rotation = game.players[1].rotation;
	hamster_blue.wheel_transform->rotation = game.players[1].wheel_rotation;
	hamster_blue.lance_transform->rotation = game.players[1].lance_rotation;
	hamster_blue.lance_transform->position = game.players[1].lance_position;

}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.repeat) {
			//ignore repeats
		} else if (evt.key.keysym.sym == SDLK_a) {
			controls.left.downs += 1;
			controls.left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			controls.right.downs += 1;
			controls.right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			controls.up.downs += 1;
			controls.up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			controls.down.downs += 1;
			controls.down.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			controls.jump.downs += 1;
			controls.jump.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
		} else if (evt.key.keysym.sym == SDLK_e && game.game_state == Game::WaitingForPlayer) {
			game.send_handshake_message(&client.connection);
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			controls.left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			controls.right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			controls.up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			controls.down.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			controls.jump.pressed = false;
			return true;
		}
	} else if (evt.type == SDL_MOUSEBUTTONDOWN) {
		if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
			SDL_SetRelativeMouseMode(SDL_TRUE);
			return true;
		}
		else {
			controls.LMB.downs += 1;
			controls.LMB.pressed = true;
			return true;
		}
	}else if (evt.type == SDL_MOUSEBUTTONUP) {
			controls.LMB.pressed = false;
			return true;
	} else if (evt.type == SDL_MOUSEMOTION) {
		if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
			controls.mouse_x = evt.motion.xrel / float(window_size.y);
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {

	//queue data for sending to server:
	if (game.player_type != Spectator && game.game_state == Game::GameState::InGame) {
		controls.send_controls_message(&client.connection);
	}

	//reset button press counters:
	controls.left.downs = 0;
	controls.right.downs = 0;
	controls.up.downs = 0;
	controls.down.downs = 0;
	controls.jump.downs = 0;
	controls.LMB.downs = 0;
	controls.mouse_x = 0.0f;

	//send/receive data:
	client.poll([this](Connection *c, Connection::Event event){
		if (event == Connection::OnOpen) {
			std::cout << "[" << c->socket << "] opened" << std::endl;
		} else if (event == Connection::OnClose) {
			std::cout << "[" << c->socket << "] closed (!)" << std::endl;
			throw std::runtime_error("Lost connection to server!");
		} else { assert(event == Connection::OnRecv);
			//std::cout << "[" << c->socket << "] recv'd data. Current buffer:\n" << hex_dump(c->recv_buffer); std::cout.flush(); //DEBUG
			bool handled_message;
			try {
				do {
					handled_message = false;
					if (game.recv_state_message(c)) {
						handled_message = true;
						update_to_server_state();
					}
				} while (handled_message);
			} catch (std::exception const &e) {
				std::cerr << "[" << c->socket << "] malformed message from server: " << e.what() << std::endl;
				//quit the game:
				throw e;
			}
		}
	}, 0.0);

}

void PlayMode::draw(glm::uvec2 const &drawable_size) {

	if (game.game_state == Game::WaitingForPlayer) {
		draw_start_menu(drawable_size);
		return;
	}

	camera->aspect = float(drawable_size.x) / float(drawable_size.y);
	
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.435f, 0.80f, 1.0f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	GL_ERRORS(); //print any errors produced by this setup code

	scene.draw(*camera);

	GL_ERRORS();

	draw_ui(drawable_size);
}


void PlayMode::draw_ui(glm::uvec2 const &drawable_size)
{
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  
	unsigned int VAO, VBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 4, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
    // activate corresponding render state	
	glUseProgram(ui_render_program->program);
	float aspect = float(drawable_size.y) / float(drawable_size.x);
	glm::mat4 projection = glm::ortho(0.0f, 1280.0f, 0.0f, 1280.0f * aspect);
	glUniformMatrix4fv(ui_render_program->PROJECTION_mat4,  1, GL_FALSE, glm::value_ptr(projection));

	GL_ERRORS();
    glActiveTexture(GL_TEXTURE0);
	GL_ERRORS();
    glBindVertexArray(VAO);
	GL_ERRORS();


	assert(game.game_state != Game::WaitingForPlayer);
	{ // Health bar rendering... Its so bad don't look
		const float health_size = 256.0f;
		float xpos = health_size / 8.0f;
		float ypos = - health_size / 4.0f;
		float w = health_size;
		float h = health_size;
		float red_health = float(game.players[0].health) / 25.0f*0.65f;
		float blue_health = float(game.players[1].health) / 25.0f* 0.65f;
		if (game.player_type == BlueHamster) {
			glUniform3f(ui_render_program->TexColor_vec3, 0.3f, 0.5f, 0.9f);
			w = 0.35f * w + blue_health * w;
		}
		else {
			glUniform3f(ui_render_program->TexColor_vec3, 1.0f, 0.3f, 0.3f);
			w = 0.35f * w + red_health * w;
		}
		float vertices_health_bottom[4][4] = {
			{ xpos, ypos,   0.0f, 0.0f }, // Top-left
			{ xpos + w, ypos, 1.0f, 0.0f },  // Top-right      
			{ xpos, ypos + h, 0.0f, 1.0f }, // Bottom-left
			{ xpos + w, ypos + h, 1.0f, 1.0f }, // Bottom-right
		};
		glBindTexture(GL_TEXTURE_2D, health_UI_fill);
		// update content of VBO memory
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices_health_bottom), vertices_health_bottom); 
		// render triangle strip
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		xpos = health_size / 8.0f;
		ypos = - health_size / 4.0f;
		w = health_size;
		h = health_size;
		float vertices_bottom[4][4] = {
			{ xpos, ypos,   0.0f, 0.0f }, // Top-left
			{ xpos + w, ypos, 1.0f, 0.0f },  // Top-right      
			{ xpos, ypos + h, 0.0f, 1.0f }, // Bottom-left
			{ xpos + w, ypos + h, 1.0f, 1.0f }, // Bottom-right
		};

		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices_bottom), vertices_bottom); 
		glUniform3f(ui_render_program->TexColor_vec3, 1.0f, 1.0f, 1.0f);
		if (game.player_type == BlueHamster) {
			glBindTexture(GL_TEXTURE_2D, blue_hamster_UI);
		}
		else {
			glBindTexture(GL_TEXTURE_2D, red_hamster_UI);
		}
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		GL_ERRORS();
		xpos = health_size * 0.35f;
		ypos = health_size * 0.25f;
		w = health_size * 0.7f;
		h = w;

		if (game.player_type == BlueHamster) {
			glUniform3f(ui_render_program->TexColor_vec3, 1.0f, 0.3f, 0.3f);
			w = 0.35f * w + red_health * w;
		}
		else {
			glUniform3f(ui_render_program->TexColor_vec3, 0.3f, 0.5f, 0.9f);
			w = 0.35f * w + blue_health * w;
		}
		float vertices_health_top [4][4] = {
			{ xpos, ypos,   0.0f, 0.0f }, // Top-left
			{ xpos + w, ypos, 1.0f, 0.0f },  // Top-right      
			{ xpos, ypos + h, 0.0f, 1.0f }, // Bottom-left
			{ xpos + w, ypos + h, 1.0f, 1.0f }, // Bottom-right
		};
		glBindTexture(GL_TEXTURE_2D, health_UI_fill);
		// update content of VBO memory
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices_health_top), vertices_health_top); 
		// render triangle strip
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);


		xpos = health_size * 0.35f;
		ypos = health_size * 0.25f;
		w = health_size * 0.7f;
		h = w;
		float vertices_top[4][4] = {
			{ xpos, ypos,   0.0f, 0.0f }, // Top-left
			{ xpos + w, ypos, 1.0f, 0.0f },  // Top-right      
			{ xpos, ypos + h, 0.0f, 1.0f }, // Bottom-left
			{ xpos + w, ypos + h, 1.0f, 1.0f }, // Bottom-right
		};
		if (game.player_type == BlueHamster) {
			glBindTexture(GL_TEXTURE_2D, red_hamster_UI);
		}
		else {
			glBindTexture(GL_TEXTURE_2D, blue_hamster_UI);
		}

		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices_top), vertices_top); 

		glUniform3f(ui_render_program->TexColor_vec3, 1.0f, 1.0f, 1.0f);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}

	glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_BLEND);
	glUseProgram(0);
	
	if (game.game_state == Game::Ended) {
		if (game.players[0].health <= 0) {
			if (game.players[1].health <= 0) {
				RenderText("Tied?!?!", 900.0f, 500.f, 1.5f, glm::vec3(1.0f, 1.0f, 1.0f), drawable_size);
			}
			else {
				RenderText("Blue Hamster Wins!", 650.0f, 600.f, 1.5f, glm::vec3(0.0f, 0.3f, 0.3f), drawable_size);
			}
		}
		else if (game.players[1].health <= 0) {
			RenderText("Red Hamster Wins!", 675.0f, 600.f, 1.5f, glm::vec3(.7f, 0.05f, 0.05f), drawable_size);
		}
		else { // player disconnected and got removed
			RenderText("A player has disconnected...", 575.0f, 600.f, 1.5f, glm::vec3(.7f, 0.1f, 0.0f), drawable_size);
		}
		RenderText("going back to the main menu in 5 seconds...", 500.0f, 500.f, 1.0f, glm::vec3(.7f, .7f, .7f), drawable_size);
	}
	
	GL_ERRORS();
}

void PlayMode::draw_start_menu(glm::uvec2 const &drawable_size)
{	
	if (game.player_type == PlayerType::RedHamster) {
		glClearColor(0.5255f, 0.1843f, 0.20392f, 1.0f);
	}
	else if (game.player_type == PlayerType::BlueHamster) {
		glClearColor(0.15294f, 0.2902f, 0.651f, 1.0f);
	}
	else {
		glClearColor(1.0f, 0.753f, 0.1216f, 1.0f);
	}
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if (game.player_type == PlayerType::Spectator) {
		RenderText("Press 'E' to get ready!", 250, 70, 1.0f, glm::vec3(0.3f, 0.3f, 0.3f), drawable_size);
	}
	else {
		RenderText("Ready!", 400, 70, 1.0f, glm::vec3(0.8f, 0.8f, 0.8f), drawable_size);
	}
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


	glEnable(GL_FRAMEBUFFER_SRGB);
	unsigned int VAO, VBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 4, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
    // activate corresponding render state	
	glUseProgram(ui_render_program->program);
	float aspect = float(drawable_size.y) / float(drawable_size.x);
	glm::mat4 projection = glm::ortho(0.0f, 1920.0f, 0.0f, 1920.0f * aspect);
	// main menu is 1920 * 1080
	glUniformMatrix4fv(ui_render_program->PROJECTION_mat4,  1, GL_FALSE, glm::value_ptr(projection));
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);
	const float canvas_w = 1920.0f;
	const float canvas_h = 1920.0f * aspect;
	constexpr float image_aspect = 1080.0f / 1920.0f;
	float w = 1920.0f;
	float h = 1080.0f;
	float offset_x = 0.0f;
	float offset_y = 0.0f;
	if (canvas_h < 1080.0f) {
		h = canvas_h;
		w = canvas_h / image_aspect;
		offset_x = (canvas_w - w) / 2.0f;
	}
	else if (canvas_h > 1080.0f) {
		w = canvas_w;
		h = canvas_w * image_aspect;
		offset_y = (canvas_h - h) / 2.0f;
	}

	float vertices_bottom[4][4] = {
		{ offset_x, offset_y,   0.0f, 0.0f }, // Top-left
		{ offset_x + w, offset_y, 1.0f, 0.0f },  // Top-right      
		{ offset_x, offset_y + h, 0.0f, 1.0f }, // Bottom-left
		{ offset_x + w, offset_y + h, 1.0f, 1.0f }, // Bottom-right
	};
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices_bottom), vertices_bottom); 
	glUniform3f(ui_render_program->TexColor_vec3, 1.0f, 1.0f, 1.0f);

	glBindTexture(GL_TEXTURE_2D, main_menu);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_BLEND);
	glUseProgram(0);

	GL_ERRORS();
}


void PlayMode::RenderText(std::string text, float x, float y, float scale, glm::vec3 color, glm::uvec2 const &drawable_size)
{
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	unsigned int VAO, VBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	GL_ERRORS();
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
    // activate corresponding render state	
	GL_ERRORS();
	glUseProgram(font_render_program->program);
	float aspect = float(drawable_size.y) / float(drawable_size.x);
	glm::mat4 projection = glm::ortho(0.0f, 1920.0f, 0.0f, 1920.0f * aspect);
	glUniformMatrix4fv(font_render_program->PROJECTION_mat4,  1, GL_FALSE, glm::value_ptr(projection));

    glUniform3f(font_render_program->TexColor_vec3, color.x, color.y, color.z);
	GL_ERRORS();
    glActiveTexture(GL_TEXTURE0);
	GL_ERRORS();
    glBindVertexArray(VAO);
	GL_ERRORS();
    // iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++)
    {
		char character = *c;
        Font::Character ch = font->characters.find(character)->second;

        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;
        // update VBO for each character
        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },            
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }           
        };
        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        // update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); 
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (ch.Advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64)
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_BLEND);
	glUseProgram(0);
	GL_ERRORS();
}
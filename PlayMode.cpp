#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"
#include "DrawLines.hpp"
#include "gl_errors.hpp"
#include "Mesh.hpp"
#include "data_path.hpp"
#include "hex_dump.hpp"
#include "UIRenderProgram.hpp"
// for image import
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


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
	std::cout <<"loaded hamster texture: "<<hamster_tex<<std::endl;
	wall_tex = load_tex_to_GL(data_path("textures/colormap.png"));
	std::cout <<"loaded wall texture: "<<wall_tex<<std::endl;
	red_hamster_UI = load_tex_to_GL(data_path("ui/HamsterHealthRed.png"));
	std::cout <<"loaded red hamster health ui: "<<red_hamster_UI<<std::endl;
	blue_hamster_UI = load_tex_to_GL(data_path("ui/HamsterHealthBlue.png"));
	std::cout <<"loaded blue hamster health ui: "<<blue_hamster_UI<<std::endl;
	health_UI_fill = load_tex_to_GL(data_path("ui/HamsterHealthFill.png"));
	std::cout <<"loaded hamster health fill ui: "<<health_UI_fill<<std::endl;


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
	std::advance(camera_it,static_cast<uint8_t>(game.player_type));
	camera = &(*camera_it);
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
	if (game.player_type == Spectator) return false;
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
	controls.send_controls_message(&client.connection);

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

	camera->aspect = float(drawable_size.x) / float(drawable_size.y);
	
	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	// {//let player know they are dead
	// 	float aspect = float(drawable_size.x) / float(drawable_size.y);
	// 	DrawLines lines(glm::mat4(
	// 		1.0f / aspect, 0.0f, 0.0f, 0.0f,
	// 		0.0f, 1.0f, 0.0f, 0.0f,
	// 		0.0f, 0.0f, 1.0f, 0.0f,
	// 		0.0f, 0.0f, 0.0f, 1.0f
	// 	));
	// 	constexpr float H = 0.09f;
	// 	if (game_end) {
	// 		glClearColor(0.8f, .7569f, .5f, 1.0f);
	// 		float ofs = 6.0f / drawable_size.y;
	// 		lines.draw_text("DEAD HAMSTER",
	// 			glm::vec3(-float(drawable_size.x)*H / 200.0f, 0.35f, 0.0),
	// 			glm::vec3(H*3, 0.0f, 0.0f), glm::vec3(0.0f, H*3, 0.0f),
	// 			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
	// 		lines.draw_text("DEAD HAMSTER",
	// 			glm::vec3(-float(drawable_size.x)*H / 200.0f + ofs, ofs +0.35f, 0.0),
	// 			glm::vec3(H*3, 0.0f, 0.0f), glm::vec3(0.0f, H*3, 0.0f),
	// 			glm::u8vec4(0xff, 0x00, 0x00, 0x00));
	// 		lines.draw_text("Press 'r' to restart",
	// 			glm::vec3(-float(drawable_size.x)*H / 225.0f, -0.5f, 0.0),
	// 			glm::vec3(H*2.0f, 0.0f, 0.0f), glm::vec3(0.0f, H*2.0f, 0.0f),
	// 			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
	// 		lines.draw_text("Press 'r' to restart",
	// 			glm::vec3(-float(drawable_size.x)*H  / 225.0f+ofs, -.5f, 0.0),
	// 			glm::vec3(H*2, 0.0f, 0.0f), glm::vec3(0.0f, H*2.0f, 0.0f),
	// 			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	// 		lines.draw_text("Score: " + std::to_string(score),
	// 			glm::vec3(-float(drawable_size.x)*H / 350.0f, -.25f, 0.0),
	// 			glm::vec3(H*2, 0.0f, 0.0f), glm::vec3(0.0f, H*2, 0.0f),
	// 			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
	// 		lines.draw_text("Score: " + std::to_string(score),
	// 			glm::vec3(-float(drawable_size.x)*H / 350.0f + ofs, -0.25f + ofs, 0.0),
	// 			glm::vec3(H*2, 0.0f, 0.0f), glm::vec3(0.0f, H*2, 0.0f),
	// 			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	// 		glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	// 		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// 		GL_ERRORS(); //print any errors produced by this setup code
	// 		return;
	// 	}
	// }

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

	const float health_size = 256.0f;
	float xpos = health_size / 8.0f;
	float ypos = - health_size / 4.0f;
	float w = health_size;
	float h = health_size;
	float red_health = std::clamp(float(game.players[0].health) / 25.0f, 0.0f, 0.75f);
	float blue_health = std::clamp(float(game.players[1].health) / 25.0f, 0.0f, 0.75f);
	
	if (game.player_type == BlueHamster) {
		glUniform3f(ui_render_program->TexColor_vec3, 0.3f, 0.5f, 0.9f);
		w = 0.25f * w + blue_health * w;
	}
	else {
		glUniform3f(ui_render_program->TexColor_vec3, 1.0f, 0.3f, 0.3f);
		w = 0.25f * w + red_health * w;
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
		w = 0.25f * w + red_health * w;
	}
	else {
		glUniform3f(ui_render_program->TexColor_vec3, 0.3f, 0.5f, 0.9f);
		w = 0.25f * w + blue_health * w;
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
	GL_ERRORS();

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
	GL_ERRORS();
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices_top), vertices_top); 
	GL_ERRORS();
	glUniform3f(ui_render_program->TexColor_vec3, 1.0f, 1.0f, 1.0f);
	GL_ERRORS();
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	GL_ERRORS();
	glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_BLEND);
	glUseProgram(0);

	GL_ERRORS();
}
#pragma once
#include <ft2build.h>

#include <hb.h>
#include <hb-ft.h>
#include <glm/glm.hpp>

#include <unordered_map>
#include <string>

/**
 * Handles loading fonts and uploading textures to GL, 
 * adpated from https://learnopengl.com/In-Practice/Text-Rendering
 * 
 */
struct Font {
        struct Character {
        unsigned int TextureID;  // ID handle of the glyph texture
        glm::ivec2   Size;       // Size of glyph
        glm::ivec2   Bearing;    // Offset from baseline to left/top of glyph
        uint32_t Advance;    // Offset to advance to next glyph
    };

    std::unordered_map<char, Character> characters;

    Font(std::string font_path);
};
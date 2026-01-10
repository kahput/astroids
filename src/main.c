#include "audio_manager.h"
#include "world.h"

typedef struct {
	Arena frame_arena;

	float dt;
	uint32_t window_width, window_height;

	Shader flash_shader;

	bool32 show_debug, show_ui;
} GameContext;

typedef Rectangle TextureArea;

#ifdef PLATFORM_WEB
const char *FLASH_SHADER_CODE =
	"precision mediump float;\n"
	"varying vec2 fragTexCoord;\n"
	"varying vec4 fragColor;\n"
	"uniform sampler2D texture0;\n"
	"void main()\n"
	"{\n"
	"    vec4 texelColor = texture2D(texture0, fragTexCoord);\n"
	"    if (texelColor.a <= 0.1) discard;\n"
	"    gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);\n"
	"}\n";
#else
const char *FLASH_SHADER_CODE =
	"#version 330\n"
	"in vec2 fragTexCoord;\n"
	"in vec4 fragColor;\n"
	"out vec4 finalColor;\n"
	"uniform sampler2D texture0;\n"
	"void main()\n"
	"{\n"
	"    vec4 texelColor = texture(texture0, fragTexCoord);\n"
	"    if (texelColor.a <= 0.1) discard;\n"
	"    finalColor = vec4(1.0, 1.0, 1.0, 1.0f);\n"
	"}\n";
#endif

int main(void) {
	InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Astroids");
	SetTargetFPS(60);
	audio_initialize();

	Texture atlas = LoadTexture("assets/sprites/asteroid_sprite.png");
	Texture paddle_texture = LoadTexture("assets/sprites/boss_paddle.png");
	Shader flash_shader = LoadShaderFromMemory(NULL, FLASH_SHADER_CODE);

	GameWorld world = { 0 };
	world_init(&world, &atlas, &paddle_texture, &flash_shader);
	SetExitKey(KEY_NULL);

	Color DARK = { 20, 20, 20, 255 };
	while (world.running && WindowShouldClose() == false) {
		float dt = GetFrameTime();
		audio_update(dt);

		world_update(&world, dt);

		BeginDrawing();
		world_draw(&world);
		EndDrawing();

		arena_reset(&world.frame);
	}

	audio_unload();
	UnloadShader(flash_shader);
	CloseWindow();

	arena_destroy(&world.frame);
}

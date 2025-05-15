#define COLOR_LIB_IMPLEMENTATION
#include "colors.h"
#include "colorutil.h"
#include "rlgl.h"
#include <raylib.h>
#include <raymath.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#define MAX_SAMPLES 100000
#define MAX_COLORS 40000
#define CUBE_SIDE_LEN 0.05f

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

#define PALETTE_SIZE 16

Image target_image;
Texture2D target_image_tex;
struct image_info info = {0};

void UpdateTexturesFromFilename(char *filename) {
  // TODO: add free command
  for (int i = 0; i < 4; i++) {
    free(info.transform_list[i]);
  }
  free(info.transform_list);
  UnloadTexture(target_image_tex);
  UnloadImage(target_image);
  Texture texture = LoadTexture(filename);
  Image loaded_image = LoadImageFromTexture(texture);
  process_image(&info, loaded_image);
  target_image = loaded_image;
  target_image_tex = texture;
}

void GotFileFromEmscripten(char *filename) {
  printf("got file %s\n", filename);

  UpdateTexturesFromFilename(filename);
}

void Draw_Image_In_Region(Texture2D tex, Rectangle region) {
  Rectangle src = (Rectangle){0, 0, tex.width, tex.height};

  bool height_greater = tex.width < tex.height;
  Rectangle dest;
  float image_ratio = (float)tex.height / (float)tex.width;

  // TODO: this is messed for vertical images
  // also add define for the region to display images
  // in
  if (height_greater) {
    size_t new_width = region.width / image_ratio;
    dest = (Rectangle){(region.x + region.width - new_width), region.y,
                       new_width, region.height};
  } else {
    dest = (Rectangle){region.x, region.y, region.width,
                       region.height * image_ratio};
  }

  DrawTexturePro(tex, src, dest, (Vector2){0, 0}, 0, WHITE);
}

bool color_in_list(Color cur_color, uint8_t *drawn_pixel_map) {
  // use cur color to see what to check
  unsigned int index = (cur_color.r / 255.0f) + (cur_color.g / 255.0f) * (256) +
                       (cur_color.b / 255.0f) * (256 * 256);
  unsigned int byte_index = index / (8);
  unsigned int cur_bitfield = drawn_pixel_map[byte_index];
  bool present = (cur_bitfield >> (index - (byte_index * 8))) & 0x1;
  if (!present) {
    // need to set in pixel map
    drawn_pixel_map[byte_index] |= 1 << (index - (byte_index * 8));
  }
  return present;
}

int populate_color_list(Image target_image, Color *color_list,
                        uint8_t *drawn_pixel_map) {
  int color_cnt = 0;
  for (int i = 0; i < MAX_SAMPLES; i++) {
    if (color_cnt > MAX_COLORS) {
      break;
    }
    Vector2 coord = {rand() % target_image.width, rand() % target_image.height};
    Color color = GetImageColor(target_image, coord.x, coord.y);
    if (!color_in_list(color, drawn_pixel_map)) {
      color_list[color_cnt++] = color;
    }
  }
  return color_cnt;
}

void load_transforms_from_color_list(Matrix *transform_list, Color *color_list,
                                     int color_cnt, unsigned int quadrant) {
  Vector3 quadrant_lookup[4] = {(Vector3){5, 5, 5}, (Vector3){-5, 5, -5},
                                (Vector3){5, 5, -5}, (Vector3){-5, 5, 5}};
  for (int i = 0; i < color_cnt; i++) {
    Color cur_color = color_list[i];
    Vector3 pos;
    pos.x = ((double)cur_color.r / 255) * quadrant_lookup[quadrant].x;
    pos.y = ((double)cur_color.g / 255) * quadrant_lookup[quadrant].y;
    pos.z = ((double)cur_color.b / 255) * quadrant_lookup[quadrant].z;

    transform_list[i] = MatrixTranslate(pos.x, pos.y, pos.z);
  }
}

void process_image(struct image_info *info, Image target_image) {

  // struct image_info info = {0};
  //  info.drawn_pixel_map = calloc(1, (256 * 256 * 256) / (8 *
  //  sizeof(uint8_t)));
  memset(info->drawn_pixel_map, 0, (256 * 256 * 256) / (8 * sizeof(uint8_t)));
  memset(info->color_list, 0, sizeof(Color) * MAX_COLORS);
  memset(info->palette, 0, sizeof(Color) * PALETTE_SIZE);
  memset(info->palette_color_names, 0, sizeof(char *) * PALETTE_SIZE);
  info->num_pixels = target_image.width * target_image.height;
  // info.color_list = malloc(MAX_COLORS * sizeof(Color));
  // info.palette = malloc(PALETTE_SIZE * sizeof(Color));

  info->color_cnt = populate_color_list(target_image, info->color_list,
                                        info->drawn_pixel_map);

  // generate the palette from the randomly sampled colors
  info->palette_len = gen_median_palette_from_color_list(
      (ColorStruct *)&info->palette[0], PALETTE_SIZE,
      (ColorStruct *)&info->color_list[0], info->color_cnt);
  for (int i = 0; i < info->palette_len; i++) {
    info->palette_color_names[i] = find_closest_color(
        info->palette[i].r, info->palette[i].g, info->palette[i].b);
  }

  printf("found %ld unique colors\n", info->color_cnt);
  printf("Got a palette length %ld\n", info->palette_len);

  info->transform_list = malloc(4 * sizeof(Matrix));

  for (int i = 0; i < 4; i++) {
    info->transform_list[i] = malloc(sizeof(Matrix) * info->color_cnt);
    load_transforms_from_color_list(info->transform_list[i], info->color_list,
                                    info->color_cnt, i);
  }
}

void init_info(struct image_info *info) {
  info->drawn_pixel_map = calloc(1, (256 * 256 * 256) / (8 * sizeof(uint8_t)));
  info->color_list = malloc(MAX_COLORS * sizeof(Color));
  info->palette = malloc(PALETTE_SIZE * sizeof(Color));
  info->palette_color_names = malloc(PALETTE_SIZE * sizeof(char *));
}

uint64_t get_current_ms() {

#ifdef __EMSCRIPTEN__
  uint64_t ms_timestamp = emscripten_get_now();
#else
  struct timespec time;
  clock_gettime(CLOCK_MONOTONIC_RAW, &time);
  uint64_t ms_timestamp = (time.tv_sec) * 1000 + (time.tv_nsec) / 1000000;
#endif
  return ms_timestamp;
}

void Draw_And_Render_Copy_Particles(particle *particles) {
  // iterate through the particles draw and update locations
  for (int i = 0; i < NUM_PARTICLES; i++) {

    if (info.copy_particles[i].alive) {

      uint64_t ms_timestamp = get_current_ms();
      if (info.copy_particles[i].particle_lifetime >
          ms_timestamp - info.copy_particles[i].start_time) {
        // interpolate color alpha
        float progress =
            (float)(ms_timestamp - info.copy_particles[i].start_time) /
            info.copy_particles[i].particle_lifetime;
        info.copy_particles[i].particle_color.a =
            255 - ((-(cosf(PI * progress) - 1) / 2) * 255);

        // need to render and move the particle
        //            DrawRectangleV(info.copy_particles[i].location,
        //            (Vector2){20, 20},
        //                          info.copy_particles[i].color);
        char particle_text[50];

        snprintf(particle_text, 49, "Copied %s hex code to clipboard",
                 info.copy_particles[i].clicked_color_name);

        // shift so text fits on screen
        uint16_t particle_right_padding = 20;
        if (particle_right_padding + MeasureText(particle_text, 24) +
                info.copy_particles[i].location.x >
            SCREEN_WIDTH) {
          info.copy_particles[i].location.x -=
              (MeasureText(particle_text, 24) +
               info.copy_particles[i].location.x + particle_right_padding) -
              SCREEN_WIDTH;
        }
        DrawText(particle_text, info.copy_particles[i].location.x,
                 info.copy_particles[i].location.y, 24,
                 info.copy_particles[i].particle_color);
        // advance position
        info.copy_particles[i].location.x += info.copy_particles[i].velocity.x;
        info.copy_particles[i].location.y += info.copy_particles[i].velocity.y;
      } else {
        info.copy_particles[i].alive = false;
      }
    }
  }
}

void Add_Particle(particle *particles, const particle *new_particle) {
  uint64_t max_timestamp = UINT64_MAX;
  int32_t oldest_particle_index = -1;
  for (int i = 0; i < NUM_PARTICLES; i++) {
    // find the first that is not alive
    if (!particles[i].alive) {
      particles[i] = *new_particle;
      snprintf(particles[i].text, 49, "Copied %s hex code to clipboard",
               particles[i].clicked_color_name);
      return;
    }
    if (particles[i].start_time < max_timestamp) {
      max_timestamp = particles[i].start_time;
      oldest_particle_index = i;
    }
  }
  // didnt find any that were empty
  // replace the oldest
  printf("replacing oldest particle at index %d\n", oldest_particle_index);
  particles[oldest_particle_index] = *new_particle;

  snprintf(particles[oldest_particle_index].text, 49,
           "Copied %s hex code to clipboard",
           particles[oldest_particle_index].clicked_color_name);
}

typedef struct {
  float h; // Hue in degrees [0, 360]
  float s; // Saturation in percentage [0, 100]
  float v; // Value in percentage [0, 100]
} HSV;

HSV rgb_to_hsv(unsigned char r, unsigned char g, unsigned char b) {
  float rf = r / 255.0f;
  float gf = g / 255.0f;
  float bf = b / 255.0f;

  float max = (rf > gf) ? ((rf > bf) ? rf : bf) : ((gf > bf) ? gf : bf);
  float min = (rf < gf) ? ((rf < bf) ? rf : bf) : ((gf < bf) ? gf : bf);
  float delta = max - min;

  HSV hsv = {
      0, 0,
      max *
          100}; // Initialize hue to 0, saturation to 0, value to max percentage

  if (delta > 0.00001f) {
    hsv.s = (max > 0.0f) ? (delta / max) * 100.0f : 0.0f;

    if (max == rf) {
      hsv.h = 60.0f * (fmodf(((gf - bf) / delta), 6.0f));
    } else if (max == gf) {
      hsv.h = 60.0f * (((bf - rf) / delta) + 2.0f);
    } else { // max == bf
      hsv.h = 60.0f * (((rf - gf) / delta) + 4.0f);
    }

    if (hsv.h < 0.0f) {
      hsv.h += 360.0f;
    }
  }

  return hsv;
}

int main(int argc, char *argv[]) {
  // goal is to load image
  // and graph the color of each pixel
  // on 3d graph
  const int scr_width = SCREEN_WIDTH;
  const int scr_height = SCREEN_HEIGHT;
  srand(1);

  if (argc > 1) {
    const char *filename = argv[1];
    if (!FileExists(filename)) {
      printf("file %s does not exist\n", filename);
      exit(1);
    }
    target_image = LoadImage(filename);
  } else {
    // if no file is provided use default image
    target_image = LoadImage("resources/oceansmall.png");
  }

  Image color_wheel = LoadImage("resources/color_wheel.png");

  InitWindow(scr_width, scr_height, "image color grapher");
  SetTargetFPS(120);

  Texture color_wheel_texture = LoadTextureFromImage(color_wheel);
  init_info(&info);
  target_image_tex = LoadTextureFromImage(target_image);

  Camera camera = {0};
  camera.position = (Vector3){10.0f, 10.0f, 10.0f}; // Camera position
  camera.target = (Vector3){0.0f, 0.0f, 0.0f};      // Camera looking at point
  camera.up =
      (Vector3){0.0f, 1.0f, 0.0f}; // Camera up vector (rotation towards target)
  camera.fovy = 45.0f;             // Camera field-of-view Y
  camera.projection = CAMERA_PERSPECTIVE; // Camera mode type

  // maybe instead need to find the unique pixels?
  // to reduce number needed
  //

  Mesh my_small_sphere = GenMeshSphere(CUBE_SIDE_LEN, 4, 8);

  // Load lighting shader
  Shader shader = LoadShader("resources/lighting_instancing_100.vs",
                             "resources/lighting_100.fs");

  // Get shader locations
  shader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(shader, "mvp");
  shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader, "viewPos");
  shader.locs[SHADER_LOC_MATRIX_MODEL] =
      GetShaderLocationAttrib(shader, "instanceTransform");

  // Set shader value: ambient light level
  int ambientLoc = GetShaderLocation(shader, "ambient");
  SetShaderValue(shader, ambientLoc, (float[4]){0.2f, 0.2f, 0.2f, 1.0f},
                 SHADER_UNIFORM_VEC4);

  Material matInstances = LoadMaterialDefault();
  matInstances.shader = shader;
  printf("making color list\n");

  process_image(&info, target_image);

  //--------------------------------------------------------------------------------------

  // Custom file dialog

  // Main game loop
  while (!WindowShouldClose()) // Detect window close button or ESC key
  {

    // Update
    //----------------------------------------------------------------------------------
    UpdateCamera(&camera, CAMERA_ORBITAL);

    float cameraPos[3] = {camera.position.x, camera.position.y,
                          camera.position.z};
    SetShaderValue(shader, shader.locs[SHADER_LOC_VECTOR_VIEW], cameraPos,
                   SHADER_UNIFORM_VEC3);
    rlEnableDepthTest();
    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();

    rlEnableDepthTest();

    ClearBackground(BLACK);

    BeginMode3D(camera);

    DrawGrid(10, 1.0f); // Draw a grid
    DrawCylinderEx((Vector3){-5, 0, 0}, (Vector3){5, 0, 0}, .1, .1, 12, RED);
    DrawCylinderEx((Vector3){0, 0, 0}, (Vector3){0, 5, 0}, .1, .1, 12, GREEN);
    DrawCylinderEx((Vector3){0, 0, -5}, (Vector3){0, 0, 5}, .1, .1, 12, BLUE);

    // draw all quadrants
    DrawMeshInstanced(my_small_sphere, matInstances, info.transform_list[0],
                      info.color_cnt);
    DrawMeshInstanced(my_small_sphere, matInstances, info.transform_list[1],
                      info.color_cnt);
    DrawMeshInstanced(my_small_sphere, matInstances, info.transform_list[2],
                      info.color_cnt);
    DrawMeshInstanced(my_small_sphere, matInstances, info.transform_list[3],
                      info.color_cnt);

    EndMode3D();

    DrawFPS(10, 10);

    Draw_Image_In_Region(target_image_tex,
                         (Rectangle){SCREEN_WIDTH - 200, 0, 200, 200});
    DrawText("Drag and Drop Image Or Upload in Top Left", 0, SCREEN_HEIGHT - 20,
             20, WHITE);

    // draw the palette
    // in the middle of the screen
    uint16_t pallete_color_width = 40;
    uint16_t padding = 2;

    uint16_t start_x =
        SCREEN_WIDTH / 2 -
        ((padding + pallete_color_width) * info.palette_len + padding) / 2;
    uint16_t color_y = SCREEN_HEIGHT - 80;
    DrawRectangle(start_x, color_y - padding,
                  padding * info.palette_len +
                      pallete_color_width * info.palette_len + padding,
                  pallete_color_width + padding + padding, DARKGRAY);
    start_x += padding;
    for (int i = 0; i < info.palette_len; i++) {
      uint32_t cur_x = start_x + padding * i;
      Rectangle color_rect =
          (Rectangle){cur_x, color_y, pallete_color_width, pallete_color_width};
      DrawRectangleRec(color_rect, info.palette[i]);
      if (CheckCollisionPointRec(GetMousePosition(), color_rect)) {
        //        printf("Got mouse inside of rect with color (%d,%d,%d)\n",
        //              info.palette[i].r, info.palette[i].g,
        //              info.palette[i].b);

        // Draw backgroundrectangle
        ColorStruct tooltip_color = {.r = info.palette[i].r,
                                     .g = info.palette[i].g,
                                     .b = info.palette[i].b,
                                     .a = info.palette[i].a};
        Color tooltip_background =
            calculate_luminance(tooltip_color) > 127 ? DARKGRAY : LIGHTGRAY;
        DrawRectangle(cur_x - padding, color_y - 100, 100 + padding * 2, 100,
                      tooltip_background);

        uint16_t x_location_with_padding = start_x + padding * i;

        DrawRectangle(x_location_with_padding, color_y - 60 - padding, 60, 60,
                      info.palette[i]);
        const char *color_name = info.palette_color_names[i];

        // printf("Closest named color to (%d,%d,%d) = %s\n", info.palette[i].r,
        //       info.palette[i].g, info.palette[i].b, color_name);
        DrawText(color_name, x_location_with_padding, color_y - 80, 12,
                 info.palette[i]);
        if (IsMouseButtonPressed(0) &&
            CheckCollisionPointRec(GetMousePosition(), color_rect)) {
          char color_buf[20];
          snprintf(color_buf, 20, "#%x%x%x", info.palette[i].r,
                   info.palette[i].g, info.palette[i].b);
          SetClipboardText(color_buf);
          printf("Mouse button pressed while on color %s\n", color_buf);
          uint64_t ms_timestamp = get_current_ms();

          particle new_particle = (particle){.start_time = ms_timestamp,
                                             .particle_lifetime = 1000,
                                             .location = GetMousePosition(),
                                             .velocity = (Vector2){0, -1.5},
                                             .alive = true,
                                             .particle_color = WHITE,
                                             .clicked_color_name = color_name};
          Add_Particle(&info.copy_particles[0], &new_particle);
        }

        // draw color_wheel
        HSV hsv =
            rgb_to_hsv(info.palette[i].r, info.palette[i].g, info.palette[i].b);
        uint16_t color_wheel_radius = 50;

        uint16_t color_wheel_y = color_y - 220;
        Draw_Image_In_Region(color_wheel_texture,
                             (Rectangle){x_location_with_padding, color_wheel_y,
                                         color_wheel_radius * 2,
                                         color_wheel_radius * 2});
        // find location in color_wheel
        Vector2 color_wheel_center = {x_location_with_padding +
                                          color_wheel_radius,
                                      color_wheel_y + color_wheel_radius};
        Vector2 hsv_to_cartesian = {
            color_wheel_radius *
                (((float)hsv.s / 100) * sinf(hsv.h * (PI / 180))),
            color_wheel_radius *
                (((float)hsv.s / 100) * -cosf(hsv.h * (PI / 180)))};

        Vector2 color_location = {color_wheel_center.x + hsv_to_cartesian.x,
                                  color_wheel_center.y + hsv_to_cartesian.y};
        uint16_t color_location_radius = 5;
        DrawCircleLines(color_location.x - (float)color_location_radius / 2,
                        color_location.y - (float)color_location_radius / 2,
                        color_location_radius, WHITE);

        // h hue in degrees v will be
      }

      start_x += pallete_color_width;
    }

    uint16_t cursor_size = 10;
    DrawRectangleLines(GetMousePosition().x - cursor_size / 2,
                       GetMousePosition().y - cursor_size / 2, cursor_size,
                       cursor_size, WHITE);

    // handle and render copied code text particle
    Draw_And_Render_Copy_Particles(&info.copy_particles[0]);

    char *version_string = "v0.1";
    int version_len = MeasureText(version_string, 20);
    int version_padding = 10;
    DrawText(version_string, SCREEN_WIDTH - version_len - version_padding,
             SCREEN_HEIGHT - 20, 20, WHITE);

    // raygui: controls drawing
    //--------------------------------------------------------------------------------

    //----------------------------------------------------------------------------------

    EndDrawing();

    // handle file dropping
    if (IsFileDropped()) {
      FilePathList files = LoadDroppedFiles();
      for (int i = 0; i < files.count; i++) {
        printf("got file dropped %s\n", files.paths[i]);
        // after this can load the file into texture
        // and sample the image
        Image test_load = LoadImage(files.paths[i]);
        Texture2D test_texture = LoadTextureFromImage(test_load);
        printf("texture id = %d\n", test_texture.id);
        if (test_texture.id == 0) {
          // invalid texture
          printf("Error unable to load %s\n", files.paths[i]);
          break;
        }
        for (int i = 0; i < 4; i++) {
          free(info.transform_list[i]);
        }
        free(info.transform_list);
        UnloadImage(target_image);
        UnloadTexture(target_image_tex);
        target_image = test_load;
        target_image_tex = test_texture;

        process_image(&info, target_image);
        break;
      }
      // tells the engine we handled the files
      UnloadDroppedFiles(files);
    }

    //----------------------------------------------------------------------------------
  }
  CloseWindow();
  return EXIT_SUCCESS;
}

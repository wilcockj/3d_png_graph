#include "colorutil.h"
#include "rlgl.h"
#include <raylib.h>
#include <raymath.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SAMPLES 100000
#define MAX_COLORS 40000
#define CUBE_SIDE_LEN 0.05f

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

#define PALETTE_SIZE 8

Image target_image;
Texture2D target_image_tex;
struct image_info info;

void UpdateTexturesFromFilename(char *filename) {
  UnloadTexture(target_image_tex);
  UnloadImage(target_image);
  Texture texture = LoadTexture(filename);
  Image loaded_image = LoadImageFromTexture(texture);
  info = process_image(loaded_image);
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
    dest = (Rectangle){region.x, 0, region.width, region.height * image_ratio};
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
typedef enum {
  RED_WIDEST,
  GREEN_WIDEST,
  BLUE_WIDEST,
  NONE_WIDEST
} Color_Wideness;

Color_Wideness get_widest_color_in_bucket(Color *color_list,
                                          size_t color_count) {
  float red_range[2] = {99999, 0};
  float green_range[2] = {99999, 0};
  float blue_range[2] = {99999, 0};
  for (int i = 0; i < color_count; i++) {
    Color cur_color = color_list[i];
    if (cur_color.r < red_range[0]) {
      red_range[0] = cur_color.r;
    }
    if (cur_color.r > red_range[1]) {
      red_range[1] = cur_color.r;
    }

    if (cur_color.g < green_range[0]) {
      green_range[0] = cur_color.g;
    }
    if (cur_color.g > green_range[1]) {
      green_range[1] = cur_color.g;
    }

    if (cur_color.b < blue_range[0]) {
      blue_range[0] = cur_color.b;
    }
    if (cur_color.b > blue_range[1]) {
      blue_range[1] = cur_color.b;
    }
  }
  float red_width = red_range[1] - red_range[0];
  float green_width = green_range[1] - green_range[0];
  float blue_width = blue_range[1] - blue_range[0];

  if (red_width > green_width && red_width > blue_width && red_width > -90000) {
    printf("Red is widest at %f\n", red_width);
    return RED_WIDEST;
  } else if (green_width > blue_width && green_width > red_width &&
             green_width > -90000) {
    printf("Green is widest at %f\n", green_width);
    return GREEN_WIDEST;
  } else if (blue_width > -90000) {
    printf("Blue is widest at %f\n", blue_width);
    return BLUE_WIDEST;
  } else {
    return NONE_WIDEST;
  }
}

int red_greater(const void *a, const void *b) {
  Color color1 = *(const Color *)a;
  Color color2 = *(const Color *)b;

  if (color1.r > color2.r) {
    return 1;
  }
  if (color2.r > color1.r) {
    return -1;
  }
  return 0;
}

int green_greater(const void *a, const void *b) {
  Color color1 = *(const Color *)a;
  Color color2 = *(const Color *)b;

  if (color1.g > color2.g) {
    return 1;
  }
  if (color2.g > color1.g) {
    return -1;
  }
  return 0;
}

int blue_greater(const void *a, const void *b) {
  Color color1 = *(const Color *)a;
  Color color2 = *(const Color *)b;

  if (color1.b > color2.b) {
    return 1;
  }
  if (color2.b > color1.b) {
    return -1;
  }
  return 0;
}

Color fetch_average_color(Color *color_list, size_t len) {
  uint64_t color[3] = {0};
  for (int i = 0; i < len; i++) {
    color[0] += color_list[i].r;
    color[1] += color_list[i].g;
    color[2] += color_list[i].b;
  }
  color[0] /= len;
  color[1] /= len;
  color[2] /= len;
  return (Color){color[0], color[1], color[2]};
}

size_t gen_palette_from_color_list(Color *palette, uint8_t palette_size,
                                   Color *color_list, size_t color_count) {
  // go through color list,
  // find largest range color
  size_t *palette_lens = calloc(palette_size, sizeof(size_t));
  uint16_t palette_color_idx = 0;
  bool need_exit_palette_loop = false;

  for (int i = 0; i < palette_size; i++) {
    Color_Wideness widest_color =
        get_widest_color_in_bucket(&color_list[0], color_count);
    // now need to sort by that color
    switch (widest_color) {
    case RED_WIDEST:
      qsort(&color_list[0], color_count, sizeof(Color), red_greater);
      break;

    case GREEN_WIDEST:
      qsort(&color_list[0], color_count, sizeof(Color), green_greater);
      break;
    case BLUE_WIDEST:
      qsort(&color_list[0], color_count, sizeof(Color), blue_greater);
      break;
    case NONE_WIDEST:
      need_exit_palette_loop = true;
      break;
    }

    if (need_exit_palette_loop) {
      break;
    }
    // need to divide the bucket

    size_t temp_color_count = color_count;
    color_count = color_count / 2;
    palette_lens[palette_color_idx++] = temp_color_count - color_count;
  }

  size_t color_list_idx = 0;
  size_t pallete_idx = 0;
  for (int i = palette_size - 1; i >= 0; i--) {
    printf("pallete %d len is %ld, color_list_idx = %ld\n", i, palette_lens[i],
           color_list_idx);
    if (palette_lens[i] < 2) {
      color_list = &color_list[palette_lens[i]];

      continue;
    }
    palette[pallete_idx++] =
        fetch_average_color(&color_list[0], palette_lens[i]);
    palette[pallete_idx - 1].a = 255; // make not see through
    printf("made past average colorizing\n");
    // color_list_idx += palette_lens[i];
    color_list = &color_list[palette_lens[i]];
  }

  for (int i = 0; i < pallete_idx; i++) {
    printf("Palette color %d is (%d,%d,%d)\n", i, palette[i].r, palette[i].g,
           palette[i].b);
  }

  free(palette_lens);
  return pallete_idx;
}

struct image_info process_image(Image target_image) {

  struct image_info info = {0};
  info.drawn_pixel_map = calloc(1, (256 * 256 * 256) / (8 * sizeof(uint8_t)));
  info.num_pixels = target_image.width * target_image.height;
  info.color_list = malloc(MAX_COLORS * sizeof(Color));
  info.palette = malloc(PALETTE_SIZE * sizeof(Color));

  info.color_cnt =
      populate_color_list(target_image, info.color_list, info.drawn_pixel_map);

  info.palette_len = gen_palette_from_color_list(
      &info.palette[0], PALETTE_SIZE, &info.color_list[0], info.color_cnt);

  printf("found %d unique colors\n", info.color_cnt);
  printf("Got a palette length %d\n", info.palette_len);

  info.transform_list = malloc(4 * sizeof(Matrix));

  for (int i = 0; i < 4; i++) {
    info.transform_list[i] = malloc(sizeof(Matrix) * info.color_cnt);
    load_transforms_from_color_list(info.transform_list[i], info.color_list,
                                    info.color_cnt, i);
  }

  return info;
}

int main(int argc, char *argv[]) {
  // goal is to load image
  // and graph the color of each pixel
  // on 3d graph
  const int scr_width = SCREEN_WIDTH;
  const int scr_height = SCREEN_HEIGHT;
  const float circle_radius = 0.1;
  const float graph_limit = 5;
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

  InitWindow(scr_width, scr_height, "image color grapher");
  SetTargetFPS(120);

  target_image_tex = LoadTextureFromImage(target_image);

  Camera camera = {0};
  camera.position = (Vector3){10.0f, 10.0f, 10.0f}; // Camera position
  camera.target = (Vector3){0.0f, 0.0f, 0.0f};      // Camera looking at point
  camera.up =
      (Vector3){0.0f, 1.0f, 0.0f}; // Camera up vector (rotation towards target)
  camera.fovy = 45.0f;             // Camera field-of-view Y
  camera.projection = CAMERA_PERSPECTIVE; // Camera mode type

  Vector3 center = {0, 0, 0};

  // maybe instead need to find the unique pixels?
  // to reduce number needed
  //

  Mesh my_pyr = GenMeshPoly(4, 1);
  Mesh my_cube = GenMeshCube(1.0f, 1.0f, 1.0f);
  Mesh my_small_sphere = GenMeshSphere(
      CUBE_SIDE_LEN, 4,
      8); // GenMeshCube(CUBE_SIDE_LEN, CUBE_SIDE_LEN, CUBE_SIDE_LEN);
  Material matIntances = LoadMaterialDefault();

  Material matDefault = LoadMaterialDefault();

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

  info = process_image(target_image);

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

    //
    uint16_t start_x = 0;
    uint16_t pallete_color_width = 20;
    uint16_t padding = 2;
    DrawRectangle(start_x, SCREEN_HEIGHT - 60 - padding,
                  padding * info.palette_len +
                      pallete_color_width * info.palette_len,
                  pallete_color_width + padding + padding, DARKGRAY);
    for (int i = 0; i < info.palette_len; i++) {
      DrawRectangle(start_x + padding * i, SCREEN_HEIGHT - 60,
                    pallete_color_width, pallete_color_width, info.palette[i]);
      start_x += pallete_color_width;
    }

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
        // TODO: add free command
        for (int i = 0; i < 4; i++) {
          free(info.transform_list[i]);
        }
        free(info.transform_list);
        free(info.color_list);
        free(info.drawn_pixel_map);
        UnloadImage(target_image);
        UnloadTexture(target_image_tex);
        target_image = test_load;
        target_image_tex = test_texture;

        info = process_image(target_image);
        break;
      }
      // tells the engine we handled the files
      UnloadDroppedFiles(files);
    }

    //----------------------------------------------------------------------------------
  }
  return EXIT_SUCCESS;
}
